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

#include "type.h"


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

#endif //__GLOBLE_H__