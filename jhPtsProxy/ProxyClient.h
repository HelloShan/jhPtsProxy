#ifndef __PROXY_CLIENT_H__
#define __PROXY_CLIENT_H__

#include "global.h"
#include "Thread.h"

class ProxyClient:public Thread
{
	string m_ip;
	uint16 m_port;

	SOCKET m_socket;
public:
	ProxyClient(){}
	virtual ~ProxyClient(){}

	bool init(string ip,uint16 port);

	virtual THREAD_FUN main();
private:
	void openConnection();
	void closeConnection();
};


#endif //__PROXY_CLIENT_H__
