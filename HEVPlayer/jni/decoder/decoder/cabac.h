//**********************************************
//cabac.h
//Unipipy @2011
//cabac engine.need modification
//**********************************************


#ifndef DECODER_CABAC_H
#define DECODER_CABAC_H

#define MAX_CABAC_STATE_COMMON 250
#define CABAC_BITS 16
#define CABAC_MASK ((1<<CABAC_BITS)-1)
#define BRANCHLESS_CABAC_DECODER 1
//#define ARCH_X86_DISABLED 1

typedef struct CABACContext{
    int low;
    int range;
    int outstanding_count;

    const uint8_t	*bytestream_start;
    const uint8_t	*bytestream;
    const uint8_t	*bytestream_end;
	uint8_t			 cabac_state[MAX_CABAC_STATE_COMMON];
}CABACContext;

extern uint8_t lent_hevc_mlps_state[4*64];
extern uint8_t lent_hevc_lps_range[4*2*64];  ///< rangeTabLPS
extern uint8_t lent_hevc_mps_state[2*64];     ///< transIdxMPS
extern uint8_t lent_hevc_lps_state[2*64];     ///< transIdxLPS
extern const uint8_t lent_hevc_norm_shift[512];


void lent_init_cabac_encoder(CABACContext *c, uint8_t *buf, int buf_size);
void lent_init_cabac_decoder(CABACContext *c, const uint8_t *buf, int buf_size);
void lent_init_cabac_states(CABACContext *c);


static void refill(CABACContext *c){
#if CABAC_BITS == 16
        c->low+= (c->bytestream[0]<<9) + (c->bytestream[1]<<1);
#else
        c->low+= c->bytestream[0]<<1;
#endif
    c->low -= CABAC_MASK;
    c->bytestream+= CABAC_BITS/8;
}

#if ! ( ARCH_X86 && HAVE_7REGS && HAVE_EBX_AVAILABLE && !defined(BROKEN_RELOCATIONS) )
static void refill2(CABACContext *c){
    int i, x;

    x= c->low ^ (c->low-1);
    i= 7 - lent_hevc_norm_shift[x>>(CABAC_BITS-1)];

    x= -CABAC_MASK;

#if CABAC_BITS == 16
        x+= (c->bytestream[0]<<9) + (c->bytestream[1]<<1);
#else
        x+= c->bytestream[0]<<1;
#endif

    c->low += x<<i;
    c->bytestream+= CABAC_BITS/8;
}
#endif

static inline void renorm_cabac_decoder(CABACContext *c){
    while(c->range < 0x100){
        c->range+= c->range;
        c->low+= c->low;
        if(!(c->low & CABAC_MASK))
            refill(c);
    }
}

static inline void renorm_cabac_decoder_once(CABACContext *c){
#ifdef ARCH_X86_DISABLED
    int temp;
#if 0
    //P3:683    athlon:475
    __asm__(
        "lea -0x100(%0), %2         \n\t"
        "shr $31, %2                \n\t"  //FIXME 31->63 for x86-64
        "shl %%cl, %0               \n\t"
        "shl %%cl, %1               \n\t"
        : "+r"(c->range), "+r"(c->low), "+c"(temp)
    );
#elif 0
    //P3:680    athlon:474
    __asm__(
        "cmp $0x100, %0             \n\t"
        "setb %%cl                  \n\t"  //FIXME 31->63 for x86-64
        "shl %%cl, %0               \n\t"
        "shl %%cl, %1               \n\t"
        : "+r"(c->range), "+r"(c->low), "+c"(temp)
    );
#elif 1
    int temp2;
    //P3:665    athlon:517
    __asm__(
        "lea -0x100(%0), %%eax      \n\t"
        "cltd                       \n\t"
        "mov %0, %%eax              \n\t"
        "and %%edx, %0              \n\t"
        "and %1, %%edx              \n\t"
        "add %%eax, %0              \n\t"
        "add %%edx, %1              \n\t"
        : "+r"(c->range), "+r"(c->low), "+a"(temp), "+d"(temp2)
    );
#elif 0
    int temp2;
    //P3:673    athlon:509
    __asm__(
        "cmp $0x100, %0             \n\t"
        "sbb %%edx, %%edx           \n\t"
        "mov %0, %%eax              \n\t"
        "and %%edx, %0              \n\t"
        "and %1, %%edx              \n\t"
        "add %%eax, %0              \n\t"
        "add %%edx, %1              \n\t"
        : "+r"(c->range), "+r"(c->low), "+a"(temp), "+d"(temp2)
    );
#else
    int temp2;
    //P3:677    athlon:511
    __asm__(
        "cmp $0x100, %0             \n\t"
        "lea (%0, %0), %%eax        \n\t"
        "lea (%1, %1), %%edx        \n\t"
        "cmovb %%eax, %0            \n\t"
        "cmovb %%edx, %1            \n\t"
        : "+r"(c->range), "+r"(c->low), "+a"(temp), "+d"(temp2)
    );
#endif
#else
    //P3:675    athlon:476
    int shift= (uint32_t)(c->range - 0x100)>>31;
    c->range<<= shift;
    c->low  <<= shift;
#endif
    if(!(c->low & CABAC_MASK))
        refill(c);
}

