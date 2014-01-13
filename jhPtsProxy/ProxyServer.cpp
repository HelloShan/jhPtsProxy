#include "ProxyServer.h"
#include "xptClient.h"
#include "sha2_interface.h"
#include <time.h>

ProxyServer *ProxyServer::instance=0;
int ProxyServer::init(uint16 port)
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
		printf("proxy server bind error,port %d\n",port);
		closesocket(s);
		return -1;
	}
	if (0 != listen(s, 64))
	{
		printf("listen proxy port %d error\n",port);
		closesocket(s);
		return -1;
	}
	m_listen = s;
	m_xptclient = XptClient::getInstance();
	m_wd = NULL;
	sha256_func = SHA2_INTERFACE::getInstance()->getSHA2_256();
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
	
	int r = select(m_listen+1, &fd, 0, 0, &sTimeout); 
	if (r<=0)
	{
		//printf("ProxyServer::dealListen have nothing\n");
		return false;
	}
	SOCKADDR_IN addr;
	socklen_t len=sizeof(SOCKADDR_IN);
	SOCKET s = accept(m_listen, (SOCKADDR*)&addr, &len);

	// set socket as non-blocking
#ifdef __WIN32__
	unsigned int nonblocking=1;
	unsigned int cbRet;
#endif
	WSAIoctl(s, FIONBIO, &nonblocking, sizeof(nonblocking), NULL, 0, (LPDWORD)&cbRet, NULL, NULL);

	printf("accept proxy client,%s:%d\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
	m_clients.push_back(s);

	string ip = inet_ntoa(addr.sin_addr);
	PeerVal v;
	map<string,PeerVal>::iterator it = m_peers.insert(make_pair(ip,v)).first;
	it->second.isonline = true;

	return true;

}

bool ProxyServer::recvData(uint32 len,char *buf,SOCKET s)
{
	uint32 index = 0;
	int ret = 0;
	while(index<len)
	{
		ret = recv(s,buf+index,len-index,0);
		if (0 == ret/* || SOCKET_ERROR == ret*/)
		{
			return false;
		}
		if (ret < 0)
		{
			int n = WSAGetLastError();
			//WSAENETRESET,WSAECONNABORTED,WSAECONNRESET
			if (n >=10052 && n <= 10054)
			{
				printf("ProxyServer::recvData error,%d\n",n);
				return false;
			}
			continue;
			if( WSAGetLastError() != WSAEWOULDBLOCK )
			{
				//printf("ProxyServer::recvCmd error,this client will disconnect\n");
				return false;
			}
		}/*
		if (ret == 0)
		{
			//return false;
		}*/
		index += ret;
	}
	return true;
}

bool ProxyServer::recvCmd(uint32 *p,SOCKET s)
{
	return recvData(4,(char*)p,s);
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
	if (send(s,(char*)&cmd,4,0) <= 0)
	{
		printf("ProxyServer::sendBlock send cmd error!!!!!\n");
	}
	//printf("ProxyServer::sendBlock send cmd,%08X\n",cmd);
	memcpy(transactionData,m_wd->coinBase1,m_wd->coinBase1Size);
	memcpy(transactionData+m_wd->coinBase1Size+4,m_wd->coinBase2,m_wd->coinBase2Size);
	for(uint32 i=0;i<n;i++)
	{
		m_wd->block.uniqueMerkleSeed = ntohl_ex(seed);
		memcpy(transactionData+m_wd->coinBase1Size,&seed,4);
		
		seed++;
		sha256_func(transactionData,transactionDataLength,m_wd->block.merkleRoot);
		sha256_func(m_wd->block.merkleRoot,32,m_wd->block.merkleRoot);
		
		if(send(s,(char*)&m_wd->block,sizeof(BlockInfo),0) <=0)
		{
			printf("ProxyServer::sendBlock send block error!!!!!\n");
		}
		//printf("ProxyServer::sendBlock send one block info,height %d\n",ntohl_ex(m_wd->block.height));
	}

	delete[] transactionData;
	return true;
}

