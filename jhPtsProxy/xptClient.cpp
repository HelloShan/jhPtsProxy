#include "xptClient.h"

int XptClient::init(string ip,int port)
{
	m_ip = ip;
	m_port = port;
	m_socket = 0;
	m_pWoker = new WorkerInfo();
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
		printf("XptClient::processReceive have nothing now\n");
		return;
	}

	uint32 head = 0;
	if(recv(m_socket,(char*)&head,4,0) != 4)
	{
		printf("XptClient::processReceive recv len error\n");
		closeConnection();
		return;
	}
	head = ntohl_ex(head);

	uint32 len = head>>8;
	uint8 opt = head;
	char *pbuf = new char[len];
	if (!pbuf)
	{
		printf("XptClient::processReceive memory error\n");
		closeConnection();
		return;
	}

	uint32 index=0;
	while(index<len)
	{
		index += recv(m_socket,pbuf+index,len-index,0);
	}
	
	switch(opt)
	{
	case XPT_OPC_S_AUTH_ACK:
		dealAuthACK(len,pbuf);
		break;
	case XPT_OPC_S_WORKDATA1:
		printf("XptClient::processReceive receive woke data opt\n");
		break;
	case XPT_OPC_S_SHARE_ACK:
		printf("XptClient::processReceive receive share ack opt\n");
		break;
	case XPT_OPC_S_MESSAGE:
		printf("XptClient::processReceive receive msg opt\n");
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
		printf("dealAuthACK error\n");
		closeConnection();
		return;
	}

	printf("login with %s,authCode %u,reason %s\n",m_pWoker->user.c_str(),authCode,reason.c_str());
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
