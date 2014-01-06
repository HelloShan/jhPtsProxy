#ifndef __XPT_CLIENT_H__
#define __XPT_CLIENT_H__

#include"global.h"
#include "Thread.h"

class XptClient:public Thread
{
	string m_ip;
	int m_port;

	SOCKET m_socket;
	//queue<
public:
	XptClient(){};
	~XptClient(){};

	int init(string ip,int port);

	virtual	THREAD_FUN  main();

private:
	void openConnection();
	void closeConnection();
	void sendWorkerLogin();
};

#endif //__XPT_CLIENT_H__