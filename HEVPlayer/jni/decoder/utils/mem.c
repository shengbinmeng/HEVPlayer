//**********************************************
//mem.c
//Unipipy @2011
//Memory related functions of the decoder
//**********************************************

#include "config.h"
#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "utils.h"
#include "mem.h"
#include "memory.h"


#undef free
#undef malloc
#undef realloc





void *lent_malloc(int32_t size)
{
    char *ptr = NULL,*p;
    char diff;

    if(size > (INT_MAX-16) || size<=0 )
        return NULL;

    ptr = (char*)malloc(size+16);
    if(!ptr)
        return ptr;
	p=ptr+16;
	p=(char*)((uintptr_t)p&~0xf);
    diff= p-ptr;
    p[-1]= diff;
	lent_dlog(NULL,"Mem malloc\n");
    return p;
}

void *lent_realloc(void *ptr, int32_t size)
{
    int32_t diff;

    if(size > (INT_MAX-16) )
        return NULL;

    if(!ptr) return lent_malloc(size);
    diff= ((char*)ptr)[-1];
    return (char*)realloc((char*)ptr - diff, size + diff) + diff;
}

void lent_free(void *ptr)
{
    if (ptr)
        free((char*)ptr - ((char*)ptr)[-1]);
}

void lent_fast_malloc(void **ptr,int *oldsize,int newsize)
{
	if(newsize<=*oldsize)
		return;
	lent_freep(ptr);
	*ptr=lent_malloc(newsize=newsize+32);
	if(!*ptr)
		newsize=0;
	*oldsize=newsize;
}