#ifndef __XPT_CLIENT_H__
#define __XPT_CLIENT_H__

#include"global.h"
#include "Thread.h"
#include "ThreadQueue.h"



void inline dumpInt(uint32 &index,uint32 var,char *buf)
{
	uint32 tmp = ntohl_ex(var);
	memcpy(buf+index,&tmp,4);
	index += 4;
}

void inline dumpStr(uint32 &index,string &str,char *buf)
{
	//string must less then 127
	uint8 tmp = (uint8)str.length();
	memcpy(buf+index,&tmp,1);
	index++;
	memcpy(buf+index,str.c_str(),str.length());
	index += (uint32)str.length();
}


void inline getInt32(uint32 &index,uint32& var,const char *buf)
{
	var = ntohl_ex(*(uint32*)(buf+index));
	index += 4;
}

void inline getInt16(uint32 &index,uint16& var,const char *buf)
{
	var = ntohs_ex(*(uint16*)(buf+index));
	index += 2;
}

void inline getInt8(uint32 &index,uint8& var,const char *buf)
{
	var = *(uint8*)(buf+index);
	index++;
}

void inline getStr(uint32 &index,string &str,const char *buf)
{
	str = "";
	uint16 len = *(uint16*)(buf+index);
	index += 2;
	if (len>0)
	{
		char *pTmp = new char[len+1];
		memcpy(pTmp,buf+index,len);
		pTmp[len] = 0;
		str = pTmp;
		delete pTmp;
	}
	index += len;
}

void inline getData(uint32 &index,uint8 *dst,uint32 len,const char *buf)
{
	if(!len) return;
	memcpy(dst,buf+index,len);
	index += len;
}

struct WorkerInfo
{
	uint32 pkglen;
	uint32 unkn;
	string user;
	string pass;
	uint32 pln;
	string version;

	//for to send socket
	char buf[1024];
	uint32 length;

	WorkerInfo(string &u)
	{
		unkn = 5;
		user = u;//"PsADEvP6kBevQD2fkRUZxmvuNJZe5u7W8k";
		pass = "x";
		pln = 1;
		version = "jhProtominer v0.1c-Linux";

		//dump for send
		//all string must less than 127
		//all string have more one byte for record the length
		uint32 dsize=11+(uint32)user.length()+pass.length()+version.length();
		pkglen=(dsize<<8)|XPT_OPC_C_AUTH_REQ;
		length = dsize+4;

		uint32 index = 0;
		dumpInt(index,pkglen,buf);
		dumpInt(index,unkn,buf);
		dumpStr(index,user,buf);
		dumpStr(index,pass,buf);
		dumpInt(index,pln,buf);
		dumpStr(index,version,buf);
	}
};


class XptClient:public Thread
{
	string m_ip;
	uint16 m_port;

	WorkerInfo *m_pWoker;

	WorkData *m_pWD;
	CRITICAL_SECTION m_wdcs;

	SOCKET m_socket;

	ThreadQueue<SubmitInfo*> m_queue;

	//queue<
	XptClient(){};
	virtual ~XptClient(){};
	static XptClient *instance;
public:

	static XptClient *getInstance()
	{
		if (!instance)
		{
			instance = new XptClient();
		}
		return instance;
	}

	int init(string ip,uint16 port,string &u);

	virtual	THREAD_FUN  main();

	WorkData *getWorkData();
	bool CheakNewWork();
	void pushSubmit(SubmitInfo *p);

	static volatile uint32 valid_share;
	static volatile uint32 invalid_share;
	static string last_reason;
private:
	void openConnection();
	void closeConnection();
	void sendWorkerLogin();
	void processSubmit();
	void processReceive();

	void dealAuthACK(uint32 len,const char *pbuf);
	void dealWorkData(uint32 len,const char *pbuf);
	void dealShareACK(uint32 len,const char *pbuf);
	void dealMsg(uint32 len,const char *pbuf);

	bool recvData(uint32 len,char *buf,SOCKET s);
};

#endif //__XPT_CLIENT_H__