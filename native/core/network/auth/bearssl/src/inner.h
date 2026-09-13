

#ifndef INNER_H__
#define INNER_H__

#include <string.h>
#include <limits.h>

#include "config.h"
#include "bearssl.h"


#if _MSC_VER
#pragma warning( disable : 4146 )
#endif


#define BR_MAX_RSA_SIZE   4096


#define BR_MIN_RSA_SIZE   512


#define BR_MAX_RSA_FACTOR   ((BR_MAX_RSA_SIZE + 64) >> 1)


#define BR_MAX_EC_SIZE   528


#ifndef BR_64
#if ((ULONG_MAX >> 31) >> 31) == 3
#define BR_64   1
#elif defined(__ia64) || defined(__itanium__) || defined(_M_IA64)
#define BR_64   1
#elif defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) \
	|| defined(__64BIT__) || defined(_LP64) || defined(__LP64__)
#define BR_64   1
#elif defined(__sparc64__)
#define BR_64   1
#elif defined(__x86_64__) || defined(_M_X64)
#define BR_64   1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define BR_64   1
#elif defined(__mips64)
#define BR_64   1
#endif
#endif


#ifndef BR_LOMUL
#if BR_ARMEL_CORTEXM_GCC
#define BR_LOMUL   1
#endif
#endif


#ifndef BR_i386
#if __i386__ || _M_IX86
#define BR_i386   1
#endif
#endif

#ifndef BR_amd64
#if __x86_64__ || _M_X64
#define BR_amd64   1
#endif
#endif




#ifndef BR_GCC
#if __GNUC__ && !__clang__
#define BR_GCC   1

#if __GNUC__ > 4
#define BR_GCC_5_0   1
#elif __GNUC__ == 4 && __GNUC_MINOR__ >= 9
#define BR_GCC_4_9   1
#elif __GNUC__ == 4 && __GNUC_MINOR__ >= 8
#define BR_GCC_4_8   1
#elif __GNUC__ == 4 && __GNUC_MINOR__ >= 7
#define BR_GCC_4_7   1
#elif __GNUC__ == 4 && __GNUC_MINOR__ >= 6
#define BR_GCC_4_6   1
#elif __GNUC__ == 4 && __GNUC_MINOR__ >= 5
#define BR_GCC_4_5   1
#elif __GNUC__ == 4 && __GNUC_MINOR__ >= 4
#define BR_GCC_4_4   1
#endif

#if BR_GCC_5_0
#define BR_GCC_4_9   1
#endif
#if BR_GCC_4_9
#define BR_GCC_4_8   1
#endif
#if BR_GCC_4_8
#define BR_GCC_4_7   1
#endif
#if BR_GCC_4_7
#define BR_GCC_4_6   1
#endif
#if BR_GCC_4_6
#define BR_GCC_4_5   1
#endif
#if BR_GCC_4_5
#define BR_GCC_4_4   1
#endif

#endif
#endif


#ifndef BR_CLANG
#if __clang__
#define BR_CLANG   1

#if __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 8)
#define BR_CLANG_3_8   1
#elif __clang_major__ == 3 && __clang_minor__ >= 7
#define BR_CLANG_3_7   1
#endif

#if BR_CLANG_3_8
#define BR_CLANG_3_7   1
#endif

#endif
#endif


#ifndef BR_MSC
#if _MSC_VER
#define BR_MSC   1

#if _MSC_VER >= 1900
#define BR_MSC_2015   1
#elif _MSC_VER >= 1800
#define BR_MSC_2013   1
#elif _MSC_VER >= 1700
#define BR_MSC_2012   1
#elif _MSC_VER >= 1600
#define BR_MSC_2010   1
#elif _MSC_VER >= 1500
#define BR_MSC_2008   1
#elif _MSC_VER >= 1400
#define BR_MSC_2005   1
#endif

#if BR_MSC_2015
#define BR_MSC_2013   1
#endif
#if BR_MSC_2013
#define BR_MSC_2012   1
#endif
#if BR_MSC_2012
#define BR_MSC_2010   1
#endif
#if BR_MSC_2010
#define BR_MSC_2008   1
#endif
#if BR_MSC_2008
#define BR_MSC_2005   1
#endif

#endif
#endif


#if BR_GCC_4_4 || BR_CLANG_3_7
#define BR_TARGET(x)   __attribute__((target(x)))
#else
#define BR_TARGET(x)
#endif


#ifndef BR_AES_X86NI
#if (BR_i386 || BR_amd64) && (BR_GCC_4_8 || BR_CLANG_3_7 || BR_MSC_2012)
#define BR_AES_X86NI   1
#endif
#endif


#ifndef BR_SSE2
#if (BR_i386 || BR_amd64) && (BR_GCC_4_4 || BR_CLANG_3_7 || BR_MSC_2005)
#define BR_SSE2   1
#endif
#endif


#ifndef BR_RDRAND
#if (BR_i386 || BR_amd64) && (BR_GCC_4_6 || BR_CLANG_3_7 || BR_MSC_2012)
#define BR_RDRAND   1
#endif
#endif



#ifndef BR_USE_URANDOM
#if defined _AIX \
	|| defined __ANDROID__ \
	|| defined __FreeBSD__ \
	|| defined __NetBSD__ \
	|| defined __OpenBSD__ \
	|| defined __DragonFly__ \
	|| defined __linux__ \
	|| (defined __sun && (defined __SVR4 || defined __svr4__)) \
	|| (defined __APPLE__ && defined __MACH__)
#define BR_USE_URANDOM   1
#endif
#endif

#ifndef BR_USE_GETENTROPY
#if (defined __linux__ \
	&& (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))) \
	|| (defined __FreeBSD__ && __FreeBSD__ >= 12) \
	|| defined __OpenBSD__
#define BR_USE_GETENTROPY   1
#endif
#endif