bool ProxyServer::recvShare(SOCKET s)
{
	SubmitInfo *p = new SubmitInfo();
	if (!p)
	{
		printf("ProxyServer::recvShare memory error\n");
		return false;
	}
	if (!recvData(sizeof(SubmitInfo),(char*)p,s))
	{
		delete p;
		return false;
	}
	m_xptclient->pushSubmit(p);

	SOCKADDR_IN addr;
	socklen_t len=sizeof(addr);
	getpeername(s,(SOCKADDR *)&addr,&len);

	string ip = inet_ntoa(addr.sin_addr);
	m_peers[ip].share++;

	return true;
}

void ProxyServer::closeClient(SOCKET s)
{
	SOCKADDR_IN addr;
	socklen_t len=sizeof(addr);
	getpeername(s,(SOCKADDR *)&addr,&len);

	string ip = inet_ntoa(addr.sin_addr);
	m_peers[ip].isonline = false;

	printf("close proxy client,%s:%d\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
	closesocket(s);
}

void ProxyServer::getMinersState(string &msg)
{
	char buf[100] = {0};
	if (m_peers.empty())
	{
		msg = "NULL!!!\n";
		return;
	}
	msg = "";
	string tmp;
	int on = 0,off = 0;
	for(map<string,PeerVal>::iterator it = m_peers.begin();it != m_peers.end();it++)
	{
		sprintf(buf,"%s		%s		%u\n",it->first.c_str(),it->second.isonline?"y":"n",it->second.share);
		tmp += buf;
		if (it->second.isonline)
			on++;
		else
			off++;
	}
	
	sprintf(buf," %d miners,%d online,%d offline\n",m_peers.size(),on,off);
	msg += buf;
	msg += tmp;

}


bool ProxyServer::dealClients()
{
	FD_SET fd;
	FD_ZERO(&fd);
	timeval sTimeout;
	sTimeout.tv_sec = 0;
	sTimeout.tv_usec = 250000;
	SOCKET max_s = 0;
	for (uint32 i = 0;i < m_clients.size();i++)
	{
		if (m_clients[i]>max_s)
		{
			max_s = m_clients[i];
		}
		FD_SET(m_clients[i],&fd);
	}

	int r = select(max_s+1, &fd, 0, 0, &sTimeout); 
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
		if (send(m_clients[i],(char*)&cmd,4,0) <= 0)
		{
			printf("ProxyServer::sendNewBlock send error\n");
		}
		//printf("ProxyServer::sendNewBlock cmd:%08X\n",cmd);
	}
}

void ProxyServer::sendCurrHour()
{
	if (m_clients.empty())
		return;

	time_t tNow = time(NULL);
	struct tm* ptm;
	ptm =localtime(&tNow);

	uint32 cmd = (ptm->tm_hour<<8)|CMD_TIME_HOUR;
	cmd = ntohl_ex(cmd);
	for (uint32 i = 0;i < m_clients.size();i++)
	{
		if (send(m_clients[i],(char*)&cmd,4,0) <= 0)
		{
			printf("ProxyServer::sendCurrHour send error\n");
		}
		//printf("ProxyServer::sendNewBlock cmd:%08X\n",cmd);
	}
}



THREAD_FUN ProxyServer::main()
{
	time_t tLast = time(NULL);
	while (1)
	{
		while (m_listen)
		{
			if (false ==dealListen())
				break;
		}

		WorkData *p=m_xptclient->getWorkData();
		if (p)
		{
			if (m_wd)
			{
				delete m_wd;
			}
			m_wd = p;
			//proxy client need to clear work queue
			if (m_wd)
			{
				sendNewBlock();
			}			
		}

		if (!m_clients.empty())
		{
			dealClients();
		}

		time_t tCurr = time(NULL);
		if (tCurr - tLast > 3)
		{
			sendCurrHour();
			tLast = tCurr;
		}

		Sleep(1);
	}

	return 0;
}