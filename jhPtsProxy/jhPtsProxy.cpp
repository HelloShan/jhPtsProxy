// jhPtsProxy.cpp : 定义控制台应用程序的入口点。
//

#ifndef __WIN32__
#include "win.h"
#endif

#include "xptClient.h"
#include "ProxyServer.h"
#include <stdio.h>

struct commandLineSvr
{
	string ip;
	uint16 port;
	uint16 proxyport;
	string user;

	commandLineSvr()
	{
		ip = "112.124.13.238";
		port = 28988;
		proxyport = 10086;
		user = "PsADEvP6kBevQD2fkRUZxmvuNJZe5u7W8k.gshesh";
	}

};


void printHelp()
{
	puts("Usage: jhPtsProxy[.exe] [options]");
	puts("Options:");
	puts("   -s                          The proxyserver will connect to this xpt server");
	puts("   -p                          The proxyserver will connect to this xpt port");
	puts("   -u                          The proxyserver will use this account address");
	puts("   -pport                      The proxyserver will listen this proxy port");
	puts("   jhPtsProxy -s 112.124.13.238 -p 28988 -pport 10086 -u xxxx");
}

void parseCommandline(int argc, char **argv,commandLineSvr &cmdline)
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
		if( memcmp(argument, "-u", 3)==0)
		{
			// -o
			if( cIdx >= argc )
			{
				printf("Missing ip after -o option\n");
				exit(0);
			}
			cmdline.user = argv[cIdx];

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
		else if( memcmp(argument, "-pport", 7)==0 )
		{
			// -p
			if( cIdx >= argc )
			{
				printf("Missing thread num after -t option\n");
				exit(0);
			}
			cmdline.proxyport = atoi(argv[cIdx]);
			cIdx++;
		}
		else if( memcmp(argument, "-help", 6)==0 || memcmp(argument, "--help", 7)==0 )
		{
			printHelp();
			exit(0);
		}
		else
		{
			printf("'%s' is an unknown option.\nType jhPtsProxy[.exe] --help for more info\n", argument); 
			exit(-1);
		}
	}
	if( argc <= 1 )
	{
		//printHelp();
		//exit(0);
		printf("use default setting\n");
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

	commandLineSvr cmdline;
	parseCommandline(argc,argv,cmdline);
	
	XptClient* pClient = XptClient::getInstance();
	//if(client.init("127.0.0.1", 28999))
	if(0 != pClient->init(cmdline.ip, cmdline.port,cmdline.user))
	{
		printf("cant not connect to xpt server\n");
		return 0;
	}
	pClient->start();

	//ProxyServer *pServer=new ProxyServer(2899);

	ProxyServer *pServer = ProxyServer::getInstance();
	if(0 != pServer->init(cmdline.proxyport))
	{
		exit(0);
	}
	pServer->start();

	char buf[128] = {0};
	time_t tStart = time(NULL);
	time_t tLast = tStart;
	while(1)
	{
		time_t tCurr = time(NULL);
		if (tCurr - tLast > 60)
		{
			sprintf(buf,"[all] valid share %u,invalid share %d,rate per min %.2f,last reason %s\n",\
				XptClient::valid_share,XptClient::invalid_share,\
				(double)XptClient::valid_share/((double)(tCurr-tStart)/60),\
				XptClient::last_reason.c_str());

			string msg;
			pServer->getMinersState(msg);
			msg += "\n\n";
			msg += buf;
			
			FILE *fp = NULL;
			fp = fopen("miners.txt", "w");
			if (NULL == fp) continue;
			fwrite(msg.c_str(),msg.length(),1,fp);
			fclose(fp);

			tLast = tCurr;
		}
		Sleep(60*1000);
	}

	pClient->join();
	pServer->join();
#ifdef __WIN32__
	WSACleanup( );
#endif
	return 0;
}