#ifndef BR_USE_WIN32_RAND
#if defined _WIN32 || defined _WIN64
#define BR_USE_WIN32_RAND   1
#endif
#endif


#ifndef BR_POWER8
#if __GNUC__ && ((_ARCH_PWR8 || _ARCH_PPC) && __CRYPTO__)
#define BR_POWER8   1
#endif
#endif


#if BR_POWER8
#if defined BR_POWER8_LE
#undef BR_POWER8_BE
#if BR_POWER8_LE
#define BR_POWER8_BE   0
#else
#define BR_POWER8_BE   1
#endif
#elif defined BR_POWER8_BE
#undef BR_POWER8_LE
#if BR_POWER8_BE
#define BR_POWER8_LE   0
#else
#define BR_POWER8_LE   1
#endif
#else
#if __LITTLE_ENDIAN__
#define BR_POWER8_LE   1
#define BR_POWER8_BE   0
#else
#define BR_POWER8_LE   0
#define BR_POWER8_BE   1
#endif
#endif
#endif


#if !defined BR_INT128 && !defined BR_UMUL128
#ifdef __SIZEOF_INT128__
#define BR_INT128    1
#elif _M_X64
#define BR_UMUL128   1
#endif
#endif


#if !defined BR_LE_UNALIGNED && !defined BR_BE_UNALIGNED

#if __i386 || __i386__ || __x86_64__ || _M_IX86 || _M_X64
#define BR_LE_UNALIGNED   1
#elif BR_POWER8_BE
#define BR_BE_UNALIGNED   1
#elif BR_POWER8_LE
#define BR_LE_UNALIGNED   1
#elif (__powerpc__ || __powerpc64__ || _M_PPC || _ARCH_PPC || _ARCH_PPC64) \
	&& __BIG_ENDIAN__
#define BR_BE_UNALIGNED   1
#endif

#endif



#ifndef BR_USE_UNIX_TIME
#if defined __unix__ || defined __linux__ \
	|| defined _POSIX_SOURCE || defined _POSIX_C_SOURCE \
	|| (defined __APPLE__ && defined __MACH__)
#define BR_USE_UNIX_TIME   1
#endif
#endif

#ifndef BR_USE_WIN32_TIME
#if defined _WIN32 || defined _WIN64
#define BR_USE_WIN32_TIME   1
#endif
#endif




typedef union {
	uint16_t u;
	unsigned char b[sizeof(uint16_t)];
} br_union_u16;

typedef union {
	uint32_t u;
	unsigned char b[sizeof(uint32_t)];
} br_union_u32;

typedef union {
	uint64_t u;
	unsigned char b[sizeof(uint64_t)];
} br_union_u64;

static inline void
br_enc16le(void *dst, unsigned x)
{
#if BR_LE_UNALIGNED
	((br_union_u16 *)dst)->u = x;
#else
	unsigned char *buf;

	buf = dst;
	buf[0] = (unsigned char)x;
	buf[1] = (unsigned char)(x >> 8);
#endif
}

static inline void
br_enc16be(void *dst, unsigned x)
{
#if BR_BE_UNALIGNED
	((br_union_u16 *)dst)->u = x;
#else
	unsigned char *buf;

	buf = dst;
	buf[0] = (unsigned char)(x >> 8);
	buf[1] = (unsigned char)x;
#endif
}

static inline unsigned
br_dec16le(const void *src)
{
#if BR_LE_UNALIGNED
	return ((const br_union_u16 *)src)->u;
#else
	const unsigned char *buf;

	buf = src;
	return (unsigned)buf[0] | ((unsigned)buf[1] << 8);
#endif
}

static inline unsigned
br_dec16be(const void *src)
{
#if BR_BE_UNALIGNED
	return ((const br_union_u16 *)src)->u;
#else
	const unsigned char *buf;

	buf = src;
	return ((unsigned)buf[0] << 8) | (unsigned)buf[1];
#endif
}

static inline void
br_enc32le(void *dst, uint32_t x)
{
#if BR_LE_UNALIGNED
	((br_union_u32 *)dst)->u = x;
#else
	unsigned char *buf;

	buf = dst;
	buf[0] = (unsigned char)x;
	buf[1] = (unsigned char)(x >> 8);
	buf[2] = (unsigned char)(x >> 16);
	buf[3] = (unsigned char)(x >> 24);
#endif
}

static inline void
br_enc32be(void *dst, uint32_t x)
{
#if BR_BE_UNALIGNED
	((br_union_u32 *)dst)->u = x;
#else
	unsigned char *buf;

	buf = dst;
	buf[0] = (unsigned char)(x >> 24);
	buf[1] = (unsigned char)(x >> 16);
	buf[2] = (unsigned char)(x >> 8);
	buf[3] = (unsigned char)x;
#endif
}

static inline uint32_t
br_dec32le(const void *src)
{
#if BR_LE_UNALIGNED
	return ((const br_union_u32 *)src)->u;
#else
	const unsigned char *buf;

	buf = src;
	return (uint32_t)buf[0]
		| ((uint32_t)buf[1] << 8)
		| ((uint32_t)buf[2] << 16)
		| ((uint32_t)buf[3] << 24);
#endif
}

static inline uint32_t
br_dec32be(const void *src)
{
#if BR_BE_UNALIGNED
	return ((const br_union_u32 *)src)->u;
#else
	const unsigned char *buf;

	buf = src;
	return ((uint32_t)buf[0] << 24)
		| ((uint32_t)buf[1] << 16)
		| ((uint32_t)buf[2] << 8)
		| (uint32_t)buf[3];
#endif
}

static inline void
br_enc64le(void *dst, uint64_t x)
{
#if BR_LE_UNALIGNED
	((br_union_u64 *)dst)->u = x;
#else
	unsigned char *buf;

	buf = dst;
	br_enc32le(buf, (uint32_t)x);
	br_enc32le(buf + 4, (uint32_t)(x >> 32));
#endif
}

