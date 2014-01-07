#ifndef __PROXY_SERVER_H__
#define __PROXY_SERVER_H__

#include "global.h"
#include "ThreadQueue.h"

#define CMD_REQ_BLOCK 1
#define CMD_ACK_BLOCK 2
#define CMD_SMT_SHARE 3
#define CMD_ACK_SHARE 4

class XptClient;
class ProxyServer:public Thread
{
	vector<SOCKET> m_clients;

	SOCKET m_listen;
	int m_port;
	XptClient *m_xptclient;
public:
	ProxyServer(){};
	~ProxyServer(){};
	
	int init(int port,XptClient *xptclient);
	
	virtual	THREAD_FUN  main();
};

#endif

