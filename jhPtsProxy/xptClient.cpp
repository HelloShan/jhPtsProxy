#include "xptClient.h"

int XptClient::init(string ip,int port)
{
	m_ip = ip;
	m_port = port;
	m_socket = 0;
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
			//recv
		}
	}
}