static lent_always_inline int get_cabac_inline(CABACContext *c, uint8_t * const state){
    //FIXME gcc generates duplicate load/stores for c->low and c->range
#if ARCH_X86_64
#define BYTESTART   "16"
#define BYTE        "24"
#define BYTEEND     "32"
#else
#define BYTESTART   "12"
#define BYTE        16
#define BYTEEND     "20"
#endif
//#if ARCH_X86 && HAVE_7REGS && HAVE_EBX_AVAILABLE && !defined(BROKEN_RELOCATIONS)
#if 0
    int bit;
	__asm{
		mov edi,c

		//ebx=low
		mov ebx,[edi]

		//esi=range
		mov esi,[edi+4]


		push edi
		//bit=*state
		mov edi,state
		movzx eax,byte ptr [edi]

		//edx=range
		mov edx,esi

		//range&=0xC0	临时range
		and esi,0xC0

		//esi=RangeLPS=lent_hevc_lps_range[range*2+bit]	更新RangeLPS
		movzx esi,byte ptr[lent_hevc_lps_range+eax+esi*2]

		//range-=RangeLPS	此时edx为未保存的c->range
		sub edx,esi

		//ecx=c->range
		mov ecx,edx

		//edx=c->range<<(CABAC_BITS+1)
		shl edx,17

		//range<<(CABAC_BITS+1)-c->low
		cmp edx,ebx

		//esi=c->range += (RangeLPS - c->range) & lps_mask
		cmova esi,ecx

		//ecx=lps_mask=((c->range<<(CABAC_BITS+1)) - c->low)>>31 利用cf填充
		sbb ecx,ecx

		//edx=c->range<<(CABAC_BITS+1)&lps_mask
		and edx,ecx

		//low-=c->range<<(CABAC_BITS+1)&lps_mask
		sub ebx,edx

		//bit^=lps_mask
		xor eax,ecx

		//ecx= lps_mask=lent_hevc_norm_shift[c->range];
		movzx ecx,byte ptr[lent_hevc_norm_shift+esi]

		//edx=(lent_hevc_mlps_state+128)[s];
		movzx edx,byte ptr[lent_hevc_mlps_state+eax+128]

		//range<<=lps_mask
		shl esi,cl

		//low<<=lps_mask
		shl ebx,cl
	
		//*state=(lent_hevc_mlps_state+128)[s]
		mov byte ptr[edi],dl



		//if(!(c->low & CABAC_MASK))
		test bx,bx

		jnz flag

		//begin refill2
		pop edi
		push edi

		//edx=bytestream[3:2:1:0]
		mov ecx,dword ptr [edi+BYTE]
		movzx edx,word ptr [ecx]

		//edx=bytestream[0:1:2:3]
		bswap edx

		//edx=x=(c->bytestream[0]<<9) + (c->bytestream[1]<<1)
		shr edx,15

		//x-=CABAC_MASK
		sub edx,0xFFFF

		//c->bytestream+= CABAC_BITS/8
		add ecx,2
		mov dword ptr[edi+BYTE],ecx

		//ecx=x=low^(low-1)
		lea ecx,[ebx-1]
		xor ecx,ebx

		//ecx=x>>15
		shr ecx,15

		//ecx=i=7-lent_hevc_norm_shift[x>>(CABAC_BITS-1)];
		movzx ecx,[lent_hevc_norm_shift+ecx]
		neg ecx
		add ecx,7

		//edx=x<<i 假设edx等于x
		shl edx,cl

		//low+=x<<i
		add ebx,edx

		flag:


		pop edi
		mov [edi+4],esi
		mov [edi],ebx
		mov [bit],eax
	};
    bit&=1;
#else /* ARCH_X86 && HAVE_7REGS && HAVE_EBX_AVAILABLE && !defined(BROKEN_RELOCATIONS) */
    int s = *state;
    int RangeLPS= lent_hevc_lps_range[2*(c->range&0xC0) + s];
    int bit, lps_mask;
    c->range -= RangeLPS;

    lps_mask= ((c->range<<(CABAC_BITS+1)) - c->low)>>31;

    c->low -= (c->range<<(CABAC_BITS+1)) & lps_mask;
    c->range += (RangeLPS - c->range) & lps_mask;

    s^=lps_mask;
    *state= (lent_hevc_mlps_state+128)[s];
    bit= s&1;

    lps_mask= lent_hevc_norm_shift[c->range];
    c->range<<= lps_mask;
    c->low  <<= lps_mask;
    if(!(c->low & CABAC_MASK))
        refill2(c);
	{
#if 0
#undef fprintf
		FILE *fp=fopen("logdec.txt","a");
		fprintf(fp,"\t\trange %d\n",c->range);
		fclose(fp);
#endif
	}
#endif
    return bit;
}

