#include "ProxyClient.h"

bool ProxyClient::init(string ip,uint16 port)
{
	m_ip = ip;
	m_port = port;
	m_socket = 0;
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
	m_socket = 0;
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
				printf("connect xpt server failed,sleep 15 seconds\n");
				Sleep(15*1000);
			}
			continue;
		}
	}
	return 0;
}