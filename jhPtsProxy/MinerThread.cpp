#include "MinerThread.h"
#include "sha2_interface.h"
#include "ProxyClient.h"

volatile uint32 MinerThread::totalShareCount = 0;
volatile uint32 MinerThread::totalCollisionCount = 0;


bool MinerThread::init(uint32 bits,uint32 threadid,bool bStat,bool bCheckHeight)
{
	COLLISION_TABLE_BITS = bits;
	COLLISION_TABLE_SIZE = (1<<COLLISION_TABLE_BITS);
	COLLISION_KEY_WIDTH = (32-COLLISION_TABLE_BITS);
	COLLISION_KEY_MASK = (0xFFFFFFFF<<(32-(COLLISION_KEY_WIDTH)));

	m_collisionMap = new uint32[COLLISION_TABLE_SIZE];
	if (!m_collisionMap)
	{
		printf("new collisionMap failed !!!\n");
		return false;
	}

	m_id = threadid;
	sha256_func = SHA2_INTERFACE::getInstance()->getSHA2_256();
	sha512_func = SHA2_INTERFACE::getInstance()->getSHA2_512();
	m_bStat = bStat;
	m_bCheckHeight = bCheckHeight;
	m_client = ProxyClient::getInstance();
	return true;
}

void MinerThread::uinit()
{
	if(m_collisionMap)
		delete[] m_collisionMap;
	m_collisionMap = NULL;
}


THREAD_FUN MinerThread::main()
{

	while (1)
	{
		if (!m_client->haveEnoughBlock())
		{
			//printf("no enough block info!!!\n");
			Sleep(10);
		}

		BlockInfo *block = m_client->getBlockInfo();
		if (!block)
		{
			continue;
		}
		//printf("protoshares_process [%d]%08X,%08X\n",m_id,block->height,block->uniqueMerkleSeed);
		protoshares_process(block);
		delete block;
	}

	return 0;
}

void MinerThread::protoshares_process(BlockInfo *block)
{
	// generate mid hash using sha256 (header hash)
	uint8 midHash[32];

	sha256_func((uint8*)block, 80, midHash);
	sha256_func((uint8*)midHash, 32, midHash);

	memset(m_collisionMap, 0x00, sizeof(uint32)*COLLISION_TABLE_SIZE);
	// start search
	// uint8 midHash[64];
	uint8 tempHash[32+4];
	uint64 resultHashStorage[8*CACHED_HASHES];
	memcpy(tempHash+4, midHash, 32);
	for(uint32 n=0; n<MAX_MOMENTUM_NONCE; n += BIRTHDAYS_PER_HASH * CACHED_HASHES)
	{
		if (m_bCheckHeight)
		{
			if( block->height != ProxyClient::cur_height )
				break;
		}

		for(uint32 m=0; m<CACHED_HASHES; m++)
		{
			*(uint32*)tempHash = ntohl_ex(n+m*8);
			sha512_func(tempHash,32+4,(uint8*)(resultHashStorage+8*m));
		}
		for(uint32 m=0; m<CACHED_HASHES; m++)
		{
			uint64* resultHash = resultHashStorage + 8*m;
			uint32 i = n + m*8;

			for(uint32 f=0; f<8; f++)
			{
				uint64 birthday = ntohll_ex(resultHash[f]) >> (64ULL-SEARCH_SPACE_BITS);
				uint32 collisionKey = (uint32)((birthday>>18) & COLLISION_KEY_MASK);
				birthday %= COLLISION_TABLE_SIZE;
				if( m_collisionMap[birthday] )
				{
					if( ((m_collisionMap[birthday]&COLLISION_KEY_MASK) != collisionKey) || protoshares_revalidateCollision(block, midHash, m_collisionMap[birthday]&~COLLISION_KEY_MASK, i+f) == false )
					{
						// invalid collision -> ignore
						// todo: Maybe mark this entry as invalid?
					}
				}
				m_collisionMap[birthday] = (i+f) | collisionKey; // we have 6 bits available for validation
			}
		}
	}
}


bool MinerThread::protoshares_revalidateCollision(BlockInfo* block, uint8* midHash, uint32 indexA, uint32 indexB)
{
	//if( indexA > MAX_MOMENTUM_NONCE )
	//	printf("indexA out of range\n");
	//if( indexB > MAX_MOMENTUM_NONCE )
	//	printf("indexB out of range\n");
	//if( indexA == indexB )
	//	printf("indexA == indexB");
	uint8 tempHash[32+4];
	uint64 resultHash[8];
	memcpy(tempHash+4, midHash, 32);
	// get birthday A
	*(uint32*)tempHash = ntohl_ex(indexA&~7);
	sha512_func(tempHash,32+4,(uint8*)resultHash);
	uint64 birthdayA = ntohll_ex(resultHash[indexA&7]) >> (64ULL-SEARCH_SPACE_BITS);
	// get birthday B
	*(uint32*)tempHash = ntohl_ex(indexB&~7);
	sha512_func(tempHash,32+4,(uint8*)resultHash);
	uint64 birthdayB = ntohll_ex(resultHash[indexB&7]) >> (64ULL-SEARCH_SPACE_BITS);
	if( birthdayA != birthdayB )
	{
		return false; // invalid collision
	}
	// birthday collision found
	if (m_bStat)
		totalCollisionCount += 2; // we can use every collision twice -> A B and B A
	//printf("Collision found %8d = %8d | num: %d\n", indexA, indexB, totalCollisionCount);
	// get full block hash (for A B)
	block->birthdayA = ntohl_ex(indexA);
	block->birthdayB = ntohl_ex(indexB);
	uint8 proofOfWorkHash[32];

	sha256_func((uint8*)block,80+8, proofOfWorkHash);
	sha256_func(proofOfWorkHash,32, proofOfWorkHash);
	bool hashMeetsTarget = true;
	uint32* generatedHash32 = (uint32*)proofOfWorkHash;
	uint32* targetHash32 = (uint32*)block->targetShare;
	for(sint32 hc=7; hc>=0; hc--)
	{
		if( ntohl_ex(generatedHash32[hc]) < ntohl_ex(targetHash32[hc]) )
		{
			hashMeetsTarget = true;
			break;
		}
		else if( ntohl_ex(generatedHash32[hc]) > ntohl_ex(targetHash32[hc]) )
		{
			hashMeetsTarget = false;
			break;
		}
	}
	if( hashMeetsTarget )
	{
		if (m_bStat)
			totalShareCount++;
		submitShare(block);
	}
	// get full block hash (for B A)
	block->birthdayA = ntohl_ex(indexB);
	block->birthdayB = ntohl_ex(indexA);
	sha256_func((uint8*)block,80+8, proofOfWorkHash);
	sha256_func(proofOfWorkHash,32, proofOfWorkHash);
	hashMeetsTarget = true;
	generatedHash32 = (uint32*)proofOfWorkHash;
	targetHash32 = (uint32*)block->targetShare;
	for(sint32 hc=7; hc>=0; hc--)
	{
		if( ntohl_ex(generatedHash32[hc]) < ntohl_ex(targetHash32[hc]) )
		{
			hashMeetsTarget = true;
			break;
		}
		else if( ntohl_ex(generatedHash32[hc]) > ntohl_ex(targetHash32[hc]) )
		{
			hashMeetsTarget = false;
			break;
		}
	}
	if( hashMeetsTarget )
	{
		if (m_bStat)
			totalShareCount++;
		submitShare(block);
	}
	return true;
}

void MinerThread::submitShare(BlockInfo *block)
{
	SubmitInfo *info = new SubmitInfo(block);
	if (!info) return;

	m_client->pushShare(info);
}