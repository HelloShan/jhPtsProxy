#include "ProxyClient.h"

bool ProxyClient::init(string ip,uint16 port,uint32 threadnum)
{
	m_ip = ip;
	m_port = port;
	m_socket = 0;
	m_threadnum = threadnum;
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
	SubmitInfo *pShare = NULL;
	uint32 cmd=ntohl_ex(CMD_SMT_SHARE);
	while(1) 
	{
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


bool ProxyClient::recvData(uint32 len,char *buf)
{
	uint32 index = 0;
	int ret = 0;
	while(index<len)
	{
		ret = recv(m_socket,buf+index,len-index,0);
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
		m_block.push(pBlock);
	}
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

	int r = select(0, &fd, 0, 0, &sTimeout); 
	if (r<=0)
	{
		//printf("ProxyClient::dealRecv have nothing\n");
		return true;
	}

	uint32 cmd = 0;
	if (!recvData(4,(char*)&cmd))
	{
		return false;
	}
	cmd = ntohl_ex(cmd);
	switch(cmd&0xFF)
	{
	case CMD_ACK_BLOCK:
		return recvBlockInfo(cmd>>8);
	case CMD_NEW_BLOCK:
		clearBlockInfo();
		break;
	default:
		printf("ProxyClient::dealRecv unk cmd error,%d\n",cmd&0xFF);
		return false;
	}
	return true;
}


bool ProxyClient::requestBlock()
{
	if (m_block.empty())
	{
		uint32 cmd=(m_threadnum<<8)|CMD_REQ_BLOCK;
		cmd = ntohl_ex(cmd);
		send(m_socket,(char*)&cmd,4,0);
	}
	return true;
}

THREAD_FUN ProxyClient::main()
{
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

		if(!requestBlock())
		{
			closeConnection();
			continue;
		}

		Sleep(5);
	}
	return 0;
}