static inline void
br_enc64be(void *dst, uint64_t x)
{
#if BR_BE_UNALIGNED
	((br_union_u64 *)dst)->u = x;
#else
	unsigned char *buf;

	buf = dst;
	br_enc32be(buf, (uint32_t)(x >> 32));
	br_enc32be(buf + 4, (uint32_t)x);
#endif
}

static inline uint64_t
br_dec64le(const void *src)
{
#if BR_LE_UNALIGNED
	return ((const br_union_u64 *)src)->u;
#else
	const unsigned char *buf;

	buf = src;
	return (uint64_t)br_dec32le(buf)
		| ((uint64_t)br_dec32le(buf + 4) << 32);
#endif
}

static inline uint64_t
br_dec64be(const void *src)
{
#if BR_BE_UNALIGNED
	return ((const br_union_u64 *)src)->u;
#else
	const unsigned char *buf;

	buf = src;
	return ((uint64_t)br_dec32be(buf) << 32)
		| (uint64_t)br_dec32be(buf + 4);
#endif
}


void br_range_dec16le(uint16_t *v, size_t num, const void *src);
void br_range_dec16be(uint16_t *v, size_t num, const void *src);
void br_range_enc16le(void *dst, const uint16_t *v, size_t num);
void br_range_enc16be(void *dst, const uint16_t *v, size_t num);

void br_range_dec32le(uint32_t *v, size_t num, const void *src);
void br_range_dec32be(uint32_t *v, size_t num, const void *src);
void br_range_enc32le(void *dst, const uint32_t *v, size_t num);
void br_range_enc32be(void *dst, const uint32_t *v, size_t num);

void br_range_dec64le(uint64_t *v, size_t num, const void *src);
void br_range_dec64be(uint64_t *v, size_t num, const void *src);
void br_range_enc64le(void *dst, const uint64_t *v, size_t num);
void br_range_enc64be(void *dst, const uint64_t *v, size_t num);


static inline uint32_t
br_swap32(uint32_t x)
{
	x = ((x & (uint32_t)0x00FF00FF) << 8)
		| ((x >> 8) & (uint32_t)0x00FF00FF);
	return (x << 16) | (x >> 16);
}





extern const uint32_t br_md5_IV[];
extern const uint32_t br_sha1_IV[];
extern const uint32_t br_sha224_IV[];
extern const uint32_t br_sha256_IV[];


void br_md5_round(const unsigned char *buf, uint32_t *val);
void br_sha1_round(const unsigned char *buf, uint32_t *val);
void br_sha2small_round(const unsigned char *buf, uint32_t *val);


void br_tls_phash(void *dst, size_t len,
	const br_hash_class *dig,
	const void *secret, size_t secret_len, const char *label,
	size_t seed_num, const br_tls_prf_seed_chunk *seed);


static inline void
br_multihash_copyimpl(br_multihash_context *dst,
	const br_multihash_context *src)
{
	memcpy((void *)dst->impl, src->impl, sizeof src->impl);
}





static inline uint32_t
NOT(uint32_t ctl)
{
	return ctl ^ 1;
}


static inline uint32_t
MUX(uint32_t ctl, uint32_t x, uint32_t y)
{
	return y ^ (-ctl & (x ^ y));
}


static inline uint32_t
EQ(uint32_t x, uint32_t y)
{
	uint32_t q;

	q = x ^ y;
	return NOT((q | -q) >> 31);
}


static inline uint32_t
NEQ(uint32_t x, uint32_t y)
{
	uint32_t q;

	q = x ^ y;
	return (q | -q) >> 31;
}


static inline uint32_t
GT(uint32_t x, uint32_t y)
{
	
	uint32_t z;

	z = y - x;
	return (z ^ ((x ^ y) & (x ^ z))) >> 31;
}


#define GE(x, y)   NOT(GT(y, x))
#define LT(x, y)   GT(y, x)
#define LE(x, y)   NOT(GT(x, y))


static inline int32_t
CMP(uint32_t x, uint32_t y)
{
	return (int32_t)GT(x, y) | -(int32_t)GT(y, x);
}


static inline uint32_t
EQ0(int32_t x)
{
	uint32_t q;

	q = (uint32_t)x;
	return ~(q | -q) >> 31;
}


static inline uint32_t
GT0(int32_t x)
{
	
	uint32_t q;

	q = (uint32_t)x;
	return (~q & -q) >> 31;
}


static inline uint32_t
GE0(int32_t x)
{
	return ~(uint32_t)x >> 31;
}


static inline uint32_t
LT0(int32_t x)
{
	return (uint32_t)x >> 31;
}


static inline uint32_t
LE0(int32_t x)
{
	uint32_t q;

	
	q = (uint32_t)x;
	return (q | ~-q) >> 31;
}


void br_ccopy(uint32_t ctl, void *dst, const void *src, size_t len);

#define CCOPY   br_ccopy


static inline uint32_t
BIT_LENGTH(uint32_t x)
{
	uint32_t k, c;

	k = NEQ(x, 0);
	c = GT(x, 0xFFFF); x = MUX(c, x >> 16, x); k += c << 4;
	c = GT(x, 0x00FF); x = MUX(c, x >>  8, x); k += c << 3;
	c = GT(x, 0x000F); x = MUX(c, x >>  4, x); k += c << 2;
	c = GT(x, 0x0003); x = MUX(c, x >>  2, x); k += c << 1;
	k += GT(x, 0x0001);
	return k;
}


static inline uint32_t
MIN(uint32_t x, uint32_t y)
{
	return MUX(GT(x, y), y, x);
}


static inline uint32_t
MAX(uint32_t x, uint32_t y)
{
	return MUX(GT(x, y), x, y);
}


#define MUL(x, y)   ((uint64_t)(x) * (uint64_t)(y))

#if BR_CT_MUL31


