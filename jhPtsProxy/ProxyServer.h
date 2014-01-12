#ifndef __PROXY_SERVER_H__
#define __PROXY_SERVER_H__

#include "global.h"
#include "Thread.h"
#include "ThreadQueue.h"
#include "sha2_interface.h"


class XptClient;
class ProxyServer:public Thread
{
	vector<SOCKET> m_clients;

	SOCKET m_listen;
	uint16 m_port;
	XptClient *m_xptclient;
	WorkData *m_wd;
	SHA2_FUNC sha256_func;
	ProxyServer(){};
	virtual ~ProxyServer(){};
	static ProxyServer *instance;
public:
	static ProxyServer *getInstance()
	{
		if (!instance)
		{
			instance = new ProxyServer();
		}
		return instance;
	}

	int init(uint16 port);
	
	virtual	THREAD_FUN  main();

private:
	bool dealListen();
	bool dealClients();
	void sendNewBlock();

	bool recvCmd(uint32 *p,SOCKET s);
	bool sendBlock(SOCKET s,uint32 n);
	bool recvShare(SOCKET s);

	void closeClient(SOCKET s);
	bool recvData(uint32 len,char *buf,SOCKET s);

};

#endif

