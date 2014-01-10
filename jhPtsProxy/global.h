#ifndef __GLOBLE_H__
#define __GLOBLE_H__


#ifdef __WIN32__
#pragma comment(lib,"Ws2_32.lib")
#include<Winsock2.h>
#include<ws2tcpip.h>
#else
#include"win.h" // port from windows
#endif

#include<stdio.h>
#include<time.h>
#include<stdlib.h>

#include <string>
#include <queue>
using namespace std;


#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
#define CHAR uint8
#define WORD uint16
#define UINT uint32
#define UINT64 uint64
#define LL(value) (value ## ll)

#ifdef USE_SEX_NTOH_EX
#define 		ntohs_ex(value) \
	(((((WORD)((WORD)(value) & 0x00ff)) << 8) \
	| (((WORD)((WORD)(value) & 0xff00)) >> 8)))

#define 		ntohl_ex(value) \
	(((((UINT)((UINT)(value) & 0x000000ff)) << 24) \
	| (((UINT)((UINT)(value) & 0x0000ff00)) << 8) \
	| (((UINT)((UINT)(value) & 0x00ff0000)) >> 8) \
	| (((UINT)((UINT)(value) & 0xff000000)) >> 24)))

#define 		ntohll_ex(value) \
	(((((UINT64)((UINT64)(value) & LL(0x00000000000000ff))) << 56) \
	| (((UINT64)((UINT64)(value) & LL(0x000000000000ff00))) << 40) \
	| (((UINT64)((UINT64)(value) & LL(0x0000000000ff0000))) << 24) \
	| (((UINT64)((UINT64)(value) & LL(0x00000000ff000000))) << 8)  \
	| (((UINT64)((UINT64)(value) & LL(0xff00000000000000))) >> 56) \
	| (((UINT64)((UINT64)(value) & LL(0x00ff000000000000))) >> 40) \
	| (((UINT64)((UINT64)(value) & LL(0x0000ff0000000000))) >> 24) \
	| (((UINT64)((UINT64)(value) & LL(0x000000ff00000000))) >> 8)))

#else
typedef union{
	WORD f;
	CHAR c[2];
}WORD_CONV;

static inline WORD ntohs_ex(WORD data)
{
	WORD_CONV d1, d2;

	d1.f = data;

	d2.c[0] = d1.c[1];
	d2.c[1] = d1.c[0];

	return d2.f;
}

typedef union{
	UINT f;
	CHAR c[4];
}UINT_CONV;

static inline UINT ntohl_ex(UINT data)
{
	UINT_CONV d1, d2;

	d1.f = data;

	d2.c[0] = d1.c[3];
	d2.c[1] = d1.c[2];
	d2.c[2] = d1.c[1];
	d2.c[3] = d1.c[0];


	return d2.f;
}

typedef union{
	UINT64 f;
	CHAR c[8];
}UINT64_CONV;

static inline UINT64 ntohll_ex(UINT64 data)
{
	UINT64_CONV d1, d2;

	d1.f = data;

	d2.c[0] = d1.c[7];
	d2.c[1] = d1.c[6];
	d2.c[2] = d1.c[5];
	d2.c[3] = d1.c[4];
	d2.c[4] = d1.c[3];
	d2.c[5] = d1.c[2];
	d2.c[6] = d1.c[1];
	d2.c[7] = d1.c[0];


	return d2.f;
}
#endif //USE_SEX_NTOH_EX

#else
#define     ntohl_ex(value)  (value)
#define     ntohs_ex(value)  (value)
#define     ntohll_ex(value) (value)  
#endif //__BIG_ENDIAN__


typedef unsigned char 		uint8;
typedef unsigned short		uint16;
typedef unsigned int 		uint32;
typedef unsigned long long 	uint64;

typedef signed char 		sint8;
typedef signed short 		sint16;
typedef signed int 			sint32;
typedef signed long long 	sint64;

#define CMD_REQ_BLOCK 1
#define CMD_ACK_BLOCK 2
#define CMD_SMT_SHARE 3
#define CMD_ACK_SHARE 4
#define CMD_NEW_BLOCK 5

// list of known opcodes
#define XPT_OPC_C_AUTH_REQ		1
#define XPT_OPC_S_AUTH_ACK		2
#define XPT_OPC_S_WORKDATA1		3
#define XPT_OPC_C_SUBMIT_SHARE	4
#define XPT_OPC_S_SHARE_ACK		5
#define XPT_OPC_C_SUBMIT_POW	6
#define XPT_OPC_S_MESSAGE		7

#pragma pack(1)

struct BlockInfo
{
	// block header data (relevant for midhash)
	uint32	version;
	uint8	prevBlockHash[32];
	uint8	merkleRoot[32];
	uint32	nTime;
	uint32	nBits;
	uint32	nonce;
	// birthday collision
	uint32	birthdayA;
	uint32	birthdayB;
	uint32	uniqueMerkleSeed;

	uint32	height;
	uint8	merkleRootOriginal[32]; // used to identify work
	// target
	uint8   target[32];
	uint8	targetShare[32];
};


struct SubmitInfo
{
	// block header data (relevant for midhash)
	uint32	head;
	uint8	merkleRoot[32];
	uint8	prevBlockHash[32];
	uint32	version;
	uint32	nTime;
	uint32	nonce;
	uint32	nBits;
	// birthday collision
	uint32	birthdayA;
	uint32	birthdayB;

	uint8	merkleRootOriginal[32]; // used to identify work
	uint8   uniqueMerkleSeedLen;   //always 4
	uint32  uniqueMerkleSeed;
	uint32  shareid;

	SubmitInfo(BlockInfo *block)
	{
		head = ntohl_ex(((sizeof(SubmitInfo)-4)<<8)|XPT_OPC_C_SUBMIT_SHARE);
		memcpy(merkleRoot,block->merkleRoot,32);
		memcpy(prevBlockHash,block->prevBlockHash,32);
		memcpy(&version,&block->version,4);
		memcpy(&nTime,&block->nTime,4);
		memcpy(&nonce,&block->nonce,4);
		memcpy(&nBits,&block->nBits,4);
		memcpy(&birthdayA,&block->birthdayA,4);
		memcpy(&birthdayB,&block->birthdayB,4);
		memcpy(merkleRootOriginal,block->merkleRootOriginal,32);
		uniqueMerkleSeedLen = 4;
		memcpy(&uniqueMerkleSeed,&block->uniqueMerkleSeed,4);
		shareid = 0;
	}

	SubmitInfo()
	{
		head = ntohl_ex(((sizeof(SubmitInfo)-4)<<8)|XPT_OPC_C_SUBMIT_SHARE);
	}
};

struct WorkData
{
	BlockInfo block;

	// coinbase & tx info
	uint16 coinBase1Size;
	uint8 coinBase1[512];
	uint16 coinBase2Size;
	uint8 coinBase2[512];
	uint16 txHashCount;
	uint8 txHashes[32*16];
};

#pragma pack()

#endif //__GLOBLE_H__