#define MUL31(x, y)   ((uint64_t)((x) | (uint32_t)0x80000000) \
                       * (uint64_t)((y) | (uint32_t)0x80000000) \
                       - ((uint64_t)(x) << 31) - ((uint64_t)(y) << 31) \
                       - ((uint64_t)1 << 62))
static inline uint32_t
MUL31_lo(uint32_t x, uint32_t y)
{
	uint32_t xl, xh;
	uint32_t yl, yh;

	xl = (x & 0xFFFF) | (uint32_t)0x80000000;
	xh = (x >> 16) | (uint32_t)0x80000000;
	yl = (y & 0xFFFF) | (uint32_t)0x80000000;
	yh = (y >> 16) | (uint32_t)0x80000000;
	return (xl * yl + ((xl * yh + xh * yl) << 16)) & (uint32_t)0x7FFFFFFF;
}

#else


#define MUL31(x, y)     ((uint64_t)(x) * (uint64_t)(y))
#define MUL31_lo(x, y)  (((uint32_t)(x) * (uint32_t)(y)) & (uint32_t)0x7FFFFFFF)

#endif


#if BR_CT_MUL15
#define MUL15(x, y)   (((uint32_t)(x) | (uint32_t)0x80000000) \
                       * ((uint32_t)(y) | (uint32_t)0x80000000) \
		       & (uint32_t)0x7FFFFFFF)
#else
#define MUL15(x, y)   ((uint32_t)(x) * (uint32_t)(y))
#endif


#if BR_NO_ARITH_SHIFT
#define ARSH(x, n)   (((uint32_t)(x) >> (n)) \
                      | ((-((uint32_t)(x) >> 31)) << (32 - (n))))
#else
#define ARSH(x, n)   ((*(int32_t *)&(x)) >> (n))
#endif


uint32_t br_divrem(uint32_t hi, uint32_t lo, uint32_t d, uint32_t *r);


static inline uint32_t
br_rem(uint32_t hi, uint32_t lo, uint32_t d)
{
	uint32_t r;

	br_divrem(hi, lo, d, &r);
	return r;
}


static inline uint32_t
br_div(uint32_t hi, uint32_t lo, uint32_t d)
{
	uint32_t r;

	return br_divrem(hi, lo, d, &r);
}






uint32_t br_i32_bit_length(uint32_t *x, size_t xlen);


void br_i32_decode(uint32_t *x, const void *src, size_t len);


uint32_t br_i32_decode_mod(uint32_t *x,
	const void *src, size_t len, const uint32_t *m);


void br_i32_reduce(uint32_t *x, const uint32_t *a, const uint32_t *m);


void br_i32_decode_reduce(uint32_t *x,
	const void *src, size_t len, const uint32_t *m);


void br_i32_encode(void *dst, size_t len, const uint32_t *x);


void br_i32_muladd_small(uint32_t *x, uint32_t z, const uint32_t *m);


static inline uint32_t
br_i32_word(const uint32_t *a, uint32_t off)
{
	size_t u;
	unsigned j;

	u = (size_t)(off >> 5) + 1;
	j = (unsigned)off & 31;
	if (j == 0) {
		return a[u];
	} else {
		return (a[u] >> j) | (a[u + 1] << (32 - j));
	}
}


uint32_t br_i32_iszero(const uint32_t *x);


uint32_t br_i32_add(uint32_t *a, const uint32_t *b, uint32_t ctl);


uint32_t br_i32_sub(uint32_t *a, const uint32_t *b, uint32_t ctl);


void br_i32_mulacc(uint32_t *d, const uint32_t *a, const uint32_t *b);


static inline void
br_i32_zero(uint32_t *x, uint32_t bit_len)
{
	*x ++ = bit_len;
	memset(x, 0, ((bit_len + 31) >> 5) * sizeof *x);
}


uint32_t br_i32_ninv32(uint32_t x);


void br_i32_to_monty(uint32_t *x, const uint32_t *m);


void br_i32_from_monty(uint32_t *x, const uint32_t *m, uint32_t m0i);


void br_i32_montymul(uint32_t *d, const uint32_t *x, const uint32_t *y,
	const uint32_t *m, uint32_t m0i);


void br_i32_modpow(uint32_t *x, const unsigned char *e, size_t elen,
	const uint32_t *m, uint32_t m0i, uint32_t *t1, uint32_t *t2);






uint32_t br_i31_iszero(const uint32_t *x);


uint32_t br_i31_add(uint32_t *a, const uint32_t *b, uint32_t ctl);


uint32_t br_i31_sub(uint32_t *a, const uint32_t *b, uint32_t ctl);


uint32_t br_i31_bit_length(uint32_t *x, size_t xlen);


void br_i31_decode(uint32_t *x, const void *src, size_t len);


uint32_t br_i31_decode_mod(uint32_t *x,
	const void *src, size_t len, const uint32_t *m);


static inline void
br_i31_zero(uint32_t *x, uint32_t bit_len)
{
	*x ++ = bit_len;
	memset(x, 0, ((bit_len + 31) >> 5) * sizeof *x);
}


void br_i31_rshift(uint32_t *x, int count);


void br_i31_reduce(uint32_t *x, const uint32_t *a, const uint32_t *m);


void br_i31_decode_reduce(uint32_t *x,
	const void *src, size_t len, const uint32_t *m);


void br_i31_muladd_small(uint32_t *x, uint32_t z, const uint32_t *m);


void br_i31_encode(void *dst, size_t len, const uint32_t *x);


uint32_t br_i31_ninv31(uint32_t x);


void br_i31_montymul(uint32_t *d, const uint32_t *x, const uint32_t *y,
	const uint32_t *m, uint32_t m0i);


void br_i31_to_monty(uint32_t *x, const uint32_t *m);


void br_i31_from_monty(uint32_t *x, const uint32_t *m, uint32_t m0i);


