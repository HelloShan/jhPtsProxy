#include "xptClient.h"


int XptClient::init(string ip,int port)
{
	m_ip = ip;
	m_port = port;
	m_socket = 0;
	m_pWoker = new WorkerInfo();
	m_pWD = new WorkData();
	InitializeCriticalSection(&m_wdcs);
	m_newwd = false;
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

void XptClient::processReceive()
{
	fd_set r;
	struct timeval timeout;
	int ret;

	FD_ZERO(&r);
	FD_SET(m_socket, &r);
	timeout.tv_sec = 1;
	timeout.tv_usec =0;
	ret = select(m_socket+1, &r, 0, 0, &timeout);
	if (ret<=0)
	{
		//printf("XptClient::processReceive have nothing now\n");
		return;
	}

	uint32 head = 0;
	uint32 index = 0;
	uint32 len = 4;
	while(index<len)
	{
		ret = recv(m_socket,((char*)&head)+index,len-index,0);
		if (ret <= 0)
		{
			if( WSAGetLastError() != WSAEWOULDBLOCK )
			{
				printf("XptClient::processReceive recv head error,will disconnect\n");
				closeConnection();
				return;
			}
		}
		index += ret;
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

	index = 0;
	while(index<len)
	{
		ret = recv(m_socket,pbuf+index,len-index,0);
		if (ret <= 0)
		{
			if( WSAGetLastError() != WSAEWOULDBLOCK )
			{
				printf("XptClient::processReceive redv buff error,will disconnect\n");
				closeConnection();
				delete[] pbuf;
				return;
			}
		}
		index += ret;
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
	getData(index,wd.txHashes,wd.txHashCount*32,pbuf);

	if (index>len)
	{
		printf("XptClient::dealWorkData error\n");
		closeConnection();
		return;
	}

	EnterCriticalSection(&m_wdcs);
	memcpy(m_pWD,&wd,sizeof(WorkData));
	m_newwd = true;
	LeaveCriticalSection(&m_wdcs);
}

WorkData *XptClient::getWorkData()
{
	if (m_newwd)
	{
		WorkData *p_wd = new WorkData();
		EnterCriticalSection(&m_wdcs);
		memcpy(m_pWD,p_wd,sizeof(WorkData));
		m_newwd = false;
		LeaveCriticalSection(&m_wdcs);
		return p_wd;
	}
	return NULL;
}

bool XptClient::CheakNewWork()
{
	return m_newwd;
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
