#include "ProxyClient.h"

ProxyClient *ProxyClient::instance=0;
volatile uint32 ProxyClient::cur_height=0;
volatile uint32 ProxyClient::cur_hour=0;

bool ProxyClient::init(string ip,uint16 port,uint32 threadnum)
{
	m_ip = ip;
	m_port = port;
	m_socket = 0;
	m_threadnum = threadnum;
	m_havereq = false;
	return true;
}

void ProxyClient::openConnection()
{
	SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(SOCKADDR_IN));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(m_port);
	addr.sin_addr.s_addr=inet_addr(m_ip.c_str());
	printf("connect to proxy server:%s:%d,addr.sin_port=%d\n", m_ip.c_str(), m_port,addr.sin_port);
	int result = connect(s,(SOCKADDR*)&addr,sizeof(SOCKADDR_IN));
	if( result )
	{
		m_socket = 0;
		return;
	}
	m_socket = s;

#ifdef __WIN32__
	unsigned int nonblocking=1;
	unsigned int cbRet;
#endif
	WSAIoctl(m_socket, FIONBIO, &nonblocking, sizeof(nonblocking), NULL, 0, (LPDWORD)&cbRet, NULL, NULL);
}

void ProxyClient::closeConnection()
{
	if (m_socket)
	{
		closesocket(m_socket);
	}
	printf("close proxy client\n");
	m_socket = 0;
	m_block.clear();
}

BlockInfo *ProxyClient::getBlockInfo()
{
	BlockInfo *ret = NULL;
	m_block.pop(ret);
	return ret;
}

bool ProxyClient::sendShare()
{
	uint32 cmd=ntohl_ex(CMD_SMT_SHARE);
	while(1) 
	{
		SubmitInfo *pShare = NULL;
		m_submit.pop(pShare);
		if(NULL == pShare)
		{
			break;
		}
		send(m_socket,(char*)&cmd,4,0);
		send(m_socket,(char*)pShare,sizeof(SubmitInfo),0);
		delete pShare;
	}
	return true;
}

bool ProxyClient::haveEnoughBlock()
{
	return !m_block.empty();
}


bool ProxyClient::recvData(uint32 len,char *buf)
{
	uint32 index = 0;
	int ret = 0;
	while(index<len)
	{
		ret = recv(m_socket,buf+index,len-index,0);
		if (0 == ret /* || SOCKET_ERROR == ret*/)
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

bool ProxyClient::recvBlockInfo(uint32 n)
{
	for(uint32 i = 0;i < n;i++)
	{
		BlockInfo *pBlock = new BlockInfo();
		if (!pBlock)
		{
			printf("ProxyClient::recvBlockInfo memory error\n");
			return false;
		}
		if (!recvData(sizeof(BlockInfo),(char*)pBlock))
		{
			delete pBlock;
			return false;
		}
		//printf("recv a new block,height:%d\n",ntohl_ex(pBlock->height));
		if (i == 0)
		{
			cur_height = pBlock->height;
		}		
		m_block.push(pBlock);
	}
	m_havereq = false;
	return true;
}

void ProxyClient::clearBlockInfo()
{
	m_block.clear();
}

bool ProxyClient::dealRecv()
{
	FD_SET fd;
	FD_ZERO(&fd);
	timeval sTimeout;
	sTimeout.tv_sec = 0;
	sTimeout.tv_usec = 250000;
	FD_SET(m_socket,&fd);

	int r = select(m_socket+1, &fd, 0, 0, &sTimeout); 
	if (r<=0)
	{
		//printf("ProxyClient::dealRecv have nothing,ret %d\n",r);
		return true;
	}

	uint32 cmd = 0;
	if (!recvData(4,(char*)&cmd))
	{
		printf("ProxyClient::dealRecv recv cmd error\n");
		return false;
	}
	cmd = ntohl_ex(cmd);
	switch(cmd&0xFF)
	{
	case CMD_ACK_BLOCK:
		//printf("ProxyClient::dealRecv recv CMD_ACK_BLOCK cmd:%08X\n",cmd);
		return recvBlockInfo(cmd>>8);
	case CMD_NEW_BLOCK:
		//printf("ProxyClient::dealRecv recv CMD_NEW_BLOCK cmd:%08X\n",cmd);
		clearBlockInfo();
		break;
	case CMD_TIME_HOUR:
		cur_hour = cmd>>8;
		break;
	default:
		printf("ProxyClient::dealRecv recv unk cmd error,%08X\n",cmd);
		return false;
	}
	return true;
}


bool ProxyClient::requestBlock()
{
	if (m_block.size()<m_threadnum)
	{
		uint32 cmd=(m_threadnum<<8)|CMD_REQ_BLOCK;
		cmd = ntohl_ex(cmd);
		send(m_socket,(char*)&cmd,4,0);
		//printf("ProxyClient::requestBlock send CMD_REQ_BLOCK cmd,%08X\n",cmd);
	}
	return true;
}

void ProxyClient::pushShare(SubmitInfo *p)
{
	if (!p) return;
	m_submit.push(p);
}

THREAD_FUN ProxyClient::main()
{
	time_t last = time(NULL);
	while (1)
	{
		if(!m_socket)
		{
			openConnection();
			if(!m_socket)
			{
				printf("connect proxy server failed,wait 15 seconds\n");
				Sleep(15*1000);
			}
			continue;
		}
		if(!sendShare())
		{
			closeConnection();
			continue;
		}

		if(!dealRecv())
		{
			closeConnection();
			continue;
		}

		if (!m_havereq)
		{
			if(!requestBlock())
			{
				closeConnection();
				continue;
			}
			else
			{
				m_havereq = true;
				last = time(NULL);
			}
		}
		else
		{
			if (time(NULL)-last>5)
			{
				m_havereq = false;
				last = time(NULL);
			}
		}

		Sleep(10);
		
	}
	return 0;
}