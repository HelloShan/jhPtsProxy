#include "ProxyServer.h"
#include "xptClient.h"
#include "sha2_interface.h"

int ProxyServer::init(uint16 port,XptClient *xptclient)
{
	m_port = port;
	m_listen = 0;

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(SOCKADDR_IN));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=ADDR_ANY;
	if( bind(s,(SOCKADDR*)&addr,sizeof(SOCKADDR_IN)) == SOCKET_ERROR )
	{
		closesocket(s);
		return -1;
	}
	if (0 != listen(s, 64))
	{
		printf("listen error\n");
		closesocket(s);
		return -1;
	}
	m_listen = s;
	m_xptclient = xptclient;
	m_wd = NULL;
	printf("proxy listen port:%d\n",port);
	return 0;
}

bool ProxyServer::dealListen()
{
	FD_SET fd;
	timeval sTimeout;
	sTimeout.tv_sec = 0;
	sTimeout.tv_usec = 250000;
	FD_ZERO(&fd);
	FD_SET(m_listen,&fd);
	
	int r = select((int)m_listen+1, &fd, 0, 0, &sTimeout); 
	if (r<=0)
	{
		//printf("ProxyServer::dealListen have nothing\n");
		return false;
	}
	SOCKADDR_IN addr;
	int len=sizeof(SOCKADDR_IN);
	SOCKET s = accept(m_listen, (SOCKADDR*)&addr, &len);

	// set socket as non-blocking
#ifdef __WIN32__
	unsigned int nonblocking=1;
	unsigned int cbRet;
#endif
	WSAIoctl(s, FIONBIO, &nonblocking, sizeof(nonblocking), NULL, 0, (LPDWORD)&cbRet, NULL, NULL);

	printf("accept proxy client,%s:%d\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
	m_clients.push_back(s);
	return true;

}

bool ProxyServer::recvCmd(uint32 *p,SOCKET s)
{
	uint32 index = 0;
	uint32 len = 4;
	int ret = 0;
	while(index<len)
	{
		ret = recv(s,((char*)p)+index,len-index,0);
		if (ret <= 0)
		{
			if( WSAGetLastError() != WSAEWOULDBLOCK )
			{
				//printf("ProxyServer::recvCmd error,this client will disconnect\n");
				return false;
			}
		}
		index += ret;
	}
	return true;
}

bool ProxyServer::sendBlock(SOCKET s,uint32 n)
{
	static uint32 seed = 0;

	if (NULL == m_wd)
	{
		//no work data,do nothing
		return true;
	}
	
	uint32 transactionDataLength = m_wd->coinBase1Size+m_wd->coinBase2Size+4;
	uint8 *transactionData = (uint8*)new char[transactionDataLength];
	if (!transactionData)
	{
		printf("ProxyServer::sendBlock memory error\n");
		return false;
	}
	uint32 cmd = (n<<8)|CMD_ACK_BLOCK;
	cmd = ntohl_ex(cmd);
	send(s,(char*)&cmd,4,0);
	memcpy(transactionData,m_wd->coinBase1,m_wd->coinBase1Size);
	memcpy(transactionData+m_wd->coinBase1Size+4,m_wd->coinBase2,m_wd->coinBase2Size);
	for(uint32 i=0;i<n;i++)
	{
		memcpy(transactionData+m_wd->coinBase1Size,&seed,4);
		
		seed++;
		SHA2_INTERFACE::instance->SHA2_sha256(transactionData,transactionDataLength,m_wd->block.merkleRoot);
		SHA2_INTERFACE::instance->SHA2_sha256(m_wd->block.merkleRoot,32,m_wd->block.merkleRoot);
		
		send(s,(char*)&m_wd->block,sizeof(BlockInfo),0);
	}

	delete[] transactionData;
	return true;
}

bool ProxyServer::recvShare(SOCKET s)
{
	uint32 index = 0;
	uint32 len = sizeof(SubmitInfo);
	int ret = 0;
	SubmitInfo *p = new SubmitInfo();
	if (!p)
	{
		printf("ProxyServer::recvShare memory error\n");
		return false;
	}
	while(index<len)
	{
		ret = recv(s,((char*)p)+index,len-index,0);
		if (ret <= 0)
		{
			if( WSAGetLastError() != WSAEWOULDBLOCK )
			{
				//printf("ProxyServer::recvShare error,this client will disconnect\n");
				delete p;
				return false;
			}
		}
		index += ret;
	}
	m_xptclient->pushSubmit(p);
	return true;
}

void ProxyServer::closeClient(SOCKET s)
{
	SOCKADDR_IN sa;
	int len=sizeof(sa);
	getpeername(s,(SOCKADDR *)&sa,&len);
	printf("close proxy client,%s:%d\n",inet_ntoa(sa.sin_addr),ntohs(sa.sin_port));
	closesocket(s);
}

bool ProxyServer::dealClients()
{
	FD_SET fd;
	FD_ZERO(&fd);
	timeval sTimeout;
	sTimeout.tv_sec = 0;
	sTimeout.tv_usec = 250000;
	for (uint32 i = 0;i < m_clients.size();i++)
	{
		FD_SET(m_clients[i],&fd);
	}

	int r = select(0, &fd, 0, 0, &sTimeout); 
	if (r<=0)
	{
		//printf("ProxyServer::dealClients have nothing\n");
		return false;
	}
	
	for (vector<SOCKET>::iterator it = m_clients.begin();it != m_clients.end();)
	{
		if(FD_ISSET(*it,&fd))
		{
			uint32 cmd = 0;
			if (!recvCmd(&cmd,*it))
			{
				closeClient(*it);
				it = m_clients.erase(it);
				continue;
			}
			else
			{
				cmd = ntohl_ex(cmd);
				switch(cmd&0xFF)
				{
				case CMD_REQ_BLOCK:
					if (!sendBlock(*it,cmd>>8))
					{
						closeClient(*it);
						it = m_clients.erase(it);
						continue;
					}
					break;
				case CMD_SMT_SHARE:
					if(!recvShare(*it))
					{
						closeClient(*it);
						it = m_clients.erase(it);
						continue;
					}
					break;
				default:
					printf("ProxyServer::dealClients unk opt,%d\n",cmd&0xFF);
					break;
				}
			}
		}
		++it;
	}

	return true;
}

void ProxyServer::sendNewBlock()
{
	if (m_clients.empty())
		return;

	uint32 cmd = CMD_NEW_BLOCK;
	cmd = ntohl_ex(cmd);
	for (uint32 i = 0;i < m_clients.size();i++)
	{
		send(m_clients[i],(char*)&cmd,4,0);
	}
}


THREAD_FUN ProxyServer::main()
{
	while (1)
	{
		while (m_listen)
		{
			if (false ==dealListen())
				break;
		}

		if (m_xptclient->CheakNewWork())
		{
			if (m_wd)
			{
				delete m_wd;
			}
			WorkData *p=m_xptclient->getWorkData();
			if (p)
			{
				m_wd = p;
			}
			//proxy client need to clear work queue
			sendNewBlock();
		}

		if (!m_clients.empty())
		{
			dealClients();
		}

		Sleep(10);
	}

	return 0;
}