#ifndef __SHA2_INTERFACE_H__
#define __SHA2_INTERFACE_H__

#include "sha2.h"
#include "sha512.h"

enum USE_SHA512_TYPE
{
	USE_DEFAULT_SHA512 = 1,
	USE_SSE4_SHA512,
	USE_AVX_SHA512
};

typedef void (*SHA2_FUNC)(const uint8 *msg,uint32 len,uint8 *dst);


class SHA2_INTERFACE
{
	USE_SHA512_TYPE m_type;

	SHA2_INTERFACE(USE_SHA512_TYPE t)
	{
		m_type = t;

#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
#else
		if (m_type == USE_SSE4_SHA512)
		{
			Init_SHA512_sse4();
		}
		if (m_type == USE_AVX_SHA512)
		{
			Init_SHA512_avx();
		}
#endif

	}

	static void SHA2_sha256(const uint8 *msg,uint32 len,uint8 *dst)
	{
		sha256_ctx ctx;
		sha256_init(&ctx);
		sha256_update(&ctx,(const unsigned char *)msg,len);
		sha256_final(&ctx,(unsigned char *)dst);
	}

	static void SHA2_sha512_default(const uint8 *msg,uint32 len,uint8 *dst)
	{
		sha512_ctx c512;
		sha512_init(&c512);
		sha512_update_final(&c512,(const unsigned char *)msg,len,(unsigned char *)dst);
	}

#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
#else
	static void SHA2_sha512_sse4avx(const uint8 *msg,uint32 len,uint8 *dst)
	{
		SHA512_Context c512_avxsse;
		SHA512_Init(&c512_avxsse);
		SHA512_Update(&c512_avxsse, msg, len);
		SHA512_Final(&c512_avxsse, (unsigned char*)dst);
	}
#endif

	static SHA2_INTERFACE *instance;
public:
	virtual ~SHA2_INTERFACE()
	{

	}

	static SHA2_INTERFACE *getInstance(USE_SHA512_TYPE t=USE_DEFAULT_SHA512)
	{
		if (!instance)
		{
			instance = new SHA2_INTERFACE(t);
		}
		return instance;
	}

	SHA2_FUNC getSHA2_256()
	{
		return &SHA2_INTERFACE::SHA2_sha256;
	}

	SHA2_FUNC getSHA2_512()
	{

#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
#else
		if (m_type != USE_DEFAULT_SHA512)
			return &SHA2_INTERFACE::SHA2_sha512_sse4avx;
#endif

		return &SHA2_INTERFACE::SHA2_sha512_default;
	}

};

#endif//__SHA2_INTERFACE_H__