void br_i31_modpow(uint32_t *x, const unsigned char *e, size_t elen,
	const uint32_t *m, uint32_t m0i, uint32_t *t1, uint32_t *t2);


uint32_t br_i31_modpow_opt(uint32_t *x, const unsigned char *e, size_t elen,
	const uint32_t *m, uint32_t m0i, uint32_t *tmp, size_t twlen);


void br_i31_mulacc(uint32_t *d, const uint32_t *a, const uint32_t *b);


uint32_t br_i31_moddiv(uint32_t *x, const uint32_t *y,
	const uint32_t *m, uint32_t m0i, uint32_t *t);





static inline void
br_i15_zero(uint16_t *x, uint16_t bit_len)
{
	*x ++ = bit_len;
	memset(x, 0, ((bit_len + 15) >> 4) * sizeof *x);
}

uint32_t br_i15_iszero(const uint16_t *x);

uint16_t br_i15_ninv15(uint16_t x);

uint32_t br_i15_add(uint16_t *a, const uint16_t *b, uint32_t ctl);

uint32_t br_i15_sub(uint16_t *a, const uint16_t *b, uint32_t ctl);

void br_i15_muladd_small(uint16_t *x, uint16_t z, const uint16_t *m);

void br_i15_montymul(uint16_t *d, const uint16_t *x, const uint16_t *y,
	const uint16_t *m, uint16_t m0i);

void br_i15_to_monty(uint16_t *x, const uint16_t *m);

void br_i15_modpow(uint16_t *x, const unsigned char *e, size_t elen,
	const uint16_t *m, uint16_t m0i, uint16_t *t1, uint16_t *t2);

uint32_t br_i15_modpow_opt(uint16_t *x, const unsigned char *e, size_t elen,
	const uint16_t *m, uint16_t m0i, uint16_t *tmp, size_t twlen);

void br_i15_encode(void *dst, size_t len, const uint16_t *x);

uint32_t br_i15_decode_mod(uint16_t *x,
	const void *src, size_t len, const uint16_t *m);

void br_i15_rshift(uint16_t *x, int count);

uint32_t br_i15_bit_length(uint16_t *x, size_t xlen);

void br_i15_decode(uint16_t *x, const void *src, size_t len);

void br_i15_from_monty(uint16_t *x, const uint16_t *m, uint16_t m0i);

void br_i15_decode_reduce(uint16_t *x,
	const void *src, size_t len, const uint16_t *m);

void br_i15_reduce(uint16_t *x, const uint16_t *a, const uint16_t *m);

void br_i15_mulacc(uint16_t *d, const uint16_t *a, const uint16_t *b);

uint32_t br_i15_moddiv(uint16_t *x, const uint16_t *y,
	const uint16_t *m, uint16_t m0i, uint16_t *t);


uint32_t br_i62_modpow_opt(uint32_t *x31, const unsigned char *e, size_t elen,
	const uint32_t *m31, uint32_t m0i31, uint64_t *tmp, size_t twlen);


typedef uint32_t (*br_i31_modpow_opt_type)(uint32_t *x,
	const unsigned char *e, size_t elen,
	const uint32_t *m, uint32_t m0i, uint32_t *tmp, size_t twlen);


uint32_t br_i62_modpow_opt_as_i31(uint32_t *x,
	const unsigned char *e, size_t elen,
	const uint32_t *m, uint32_t m0i, uint32_t *tmp, size_t twlen);



static inline size_t
br_digest_size(const br_hash_class *digest_class)
{
	return (size_t)(digest_class->desc >> BR_HASHDESC_OUT_OFF)
		& BR_HASHDESC_OUT_MASK;
}


size_t br_digest_size_by_ID(int digest_id);


const unsigned char *br_digest_OID(int digest_id, size_t *len);





void br_des_do_IP(uint32_t *xl, uint32_t *xr);


void br_des_do_invIP(uint32_t *xl, uint32_t *xr);


void br_des_keysched_unit(uint32_t *skey, const void *key);


void br_des_rev_skey(uint32_t *skey);


unsigned br_des_tab_keysched(uint32_t *skey, const void *key, size_t key_len);


unsigned br_des_ct_keysched(uint32_t *skey, const void *key, size_t key_len);


void br_des_ct_skey_expand(uint32_t *sk_exp,
	unsigned num_rounds, const uint32_t *skey);


void br_des_tab_process_block(unsigned num_rounds,
	const uint32_t *skey, void *block);


void br_des_ct_process_block(unsigned num_rounds,
	const uint32_t *skey, void *block);





extern const unsigned char br_aes_S[];


unsigned br_aes_keysched(uint32_t *skey, const void *key, size_t key_len);


unsigned br_aes_big_keysched_inv(uint32_t *skey,
	const void *key, size_t key_len);


void br_aes_big_encrypt(unsigned num_rounds, const uint32_t *skey, void *data);


void br_aes_big_decrypt(unsigned num_rounds, const uint32_t *skey, void *data);


void br_aes_small_encrypt(unsigned num_rounds,
	const uint32_t *skey, void *data);


void br_aes_small_decrypt(unsigned num_rounds,
	const uint32_t *skey, void *data);




void br_aes_ct_ortho(uint32_t *q);


void br_aes_ct_bitslice_Sbox(uint32_t *q);


void br_aes_ct_bitslice_invSbox(uint32_t *q);


void br_aes_ct_bitslice_encrypt(unsigned num_rounds,
	const uint32_t *skey, uint32_t *q);


void br_aes_ct_bitslice_decrypt(unsigned num_rounds,
	const uint32_t *skey, uint32_t *q);


unsigned br_aes_ct_keysched(uint32_t *comp_skey,
	const void *key, size_t key_len);


void br_aes_ct_skey_expand(uint32_t *skey,
	unsigned num_rounds, const uint32_t *comp_skey);




void br_aes_ct64_ortho(uint64_t *q);


