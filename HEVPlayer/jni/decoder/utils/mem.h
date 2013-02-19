//**********************************************
//mem.h
//Unipipy @2011
//Memory related functions of the decoder
//**********************************************


#ifndef UTILS_MEM_H
#define UTILS_MEM_H


#if defined(__ICC) && _ICC < 1200 || defined(__SUNPRO_C)//Intel Compiler
    #define DECLARE_ALIGNED(n,t,v)      t __attribute__ ((aligned (n))) v
    #define DECLARE_ASM_CONST(n,t,v)    const t __attribute__ ((aligned (n))) v

#elif defined(__TI_COMPILER_VERSION__)//TI Compiler
    #define DECLARE_ALIGNED(n,t,v)                      \
        AV_PRAGMA(DATA_ALIGN(v,n))                      \
        t __attribute__((aligned(n))) v
    #define DECLARE_ASM_CONST(n,t,v)                    \
        AV_PRAGMA(DATA_ALIGN(v,n))                      \
        static const t __attribute__((aligned(n))) v

#elif defined(__GNUC__)//GCC
    #define DECLARE_ALIGNED(n,t,v)      t __attribute__ ((aligned (n))) v
    #define DECLARE_ASM_CONST(n,t,v)    static const t av_used __attribute__ ((aligned (n))) v

#elif defined(_MSC_VER)//MS VC
    #define DECLARE_ALIGNED(n,t,v)      __declspec(align(n)) t v
    #define DECLARE_ASM_CONST(n,t,v)    __declspec(align(n)) static const t v

#else//default
    #define DECLARE_ALIGNED(n,t,v)      t v
    #define DECLARE_ASM_CONST(n,t,v)    static const t v

#endif

/*
#define STACK_ALIGN(align, type, name, size)\
	char name ## _stack_align_buf[size*sizeof(type)+align-1]; \
	type *name = (type *)(((unsigned long)(name##_stack_align_buf+(align-1)))&~(align-1));
*/
#define STACK_ALIGN(align, type, name, size) DECLARE_ALIGNED(16,type,name)[size];

void*	lent_malloc(int size);
void	lent_free(void *p);
void	lent_fast_malloc(void **ptr,int *oldsize,int newsize);

static lent_always_inline void lent_freep(void **p)
{
	if(!*p)
		return;
    lent_free(*p);
    *p = NULL;
}

static lent_always_inline void lent_mallocp(void **p,int size)
{
	if(*p)
		return;
	*p=lent_malloc(size);
}
static lent_always_inline void *lent_mallocz(int32_t size)
{
	void *p;
	p=lent_malloc(size);
	if(p)
		memset(p,0,size);
	return p;
}
static lent_always_inline void *lent_mallocn(int32_t size)
{
	void *p;
	p=lent_malloc(size);
	if(p)
		memset(p,-1,size);
	return p;
}


#if ARCH_X86_32
void lent_memset_zero_32(void *p,int size);
void lent_memset(void *p,unsigned char data,int size);
#endif
#endif