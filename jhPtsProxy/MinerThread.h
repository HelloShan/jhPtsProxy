#ifndef __MINER_THREAD_H__
#define __MINER_THREAD_H__
#include "global.h"
#include "Thread.h"


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

	uint32 totalCollisionCount;
	uint32 totalShareCount;

	ProxyClient *m_client;
	uint32 m_id;
public:
	MinerThread(){}
	virtual ~MinerThread(){}

	bool init(uint32 bits,ProxyClient *client,uint32 threadid);
	virtual THREAD_FUN main();

private:
	void protoshares_process(BlockInfo *block);
	bool protoshares_revalidateCollision(BlockInfo* block, uint8* midHash, uint32 indexA, uint32 indexB);
	void submitShare(BlockInfo *block);

};

#endif//__MINER_THREAD_H__