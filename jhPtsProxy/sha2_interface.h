#ifndef __SHA2_INTERFACE_H__
#define __SHA2_INTERFACE_H__

#include "sha2.h"

enum USE_SHA512_TYPE
{
	USE_DEFAULT_SHA512 = 1,
	USE_SSE4_SHA512,
	USE_AVX_SHA512
};


class SHA2_INTERFACE
{
	USE_SHA512_TYPE m_type;

	SHA2_INTERFACE(USE_SHA512_TYPE t)
	{
		m_type = t;
	}

public:
	virtual ~SHA2_INTERFACE()
	{

	}

	static SHA2_INTERFACE *instance;

	static SHA2_INTERFACE *getInstance(USE_SHA512_TYPE t=USE_DEFAULT_SHA512)
	{
		if (!instance)
		{
			return new SHA2_INTERFACE(t);
		}
		return instance;
	}

	void SHA2_sha256(const uint8 *msg,uint32 len,uint8 *dst)
	{
		sha256_ctx ctx;
		sha256_init(&ctx);
		sha256_update(&ctx,(const unsigned char *)msg,len);
		sha256_final(&ctx,(unsigned char *)dst);
	}

	void SHA2_sha512(const uint8 *msg,uint32 len,uint8 *dst)
	{
		if (m_type == USE_DEFAULT_SHA512)
		{
			sha512_ctx c512;
			sha512_update_final(&c512,(const unsigned char *)msg,len,(unsigned char *)dst);
		}
		else
		{
			//
		}
	}
};

#endif//__SHA2_INTERFACE_H__