// jhPtsProxy.cpp : 定义控制台应用程序的入口点。
//

#ifndef WIN32
#include "win.h"
#endif

#include "global.h"



int main(int argc, char** argv)
{
	// init winsock
#ifdef __WIN32__
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2),&wsa);
#endif
	

#ifdef __WIN32__
	WSACleanup( );
#endif
	return 0;
}

