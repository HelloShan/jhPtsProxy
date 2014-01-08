#ifndef __PROXY_SERVER_H__
#define __PROXY_SERVER_H__

#include "global.h"
#include "Thread.h"
#include "ThreadQueue.h"

#define CMD_REQ_BLOCK 1
#define CMD_ACK_BLOCK 2
#define CMD_SMT_SHARE 3
#define CMD_ACK_SHARE 4
#define CMD_NEW_BLOCK 5

class XptClient;
class ProxyServer:public Thread
{
	vector<SOCKET> m_clients;

	SOCKET m_listen;
	uint16 m_port;
	XptClient *m_xptclient;
	WorkData *m_wd;
public:
	ProxyServer(){};
	virtual ~ProxyServer(){};
	
	int init(uint16 port,XptClient *xptclient);
	
	virtual	THREAD_FUN  main();

private:
	bool dealListen();
	bool dealClients();
	void sendNewBlock();

	bool recvCmd(uint32 *p,SOCKET s);
	bool sendBlock(SOCKET s,uint32 n);
	bool recvShare(SOCKET s);

	void closeClient(SOCKET s);

};

#endif

