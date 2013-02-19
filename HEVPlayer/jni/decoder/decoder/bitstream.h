//**********************************************
//bitstream.h
//Unipipy @2011
//Bitstream reader
//TODO:ASM optimization
//**********************************************

#ifndef DECODER_BITSTREAM_H
#define DECODER_BITSTREAM_H

#define lent_INPUT_BUFFER_PADDING_SIZE 16

#define WORD_GET_HIGH(num,n)				((uint32_t)num>>(32-n))

typedef struct BitstreamContext {
    const uint8_t *buffer, *buffer_end;

    uint32_t *buffer_ptr;
    uint32_t cache0;
    uint32_t cache1;
    int bit_count;

    int size_in_bits;
} BitstreamContext;


/////////////////////////////////////////////////////////////////////////////////////////////////
//字节序
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef LITTLE_ENDIAN
#define bswap32(x) lent_bswap32(x)
#define bswap16(x) lent_bswap16(x)
#else
#define bswap32(x) (x)
#define bswap16(x) (x)
#endif

static lent_always_inline  uint16_t lent_bswap16(uint16_t x)
{
    x= (x>>8) | (x<<8);
    return x;
}
static lent_always_inline  uint32_t lent_bswap32(uint32_t x)
{
    x= ((x<<8)&0xFF00FF00) | ((x>>8)&0x00FF00FF);
    x= (x>>16) | (x<<16);
    return x;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//读取相关宏操作
/////////////////////////////////////////////////////////////////////////////////////////////////

#define LOAD_READER(name, bs) \
		uint32_t* name##_buffer_ptr	=(bs)->buffer_ptr;\
		uint32_t name##_cache0		=(bs)->cache0;\
		uint32_t name##_cache1		=(bs)->cache1;\
		int name##_bit_count		=(bs)->bit_count

#define SAVE_READER(name, bs) do{\
		(bs)->buffer_ptr			=name##_buffer_ptr;\
		(bs)->cache0				=name##_cache0;\
		(bs)->cache1				=name##_cache1;\
		(bs)->bit_count				=name##_bit_count;\
	}while(0)

#define UPDATE_CACHE(name, bs) do{\
	if(name##_bit_count>0){\
		uint32_t nextword			=bswap32(*name##_buffer_ptr);\
		name##_cache0				|=WORD_GET_HIGH(nextword,name##_bit_count);\
		name##_cache1				|=nextword<<name##_bit_count;\
		name##_buffer_ptr			++;\
		name##_bit_count			-=32;\
	}\
	}while(0)

#define GET_BITS(name, bs, n)		(name##_cache0>>(32-(n)))
#define SKIP_BITS(name, bs,n) do{\
	name##_cache0					<<=(n);\
	name##_cache0					|=WORD_GET_HIGH(name##_cache1,(n));\
	name##_cache1					<<=(n);\
	name##_bit_count				+=(n);\
	}while(0)

#define GET_CACHE(name, bs)			(name##_cache0)


/////////////////////////////////////////////////////////////////////////////////////////////////
//读取相关函数
/////////////////////////////////////////////////////////////////////////////////////////////////

static lent_always_inline void skip_bits(BitstreamContext *bs,int n)
{
	LOAD_READER(tmp, bs);
    tmp_bit_count += n;
    tmp_buffer_ptr += tmp_bit_count>>5;
    tmp_bit_count &= 31;
    tmp_cache0 = bswap32(tmp_buffer_ptr[-1]) << tmp_bit_count;
    tmp_cache1 = 0;
    UPDATE_CACHE(tmp, bs);
    SAVE_READER(tmp, bs);
}

static lent_always_inline void init_get_bits(BitstreamContext *bs,
                   const uint8_t *buffer, int bit_size)
{
    int buffer_size = (bit_size+7)>>3;
    if (buffer_size < 0 || bit_size < 0) {
        buffer_size = bit_size = 0;
        buffer = NULL;
    }

    bs->buffer       = buffer;
    bs->size_in_bits = bit_size;
    bs->buffer_end   = buffer + buffer_size;
    bs->buffer_ptr   = (uint32_t*)((intptr_t)buffer & ~3);
    bs->bit_count    = 32 +     8*((intptr_t)buffer &  3);
    skip_bits(bs, 0);
}

static lent_always_inline unsigned int get_bits(BitstreamContext *bs, int n){
    register int tmp;

	LOAD_READER(tmp, bs);
    UPDATE_CACHE(tmp, bs);
	tmp=GET_BITS(tmp, bs, n);
	SKIP_BITS(tmp, bs, n);
    SAVE_READER(tmp, bs);

    return tmp;
}

static lent_always_inline unsigned int get_ue(BitstreamContext *bs){
    register unsigned int tmp;
	register int log;

	LOAD_READER(tmp, bs);
    UPDATE_CACHE(tmp, bs);
	tmp=GET_CACHE(tmp, bs);

        log= 2*lent_log2(tmp) - 31;
        tmp>>= log;
        tmp--;
        SKIP_BITS(tmp, bs, 32 - log);
        SAVE_READER(tmp, bs);

    SAVE_READER(tmp, bs);

    return tmp;
}
static lent_always_inline unsigned int get_se(BitstreamContext *bs){
    register int tmp;

	tmp=get_ue(bs);
	if(tmp&1)
		return (tmp+1)>>1;
	return -(tmp>>1);
	

    //return tmp;
}

static lent_always_inline int get_bits_count(const BitstreamContext *bs) {
    return ((uint8_t*)bs->buffer_ptr - bs->buffer)*8 - 32 + bs->bit_count;
}
static lent_always_inline int get_bits_left(BitstreamContext *bs)
{
    return bs->size_in_bits - get_bits_count(bs);
}

static inline void align_get_bits(BitstreamContext *bs)
{
    int n = -get_bits_count(bs) & 7;
    if (n) skip_bits(bs, n);
}

#define get_1bit(pbs)	get_bits(pbs,1)
#define get_8bits(pbs)	get_bits(pbs,8)
#define get_16bits(pbs)	get_bits(pbs,16)



#endif