void br_aes_ct64_interleave_in(uint64_t *q0, uint64_t *q1, const uint32_t *w);


void br_aes_ct64_interleave_out(uint32_t *w, uint64_t q0, uint64_t q1);


void br_aes_ct64_bitslice_Sbox(uint64_t *q);


void br_aes_ct64_bitslice_invSbox(uint64_t *q);


void br_aes_ct64_bitslice_encrypt(unsigned num_rounds,
	const uint64_t *skey, uint64_t *q);


void br_aes_ct64_bitslice_decrypt(unsigned num_rounds,
	const uint64_t *skey, uint64_t *q);


unsigned br_aes_ct64_keysched(uint64_t *comp_skey,
	const void *key, size_t key_len);


void br_aes_ct64_skey_expand(uint64_t *skey,
	unsigned num_rounds, const uint64_t *comp_skey);


int br_aes_x86ni_supported(void);


unsigned br_aes_x86ni_keysched_enc(unsigned char *skni,
	const void *key, size_t len);


unsigned br_aes_x86ni_keysched_dec(unsigned char *skni,
	const void *key, size_t len);


int br_aes_pwr8_supported(void);


unsigned br_aes_pwr8_keysched(unsigned char *skni,
	const void *key, size_t len);





uint32_t br_rsa_pkcs1_sig_pad(const unsigned char *hash_oid,
	const unsigned char *hash, size_t hash_len,
	uint32_t n_bitlen, unsigned char *x);


uint32_t br_rsa_pkcs1_sig_unpad(const unsigned char *sig, size_t sig_len,
	const unsigned char *hash_oid, size_t hash_len,
	unsigned char *hash_out);


uint32_t br_rsa_pss_sig_pad(const br_prng_class **rng,
	const br_hash_class *hf_data, const br_hash_class *hf_mgf1,
	const unsigned char *hash, size_t salt_len,
	uint32_t n_bitlen, unsigned char *x);


uint32_t br_rsa_pss_sig_unpad(
	const br_hash_class *hf_data, const br_hash_class *hf_mgf1,
	const unsigned char *hash, size_t salt_len,
	const br_rsa_public_key *pk, unsigned char *x);


size_t br_rsa_oaep_pad(const br_prng_class **rnd, const br_hash_class *dig,
	const void *label, size_t label_len, const br_rsa_public_key *pk,
	void *dst, size_t dst_nax_len, const void *src, size_t src_len);


uint32_t br_rsa_oaep_unpad(const br_hash_class *dig,
	const void *label, size_t label_len, void *data, size_t *len);


void br_mgf1_xor(void *data, size_t len,
	const br_hash_class *dig, const void *seed, size_t seed_len);


uint32_t br_rsa_i31_keygen_inner(const br_prng_class **rng,
	br_rsa_private_key *sk, void *kbuf_priv,
	br_rsa_public_key *pk, void *kbuf_pub,
	unsigned size, uint32_t pubexp, br_i31_modpow_opt_type mp31);





typedef struct {
	int curve;
	const unsigned char *order;
	size_t order_len;
	const unsigned char *generator;
	size_t generator_len;
} br_ec_curve_def;

extern const br_ec_curve_def br_secp256r1;
extern const br_ec_curve_def br_secp384r1;
extern const br_ec_curve_def br_secp521r1;


extern const br_ec_curve_def br_curve25519;


void br_ecdsa_i31_bits2int(uint32_t *x,
	const void *src, size_t len, uint32_t ebitlen);


void br_ecdsa_i15_bits2int(uint16_t *x,
	const void *src, size_t len, uint32_t ebitlen);





typedef struct {
	const unsigned char *data;
	size_t len;
	size_t asn1len;
} br_asn1_uint;


br_asn1_uint br_asn1_uint_prepare(const void *xdata, size_t xlen);


size_t br_asn1_encode_length(void *dest, size_t len);


#define len_of_len(len)   br_asn1_encode_length(NULL, len)


size_t br_asn1_encode_uint(void *dest, br_asn1_uint pp);


const unsigned char *br_get_curve_OID(int curve);


size_t br_encode_ec_raw_der_inner(void *dest,
	const br_ec_private_key *sk, const br_ec_public_key *pk,
	int include_curve_oid);





#define BR_SSL_CHANGE_CIPHER_SPEC    20
#define BR_SSL_ALERT                 21
#define BR_SSL_HANDSHAKE             22
#define BR_SSL_APPLICATION_DATA      23


#define BR_SSL_HELLO_REQUEST          0
#define BR_SSL_CLIENT_HELLO           1
#define BR_SSL_SERVER_HELLO           2
#define BR_SSL_CERTIFICATE           11
#define BR_SSL_SERVER_KEY_EXCHANGE   12
#define BR_SSL_CERTIFICATE_REQUEST   13
#define BR_SSL_SERVER_HELLO_DONE     14
#define BR_SSL_CERTIFICATE_VERIFY    15
#define BR_SSL_CLIENT_KEY_EXCHANGE   16
#define BR_SSL_FINISHED              20


#define BR_LEVEL_WARNING   1
#define BR_LEVEL_FATAL     2


#define BR_IO_FAILED   0
#define BR_IO_IN       1
#define BR_IO_OUT      2
#define BR_IO_INOUT    3


void br_ssl_engine_fail(br_ssl_engine_context *cc, int err);


static inline int
br_ssl_engine_closed(const br_ssl_engine_context *cc)
{
	return cc->iomode == BR_IO_FAILED;
}


void br_ssl_engine_new_max_frag_len(
	br_ssl_engine_context *rc, unsigned max_frag_len);


int br_ssl_engine_recvrec_finished(const br_ssl_engine_context *rc);


void br_ssl_engine_flush_record(br_ssl_engine_context *cc);


static inline int
br_ssl_engine_has_pld_to_send(const br_ssl_engine_context *rc)
{
	return rc->oxa != rc->oxb && rc->oxa != rc->oxc;
}


