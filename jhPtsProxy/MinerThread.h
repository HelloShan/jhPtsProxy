#ifndef __MINER_THREAD_H__
#define __MINER_THREAD_H__
#include "global.h"
#include "Thread.h"
#include "sha2_interface.h"
#include "ThreadQueue.h"


#define MAX_MOMENTUM_NONCE		(1<<26)	// 67.108.864
#define SEARCH_SPACE_BITS		50
#define BIRTHDAYS_PER_HASH		8


#define CACHED_HASHES			(32)

class ProxyClient;
class MinerThread:public Thread
{

	uint32 COLLISION_TABLE_BITS;
	uint32 COLLISION_TABLE_SIZE;
	uint32 COLLISION_KEY_WIDTH;
	uint32 COLLISION_KEY_MASK;

	uint32 *m_collisionMap;

	//stat
	bool m_bStat;
	bool m_bCheckHeight;

	uint32 m_id;

	SHA2_FUNC sha256_func;
	SHA2_FUNC sha512_func;

	//ThreadQueue<BlockInfo*> m_block;
	ProxyClient *m_client;
public:
	MinerThread(){}
	virtual ~MinerThread(){}

	static volatile uint32 totalCollisionCount;
	static volatile uint32 totalShareCount;

	bool init(uint32 bits,uint32 threadid,bool bStat,bool bCheckHeight);
	void uinit();
	virtual THREAD_FUN main();

private:
	void protoshares_process(BlockInfo *block);
	bool protoshares_revalidateCollision(BlockInfo* block, uint8* midHash, uint32 indexA, uint32 indexB);
	void submitShare(BlockInfo *block);
	//BlockInfo *getBlockInfo();

};

#endif//__MINER_THREAD_H__