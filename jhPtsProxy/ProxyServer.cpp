#include "ProxyServer.h"
#include "xptClient.h"

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

bool ProxyServer::recvOpt(uint8 *opt,SOCKET s)
{
	uint32 index = 0;
	uint32 len = 1;
	int ret = 0;
	uint8 opt = 0;
	while(index<len)
	{
		ret = recv(s,((char*)&opt)+index,len-index,0);
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

bool ProxyServer::sendBlock(SOCKET s)
{

}

bool ProxyServer::dealClients()
{
	FD_SET fd;
	timeval sTimeout;
	sTimeout.tv_sec = 0;
	sTimeout.tv_usec = 250000;
	for (int i = 0;i < m_clients.size();i++)
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
			uint8 opt = 0;
			if (!recvOpt(&opt,*it))
			{
				closesocket(*it);
				m_clients.erase(it++);
			}
			else
			{
				switch(opt)
				{
				case CMD_REQ_BLOCK:
					sendBlock(*it);
					break;
				case CMD_SMT_SHARE:
					break;
				default:
					printf("ProxyServer::dealClients unk opt \n");
					break;
				}
				++it;
			}
		}
	}

}

THREAD_FUN ProxyServer::main()
{
	while (1)
	{
		if (!m_clients.empty())
		{
			dealClients();
		}
		
		while (m_listen)
		{
			if (false ==dealListen())
				break;
		}
	}
}