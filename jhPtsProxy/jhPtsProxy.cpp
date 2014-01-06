// jhPtsProxy.cpp : 定义控制台应用程序的入口点。
//

#ifndef __WIN32__
#include "win.h"
#endif

#include "xptClient.h"



int main(int argc, char** argv)
{
	// init winsock
#ifdef __WIN32__
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2),&wsa);
#endif
	
	XptClient client;
	if(client.init("127.0.0.1", 28999))
	{
		printf("cant not connect to xpt server\n");
		return 0;
	}
	client.start();

	//ProxyServer *pServer=new ProxyServer(2899);

	client.join();
#ifdef __WIN32__
	WSACleanup( );
#endif
	return 0;
}

