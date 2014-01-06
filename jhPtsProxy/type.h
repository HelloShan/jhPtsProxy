#ifndef __TYPE_H__
#define __TYPE_H__

#pragma pack(1)

typedef unsigned char 		uint8;
typedef unsigned short		uint16;
typedef unsigned int 		uint32;
#ifndef __int64
typedef unsigned __int64	uint64; 
#else
typedef unsigned long long 	uint64;
#endif

typedef signed char 		sint8;
typedef signed short 		sint16;
typedef signed int 			sint32;
#ifndef __int64
typedef signed __int64	sint64; 
#else
typedef signed long long 	sint64;
#endif

union XptHead
{
	uint32 len;
	uint8  opt[4];
};

struct WorkerInfo
{
	XptHead head;

};

#pragma pack()
#endif //__TYPE_H__