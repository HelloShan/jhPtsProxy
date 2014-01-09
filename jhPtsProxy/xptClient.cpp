#include "xptClient.h"


int XptClient::init(string ip,uint16 port)
{
	m_ip = ip;
	m_port = port;
	m_socket = 0;
	m_pWoker = new WorkerInfo();
	m_pWD = NULL;
	InitializeCriticalSection(&m_wdcs);
	return 0;
}

void XptClient::openConnection()
{
	SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(SOCKADDR_IN));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(m_port);
	addr.sin_addr.s_addr=inet_addr(m_ip.c_str());
	printf("connect to server:%s:%d,addr.sin_port=%d\n", m_ip.c_str(), m_port,addr.sin_port);
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

void XptClient::closeConnection()
{
	if (m_socket)
	{
		closesocket(m_socket);
	}
	m_socket = 0;
}

void XptClient::sendWorkerLogin()
{
	if (send(m_socket,m_pWoker->buf,m_pWoker->length,0) != m_pWoker->length)
	{
		printf("XptClient::sendWorkerLogin error\n");
		closeConnection();
	}
}

void XptClient::processSubmit()
{
	while (!m_queue.empty())
	{
		SubmitInfo *info=NULL;
		m_queue.pop(info);
		if (send(m_socket,(char*)info,sizeof(SubmitInfo), 0) != sizeof(SubmitInfo))
		{
			printf("XptClient::processSubmit error\n");
			closeConnection();
			delete info;
			return;
		}
		delete info;
	}
}
bool XptClient::recvData(uint32 len,char *buf,SOCKET s)
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

void XptClient::processReceive()
{
	fd_set r;
	struct timeval timeout;
	int ret;

	FD_ZERO(&r);
	FD_SET(m_socket, &r);
	timeout.tv_sec = 1;
	timeout.tv_usec =0;
	ret = select((int)m_socket+1, &r, 0, 0, &timeout);
	if (ret<=0)
	{
		//printf("XptClient::processReceive have nothing now\n");
		return;
	}

	uint32 head = 0;
	uint32 len = 0;
	if (!recvData(4,(char*)&head,m_socket))
	{
		printf("XptClient::processReceive recv head error,will disconnect\n");
		closeConnection();
		return;
	}

	head = ntohl_ex(head);

	len = head>>8;
	uint8 opt = head;
	char *pbuf = new char[len];
	if (!pbuf)
	{
		printf("XptClient::processReceive memory error\n");
		closeConnection();
		return;
	}

	if (!recvData(len,pbuf,m_socket))
	{
		printf("XptClient::processReceive recv buff error,will disconnect\n");
		closeConnection();
		delete[] pbuf;
		return;
	}
	
	switch(opt)
	{
	case XPT_OPC_S_AUTH_ACK:
		dealAuthACK(len,pbuf);
		break;
	case XPT_OPC_S_WORKDATA1:
		dealWorkData(len,pbuf);
		break;
	case XPT_OPC_S_SHARE_ACK:
		dealShareACK(len,pbuf);
		break;
	case XPT_OPC_S_MESSAGE:
		dealMsg(len,pbuf);
		break;
	default:
		printf("XptClient::processReceive receive unk opt:%d\n",opt);
		break;
	}

	delete[] pbuf;
}

void XptClient::dealAuthACK(uint32 len,const char *pbuf)
{
	uint32 index = 0;
	uint32 authCode = -1;
	string reason;
	getInt32(index,authCode,pbuf);
	getStr(index,reason,pbuf);
	if (index>len)
	{
		printf("XptClient::dealAuthACK error\n");
		closeConnection();
		return;
	}
	
	if (authCode != 0)
		printf("login with %s failedreason %s\n",m_pWoker->user.c_str(),reason.c_str());
	else
		printf("login with %s success\n",m_pWoker->user.c_str());
}

void XptClient::dealWorkData(uint32 len,const char *pbuf)
{
	WorkData wd;
	uint32 index = 0;
	getInt32(index,wd.block.version,pbuf);
	getInt32(index,wd.block.height,pbuf);
	getInt32(index,wd.block.nBits,pbuf);
	getData(index,wd.block.target,32,pbuf);
	getData(index,wd.block.targetShare,32,pbuf);
	getInt32(index,wd.block.nTime,pbuf);
	getData(index,wd.block.prevBlockHash,32,pbuf);
	getData(index,wd.block.merkleRoot,32,pbuf);
	getInt16(index,wd.coinBase1Size,pbuf);
	if (wd.coinBase1Size > 512)
	{
		printf("XptClient::dealWorkData wd.coinBase1Size error\n");
		closeConnection();
		return;
	}
	getData(index,wd.coinBase1,wd.coinBase1Size,pbuf);
	getInt16(index,wd.coinBase2Size,pbuf);
	if (wd.coinBase2Size > 512)
	{
		printf("XptClient::dealWorkData wd.coinBase2Size error\n");
		closeConnection();
		return;
	}
	getData(index,wd.coinBase2,wd.coinBase2Size,pbuf);
	getInt16(index,wd.txHashCount,pbuf);
	if (wd.txHashCount > 16)
	{
		printf("XptClient::dealWorkData wd.txHashCount error\n");
		closeConnection();
		return;
	}
	if (wd.txHashCount != 0)
	{
		printf("assert wd.txHashCount is %d\n",wd.txHashCount);
	}
	getData(index,wd.txHashes,wd.txHashCount*32,pbuf);

	if (index>len)
	{
		printf("XptClient::dealWorkData error\n");
		closeConnection();
		return;
	}

	EnterCriticalSection(&m_wdcs);
	if (m_pWD)
	{
		delete m_pWD;
		m_pWD = NULL;
	}
	m_pWD = new WorkData();
	if (m_pWD)
	{
		memcpy((char*)m_pWD,(char*)&wd,sizeof(WorkData));
	}
	LeaveCriticalSection(&m_wdcs);
}

WorkData *XptClient::getWorkData()
{
	EnterCriticalSection(&m_wdcs);
	WorkData *p = m_pWD;
	m_pWD = NULL;
	LeaveCriticalSection(&m_wdcs);
	return p;
}

void XptClient::pushSubmit(SubmitInfo *p)
{
	if (!p) return;
	m_queue.push(p);
}

void XptClient::dealShareACK(uint32 len,const char *pbuf)
{
	uint32 index = 0;
	uint32 shareErrorCode = -1;
	string reason;
	getInt32(index,shareErrorCode,pbuf);
	getStr(index,reason,pbuf);
	if (index>len)
	{
		printf("XptClient::dealShareACK error\n");
		closeConnection();
		return;
	}
	
	if (shareErrorCode != 0)
		printf("Invalid share,reason %s\n",reason.c_str());
}

void XptClient::dealMsg(uint32 len,const char *pbuf)
{
	uint32 index = 0;
	string msg;
	getStr(index,msg,pbuf);
	if (index>len)
	{
		printf("XptClient::dealMsg error\n");
		closeConnection();
		return;
	}
	printf("xpt Server message:%s\n",msg.c_str());
}


THREAD_FUN XptClient::main()
{
	while(1)
	{
		if (!m_socket)
		{
			openConnection();
			if (!m_socket)
			{
				printf("connect xpt server failed,sleep 15 seconds\n");
				Sleep(15*1000);
			}
			sendWorkerLogin();
			continue;
		}
		
		if (m_socket)
		{
			processSubmit();
		}

		if (m_socket)
		{
			processReceive();
		}
	}
}
