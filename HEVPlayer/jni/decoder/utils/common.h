//**********************************************
//common.h
//Unipipy @2011
//Common header of utils
//**********************************************


#ifndef COMMON_H
#define COMMON_H

#ifdef _DEBUG
#include <stdio.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "attributes.h"


#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifndef INT64_C
#define INT64_C(c)     (c ## LL)
#define UINT64_C(c)    (c ## ULL)
#endif


#define LENTABS(a) ((a) >= 0 ? (a) : (-(a)))
static lent_always_inline int LENTABS_INT(int a)
{
	return LENTABS(a);
}

#define LENTMAX(a,b) ((a) > (b) ? (a) : (b))
#define LENTMAX3(a,b,c) FFMAX(FFMAX(a,b),c)
#define LENTMIN(a,b) ((a) > (b) ? (b) : (a))
#define LENTMIN3(a,b,c) FFMIN(FFMIN(a,b),c)

static lent_always_inline int LENTMIN_INT(int a,int b)
{
	return LENTMIN(a,b);
}

#define LENTSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)
#define lent_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#define LENTALIGN(x, a) (((x)+(a)-1)&~((a)-1))

typedef union { uint16_t i; uint8_t  c[2]; } LENT_union16_t;
typedef union { uint32_t i; uint16_t b[2]; uint8_t  c[4]; } LENT_union32_t;
typedef union { uint64_t i; uint32_t a[2]; uint16_t b[4]; uint8_t c[8]; } LENT_union64_t;
typedef struct { uint64_t i[2]; } LENT_uint128_t;
typedef union { LENT_uint128_t i; uint64_t a[2]; uint32_t b[4]; uint16_t c[8]; uint8_t d[16]; } LENT_union128_t;
#define M16(src) (((LENT_union16_t*)(src))->i)
#define M32(src) (((LENT_union32_t*)(src))->i)
#define M64(src) (((LENT_union64_t*)(src))->i)
#define M128(src) (((LENT_union128_t*)(src))->i)
#define CP16(dst,src) M16(dst) = M16(src)
#define CP32(dst,src) M32(dst) = M32(src)
#define CP64(dst,src) M64(dst) = M64(src)
#define CP128(dst,src) M128(dst) = M128(src)
/* misc math functions */
extern const uint8_t lent_log2_tab[256];

extern const uint8_t lent_reverse[256];

static lent_always_inline int lent_log2_c(unsigned int v)
{
#if ARCH_X86_32
	return 32-__lzcnt(v);
#else
    int n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += lent_log2_tab[v];
    return n;
#endif
}

#include "config.h"

#include "common.h"

static lent_always_inline int lent_clip_c(int a, int amin, int amax)
{
    if      (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}

static lent_always_inline uint8_t lent_clip_uint8_c(int a)
{
    if (a&(~0xFF)) return (-a)>>31;
    else           return a;
}

static lent_always_inline int8_t lent_clip_int8_c(int a)
{
    if ((a+0x80) & ~0xFF) return (a>>31) ^ 0x7F;
    else                  return a;
}

#include "mem.h"
#include "internal.h"

#endif /* COMMON_H */

#ifndef lent_log2
#   define lent_log2       lent_log2_c
#endif
#ifndef lent_log2_16bit
#   define lent_log2_16bit lent_log2_16bit_c
#endif
#ifndef lent_ceil_log2
#   define lent_ceil_log2     lent_ceil_log2_c
#endif
#ifndef lent_clip
#   define lent_clip          lent_clip_c
#endif
#ifndef lent_clip_uint8
#   define lent_clip_uint8    lent_clip_uint8_c
#endif
#ifndef lent_clip_int8
#   define lent_clip_int8     lent_clip_int8_c
#endif
#ifndef lent_clip_uint16
#   define lent_clip_uint16   lent_clip_uint16_c
#endif
#ifndef lent_clip_int16
#   define lent_clip_int16    lent_clip_int16_c
#endif
#ifndef lent_clipl_int32
#   define lent_clipl_int32   lent_clipl_int32_c
#endif
#ifndef lent_clipf
#   define lent_clipf         lent_clipf_c
#endif
#ifndef lent_popcount
#   define lent_popcount      lent_popcount_c
#endif
