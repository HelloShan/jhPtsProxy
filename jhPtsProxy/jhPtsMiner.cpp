#include "ProxyClient.h"

int main(int argc, char** argv)
{
	// init winsock
#ifdef __WIN32__
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2),&wsa);
#endif

	ProxyClient client;
	client.init("127.0.0.1",10086,1);
	client.start();

	client.join();

#ifdef __WIN32__
	WSACleanup( );
#endif
	return 0;
}