int br_ssl_engine_init_rand(br_ssl_engine_context *cc);


void br_ssl_engine_hs_reset(br_ssl_engine_context *cc,
	void (*hsinit)(void *), void (*hsrun)(void *));


br_tls_prf_impl br_ssl_engine_get_PRF(br_ssl_engine_context *cc, int prf_id);


void br_ssl_engine_compute_master(br_ssl_engine_context *cc,
	int prf_id, const void *pms, size_t len);


void br_ssl_engine_switch_cbc_in(br_ssl_engine_context *cc,
	int is_client, int prf_id, int mac_id,
	const br_block_cbcdec_class *bc_impl, size_t cipher_key_len);


void br_ssl_engine_switch_cbc_out(br_ssl_engine_context *cc,
	int is_client, int prf_id, int mac_id,
	const br_block_cbcenc_class *bc_impl, size_t cipher_key_len);


void br_ssl_engine_switch_gcm_in(br_ssl_engine_context *cc,
	int is_client, int prf_id,
	const br_block_ctr_class *bc_impl, size_t cipher_key_len);


void br_ssl_engine_switch_gcm_out(br_ssl_engine_context *cc,
	int is_client, int prf_id,
	const br_block_ctr_class *bc_impl, size_t cipher_key_len);


void br_ssl_engine_switch_chapol_in(br_ssl_engine_context *cc,
	int is_client, int prf_id);


void br_ssl_engine_switch_chapol_out(br_ssl_engine_context *cc,
	int is_client, int prf_id);


void br_ssl_engine_switch_ccm_in(br_ssl_engine_context *cc,
	int is_client, int prf_id,
	const br_block_ctrcbc_class *bc_impl,
	size_t cipher_key_len, size_t tag_len);


void br_ssl_engine_switch_ccm_out(br_ssl_engine_context *cc,
	int is_client, int prf_id,
	const br_block_ctrcbc_class *bc_impl,
	size_t cipher_key_len, size_t tag_len);


void br_ssl_hs_client_init_main(void *ctx);
void br_ssl_hs_client_run(void *ctx);
void br_ssl_hs_server_init_main(void *ctx);
void br_ssl_hs_server_run(void *ctx);


int br_ssl_choose_hash(unsigned bf);





#if BR_POWER_ASM_MACROS

#define lxvw4x(xt, ra, rb)        lxvw4x_(xt, ra, rb)
#define stxvw4x(xt, ra, rb)       stxvw4x_(xt, ra, rb)

#define bdnz(foo)                 bdnz_(foo)
#define bdz(foo)                  bdz_(foo)
#define beq(foo)                  beq_(foo)

#define li(rx, value)             li_(rx, value)
#define addi(rx, ra, imm)         addi_(rx, ra, imm)
#define cmpldi(rx, imm)           cmpldi_(rx, imm)
#define mtctr(rx)                 mtctr_(rx)
#define vspltb(vrt, vrb, uim)     vspltb_(vrt, vrb, uim)
#define vspltw(vrt, vrb, uim)     vspltw_(vrt, vrb, uim)
#define vspltisb(vrt, imm)        vspltisb_(vrt, imm)
#define vspltisw(vrt, imm)        vspltisw_(vrt, imm)
#define vrlw(vrt, vra, vrb)       vrlw_(vrt, vra, vrb)
#define vsbox(vrt, vra)           vsbox_(vrt, vra)
#define vxor(vrt, vra, vrb)       vxor_(vrt, vra, vrb)
#define vand(vrt, vra, vrb)       vand_(vrt, vra, vrb)
#define vsro(vrt, vra, vrb)       vsro_(vrt, vra, vrb)
#define vsl(vrt, vra, vrb)        vsl_(vrt, vra, vrb)
#define vsldoi(vt, va, vb, sh)    vsldoi_(vt, va, vb, sh)
#define vsr(vrt, vra, vrb)        vsr_(vrt, vra, vrb)
#define vaddcuw(vrt, vra, vrb)    vaddcuw_(vrt, vra, vrb)
#define vadduwm(vrt, vra, vrb)    vadduwm_(vrt, vra, vrb)
#define vsububm(vrt, vra, vrb)    vsububm_(vrt, vra, vrb)
#define vsubuwm(vrt, vra, vrb)    vsubuwm_(vrt, vra, vrb)
#define vsrw(vrt, vra, vrb)       vsrw_(vrt, vra, vrb)
#define vcipher(vt, va, vb)       vcipher_(vt, va, vb)
#define vcipherlast(vt, va, vb)   vcipherlast_(vt, va, vb)
#define vncipher(vt, va, vb)      vncipher_(vt, va, vb)
#define vncipherlast(vt, va, vb)  vncipherlast_(vt, va, vb)
#define vperm(vt, va, vb, vc)     vperm_(vt, va, vb, vc)
#define vpmsumd(vt, va, vb)       vpmsumd_(vt, va, vb)
#define xxpermdi(vt, va, vb, d)   xxpermdi_(vt, va, vb, d)

#define lxvw4x_(xt, ra, rb)       "\tlxvw4x\t" #xt "," #ra "," #rb "\n"
#define stxvw4x_(xt, ra, rb)      "\tstxvw4x\t" #xt "," #ra "," #rb "\n"

#define label(foo)                #foo "%=:\n"
#define bdnz_(foo)                "\tbdnz\t" #foo "%=\n"
#define bdz_(foo)                 "\tbdz\t" #foo "%=\n"
#define beq_(foo)                 "\tbeq\t" #foo "%=\n"

