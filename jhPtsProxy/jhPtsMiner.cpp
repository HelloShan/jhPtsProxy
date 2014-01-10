#include "ProxyClient.h"
#include "MinerThread.h"
#include "sha2_interface.h"

#define COLLISION_TABLE_BITS_8     21
#define COLLISION_TABLE_BITS_32    23
#define COLLISION_TABLE_BITS_128   25
#define COLLISION_TABLE_BITS_256   26
#define COLLISION_TABLE_BITS_512   27

int main(int argc, char** argv)
{
	// init winsock
#ifdef __WIN32__
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2),&wsa);
#endif

	SHA2_INTERFACE::getInstance(USE_DEFAULT_SHA512);

	uint32 tnum = 3;

	ProxyClient client;
	//client.init("127.0.0.1",10086,1);
	client.init("10.9.43.2",10086,3*tnum);
	client.start();
	
	for (uint32 i = 1;i<=tnum;i++)
	{
		MinerThread *pMiner = new MinerThread();
		if (!pMiner->init(COLLISION_TABLE_BITS_256,&client,i))
		{
			exit(0);
		}
		pMiner->start();
	}



	client.join();

#ifdef __WIN32__
	WSACleanup( );
#endif
	return 0;
}