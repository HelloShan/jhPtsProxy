#include "ProxyServer.h"
#include "xptClient.h"
#include "sha2_interface.h"

int ProxyServer::init(int port,XptClient *xptclient)
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
	
	int r = select(m_listen+1, &fd, 0, 0, &sTimeout); 
	if (r<=0)
	{
		printf("ProxyServer::dealListen have nothing\n");
		return false;
	}
	SOCKADDR addr;
	int len=sizeof(SOCKADDR);
	SOCKET s = accept(m_listen, &addr, &len);

	// set socket as non-blocking
#ifdef __WIN32__
	unsigned int nonblocking=1;
	unsigned int cbRet;
#endif
	WSAIoctl(s, FIONBIO, &nonblocking, sizeof(nonblocking), NULL, 0, (LPDWORD)&cbRet, NULL, NULL);

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
				printf("XptClient::recvOpt error,this client will disconnect\n");
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
	uint32 cmd = (n<<24)|CMD_ACK_BLOCK;
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
				printf("XptClient::recvOpt error,this client will disconnect\n");
				delete p;
				return false;
			}
		}
		index += ret;
	}
	m_xptclient->pushSubmit(p);
	return true;
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
		printf("ProxyServer::dealClients have nothing\n");
		return false;
	}
	
	for (vector<SOCKET>::iterator it = m_clients.begin();it != m_clients.end();)
	{
		if(FD_ISSET(*it,&fd))
		{
			uint32 cmd = 0;
			if (!recvCmd(&cmd,*it))
			{
				closesocket(*it);
				m_clients.erase(it++);
				continue;
			}
			else
			{
				cmd = ntohl_ex(cmd);
				switch(cmd|0xFF)
				{
				case CMD_REQ_BLOCK:
					if (!sendBlock(*it,cmd>>8))
					{
						closesocket(*it);
						m_clients.erase(it++);
						continue;
					}
					break;
				case CMD_SMT_SHARE:
					if(!recvShare(*it))
					{
						closesocket(*it);
						m_clients.erase(it++);
						continue;
					}
					break;
				default:
					printf("ProxyServer::dealClients unk opt \n");
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