static int  get_cabac_noinline(CABACContext *c, uint8_t * const state){
    return get_cabac_inline(c,state);
}

static int  get_cabac(CABACContext *c, uint8_t * const state){
    return get_cabac_inline(c,state);
}

static int  get_cabac_bypass(CABACContext *c){
    int mask,range;

#if 0//还有问题
	__asm{

		pusha

		mov edi,c

		//ebx=low
		mov ebx,[edi]
		//esi=range
		mov esi,[edi+4]

		shl ebx,1

		test ebx,0xffff
		jne flag


		//edx=bytestream[3:2:1:0]
		mov ecx,dword ptr [edi+BYTE]
		movzx edx,word ptr [ecx]

		//edx=bytestream[0:1:2:3]
		bswap edx

		//edx=(c->bytestream[0]<<9) + (c->bytestream[1]<<1)
		shr edx,15

		add ebx,edx

		//low-=CABAC_MASK
		sub ebx,0xFFFF

		//c->bytestream+= CABAC_BITS/8
		add ecx,2
		mov dword ptr[edi+BYTE],ecx

flag:
		shl esi,17
		sub ebx,esi
		mov eax,ebx
		sar eax,31
		and	esi,eax
		add	ebx,esi

		//写回low
		mov [edi],ebx

		mov [mask],eax
		popa
		ret

	}
#else
    c->low += c->low;

    if(!(c->low & CABAC_MASK))
        refill(c);

    range= c->range<<(CABAC_BITS+1);
	c->low -= range;
    mask= c->low >> 31;
    range &= mask;
    c->low += range;
#endif
	return !(mask&1);
}


static lent_always_inline int get_cabac_bypass_sign(CABACContext *c, int val){
#if ARCH_X86 && HAVE_EBX_AVAILABLE
    __asm__ volatile(
        "movl "RANGE    "(%1), %%ebx            \n\t"
        "movl "LOW      "(%1), %%eax            \n\t"
        "shl $17, %%ebx                         \n\t"
        "add %%eax, %%eax                       \n\t"
        "sub %%ebx, %%eax                       \n\t"
        "cltd                                   \n\t"
        "and %%edx, %%ebx                       \n\t"
        "add %%ebx, %%eax                       \n\t"
        "xor %%edx, %%ecx                       \n\t"
        "sub %%edx, %%ecx                       \n\t"
        "test %%ax, %%ax                        \n\t"
        " jnz 1f                                \n\t"
        "mov  "BYTE     "(%1), %%"REG_b"        \n\t"
        "subl $0xFFFF, %%eax                    \n\t"
        "movzwl (%%"REG_b"), %%edx              \n\t"
        "bswap %%edx                            \n\t"
        "shrl $15, %%edx                        \n\t"
        "add  $2, %%"REG_b"                     \n\t"
        "addl %%edx, %%eax                      \n\t"
        "mov  %%"REG_b", "BYTE     "(%1)        \n\t"
        "1:                                     \n\t"
        "movl %%eax, "LOW      "(%1)            \n\t"

        :"+c"(val)
        :"r"(c)
        : "%eax", "%"REG_b, "%edx", "memory"
    );
    return val;
#else
    int range, mask;
    c->low += c->low;

    if(!(c->low & CABAC_MASK))
        refill(c);

    range= c->range<<(CABAC_BITS+1);
    c->low -= range;
    mask= c->low >> 31;
    range &= mask;
    c->low += range;
    return (val^mask)-mask;
#endif
}


static int get_cabac_ue_bypass(CABACContext *c,int exp)
{
#if 0
	int i=0;
	int32_t value=0;
	while(get_cabac_bypass(c))
		i++;
	value=1;
	while(i--)
		value+=value+get_cabac_bypass(c);
	return value-1;
#else
	int value=0;
	
	while( get_cabac_bypass( c ) ) {
		value += 1 << exp;
		exp++;
	}
	while( exp-- ) {
		value += get_cabac_bypass( c )<<exp;
	}
	return value;
#endif

}

/**
 *
 * @return the number of bytes read or 0 if no end
 */
static int  get_cabac_terminate(CABACContext *c){
    c->range -= 2;
    if(c->low < c->range<<(CABAC_BITS+1)){
        renorm_cabac_decoder_once(c);
        return 0;
    }else{
        return c->bytestream - c->bytestream_start;
    }
}

#endif