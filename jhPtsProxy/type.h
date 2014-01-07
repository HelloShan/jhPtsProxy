#ifndef __TYPE_H__
#define __TYPE_H__


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

#pragma pack(1)
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
};

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

struct WorkData
{
	BlockInfo block;

	// coinbase & tx info
	uint16 coinBase1Size;
	uint8 coinBase1[512];
	uint16 coinBase2Size;
	uint8 coinBase2[512];
	uint16 txHashCount;
	uint8 txHashes[32*16]; // space for 4096 tx hashes
};

#pragma pack()

#endif //__TYPE_H__