#ifndef __XPT_CLIENT_H__
#define __XPT_CLIENT_H__

#include"global.h"
#include "Thread.h"


// list of known opcodes
#define XPT_OPC_C_AUTH_REQ		1
#define XPT_OPC_S_AUTH_ACK		2
#define XPT_OPC_S_WORKDATA1		3
#define XPT_OPC_C_SUBMIT_SHARE	4
#define XPT_OPC_S_SHARE_ACK		5
#define XPT_OPC_C_SUBMIT_POW	6
#define XPT_OPC_S_MESSAGE		7

// list of error codes
#define XPT_ERROR_NONE				(0)
#define XPT_ERROR_INVALID_LOGIN		(1)
#define XPT_ERROR_INVALID_WORKLOAD	(2)
#define XPT_ERROR_INVALID_COINTYPE	(3)

void inline dumpInt(uint32 &index,uint32 var,char *buf)
{
	uint32 tmp = ntohl_ex(var);
	memcpy(buf+index,&tmp,4);
	index += 4;
}

void inline dumpStr(uint32 &index,string &str,char *buf)
{
	uint8 tmp = str.length();
	memcpy(buf+index,&tmp,1);
	index++;
	memcpy(buf+index,str.c_str(),str.length());
	index += str.length();
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
	uint8 len = *(uint8*)(buf+index);
	index++;
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

	WorkerInfo()
	{
		unkn = 5;
		user = "PsADEvP6kBevQD2fkRUZxmvuNJZe5u7W8k";
		pass = "x";
		pln = 1;
		version = "jhProtominer v0.1c-Linux";

		//dump for send
		//all string must less than 127
		//all string have more one byte for record the length
		uint32 dsize=11+user.length()+pass.length()+version.length();
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
	int m_port;

	WorkerInfo *m_pWoker;

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
	void processSubmit();
	void processReceive();

	void dealAuthACK(uint32 len,const char *pbuf);
};

#endif //__XPT_CLIENT_H__