#define li_(rx, value)            "\tli\t" #rx "," #value "\n"
#define addi_(rx, ra, imm)        "\taddi\t" #rx "," #ra "," #imm "\n"
#define cmpldi_(rx, imm)          "\tcmpldi\t" #rx "," #imm "\n"
#define mtctr_(rx)                "\tmtctr\t" #rx "\n"
#define vspltb_(vrt, vrb, uim)    "\tvspltb\t" #vrt "," #vrb "," #uim "\n"
#define vspltw_(vrt, vrb, uim)    "\tvspltw\t" #vrt "," #vrb "," #uim "\n"
#define vspltisb_(vrt, imm)       "\tvspltisb\t" #vrt "," #imm "\n"
#define vspltisw_(vrt, imm)       "\tvspltisw\t" #vrt "," #imm "\n"
#define vrlw_(vrt, vra, vrb)      "\tvrlw\t" #vrt "," #vra "," #vrb "\n"
#define vsbox_(vrt, vra)          "\tvsbox\t" #vrt "," #vra "\n"
#define vxor_(vrt, vra, vrb)      "\tvxor\t" #vrt "," #vra "," #vrb "\n"
#define vand_(vrt, vra, vrb)      "\tvand\t" #vrt "," #vra "," #vrb "\n"
#define vsro_(vrt, vra, vrb)      "\tvsro\t" #vrt "," #vra "," #vrb "\n"
#define vsl_(vrt, vra, vrb)       "\tvsl\t" #vrt "," #vra "," #vrb "\n"
#define vsldoi_(vt, va, vb, sh)   "\tvsldoi\t" #vt "," #va "," #vb "," #sh "\n"
#define vsr_(vrt, vra, vrb)       "\tvsr\t" #vrt "," #vra "," #vrb "\n"
#define vaddcuw_(vrt, vra, vrb)   "\tvaddcuw\t" #vrt "," #vra "," #vrb "\n"
#define vadduwm_(vrt, vra, vrb)   "\tvadduwm\t" #vrt "," #vra "," #vrb "\n"
#define vsububm_(vrt, vra, vrb)   "\tvsububm\t" #vrt "," #vra "," #vrb "\n"
#define vsubuwm_(vrt, vra, vrb)   "\tvsubuwm\t" #vrt "," #vra "," #vrb "\n"
#define vsrw_(vrt, vra, vrb)      "\tvsrw\t" #vrt "," #vra "," #vrb "\n"
#define vcipher_(vt, va, vb)      "\tvcipher\t" #vt "," #va "," #vb "\n"
#define vcipherlast_(vt, va, vb)  "\tvcipherlast\t" #vt "," #va "," #vb "\n"
#define vncipher_(vt, va, vb)     "\tvncipher\t" #vt "," #va "," #vb "\n"
#define vncipherlast_(vt, va, vb) "\tvncipherlast\t" #vt "," #va "," #vb "\n"
#define vperm_(vt, va, vb, vc)    "\tvperm\t" #vt "," #va "," #vb "," #vc "\n"
#define vpmsumd_(vt, va, vb)      "\tvpmsumd\t" #vt "," #va "," #vb "\n"
#define xxpermdi_(vt, va, vb, d)  "\txxpermdi\t" #vt "," #va "," #vb "," #d "\n"

#endif




#if BR_ENABLE_INTRINSICS && (BR_GCC_4_4 || BR_CLANG_3_7 || BR_MSC_2005)


#if BR_i386 || BR_amd64


#if BR_GCC && !BR_GCC_5_0
#if BR_GCC_4_6
#define BR_TARGETS_X86_UP \
	_Pragma("GCC push_options") \
	_Pragma("GCC target(\"sse2,ssse3,sse4.1,aes,pclmul,rdrnd\")")
#define BR_TARGETS_X86_DOWN \
	_Pragma("GCC pop_options")
#else
#define BR_TARGETS_X86_UP \
	_Pragma("GCC target(\"sse2,ssse3,sse4.1,aes,pclmul\")")
#define BR_TARGETS_X86_DOWN
#endif
#pragma GCC diagnostic ignored "-Wpsabi"
#endif

#if BR_CLANG && !BR_CLANG_3_8
#undef __SSE2__
#undef __SSE3__
#undef __SSSE3__
#undef __SSE4_1__
#undef __AES__
#undef __PCLMUL__
#undef __RDRND__
#define __SSE2__     1
#define __SSE3__     1
#define __SSSE3__    1
#define __SSE4_1__   1
#define __AES__      1
#define __PCLMUL__   1
#define __RDRND__    1
#endif

#ifndef BR_TARGETS_X86_UP
#define BR_TARGETS_X86_UP
#endif
#ifndef BR_TARGETS_X86_DOWN
#define BR_TARGETS_X86_DOWN
#endif

#if BR_GCC || BR_CLANG
BR_TARGETS_X86_UP
#include <x86intrin.h>
#include <cpuid.h>
#define br_bswap32   __builtin_bswap32
BR_TARGETS_X86_DOWN
#endif

#if BR_MSC
#include <stdlib.h>
#include <intrin.h>
#include <immintrin.h>
#define br_bswap32   _byteswap_ulong
#endif

static inline int
br_cpuid(uint32_t mask_eax, uint32_t mask_ebx,
	uint32_t mask_ecx, uint32_t mask_edx)
{
#if BR_GCC || BR_CLANG
	unsigned eax, ebx, ecx, edx;

	if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
		if ((eax & mask_eax) == mask_eax
			&& (ebx & mask_ebx) == mask_ebx
			&& (ecx & mask_ecx) == mask_ecx
			&& (edx & mask_edx) == mask_edx)
		{
			return 1;
		}
	}
#elif BR_MSC
	int info[4];

	__cpuid(info, 1);
	if (((uint32_t)info[0] & mask_eax) == mask_eax
		&& ((uint32_t)info[1] & mask_ebx) == mask_ebx
		&& ((uint32_t)info[2] & mask_ecx) == mask_ecx
		&& ((uint32_t)info[3] & mask_edx) == mask_edx)
	{
		return 1;
	}
#endif
	return 0;
}

#endif

#endif



#endif
