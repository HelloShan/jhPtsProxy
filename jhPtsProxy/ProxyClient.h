#ifndef __PROXY_CLIENT_H__
#define __PROXY_CLIENT_H__

#include "global.h"
#include "Thread.h"
#include "ThreadQueue.h"

class ProxyClient:public Thread
{
	string m_ip;
	uint16 m_port;
	uint32 m_threadnum;

	SOCKET m_socket;

	ThreadQueue<SubmitInfo*> m_submit;
	ThreadQueue<BlockInfo*> m_block;

public:
	ProxyClient(){}
	virtual ~ProxyClient(){}

	bool init(string ip,uint16 port,uint32 threadnum);

	virtual THREAD_FUN main();
	BlockInfo *getBlockInfo();
private:
	void openConnection();
	void closeConnection();

	bool sendShare();
	bool dealRecv();
	bool requestBlock();

	bool recvData(uint32 len,char *buf);
	bool recvBlockInfo(uint32 n);
	void clearBlockInfo();
	
};


#endif //__PROXY_CLIENT_H__
