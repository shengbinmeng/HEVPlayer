//**********************************************
//log.c
//Unipipy @2011
//Logging functions of the decoder
//**********************************************

#ifndef _MSC_VER//pipy for MSVC
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "log.h"

static int lent_log_level = LENT_LOG_INFO;
static int flags;

#if defined(_WIN32) && !defined(__MINGW32CE__)
#pragma warning(disable:4996)
#endif

static int use_color=-1;

#undef fprintf

static void colored_fputs(int level, const char *str){
    fputs(str, stderr);
}

const char* lent_default_item_name(void* ptr){
    return (*(LentClass**)ptr)->class_name;
}

void lent_log_default_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    static int print_prefix=1;
    static int count;
    static char line[1024], prev[1024];
    static int is_atty;
    LentClass* avc= ptr ? *(LentClass**)ptr : NULL;
    if(level>lent_log_level)
        return;
    line[0]=0;
#undef fprintf
    if(print_prefix && avc) {
        if(avc->version >= (50<<16 | 15<<8 | 3) && avc->parent_log_context_offset){
            LentClass** parent= *(LentClass***)(((uint8_t*)ptr) + avc->parent_log_context_offset);
            if(parent && *parent){
                snprintf(line, sizeof(line), "[%s @ %p] ", (*parent)->item_name(parent), parent);
            }
        }
        snprintf(line + strlen(line), sizeof(line) - strlen(line), "[%s @ %p] ", avc->item_name(ptr), ptr);
    }

    vsnprintf(line + strlen(line), sizeof(line) - strlen(line), fmt, vl);

    print_prefix= line[strlen(line)-1] == '\n';

#if HAVE_ISATTY
    if(!is_atty) is_atty= isatty(2) ? 1 : -1;
#endif

    if(print_prefix && (flags & AV_LOG_SKIP_REPEATED) && !strcmp(line, prev)){
        count++;
        if(is_atty==1)
            fprintf(stderr, "    Last message repeated %d times\r", count);
        return;
    }
    if(count>0){
        fprintf(stderr, "    Last message repeated %d times\n", count);
        count=0;
    }
    colored_fputs(lent_clip(level>>3, 0, 6), line);
    strcpy(prev, line);
}

static void (*lent_log_callback)(void*, int, const char*, va_list) = lent_log_default_callback;

void lent_log(void* avcl, int level, const char *fmt, ...)
{
    LentClass* avc= avcl ? *(LentClass**)avcl : NULL;
    va_list vl;
    va_start(vl, fmt);
    if(avc && avc->version >= (50<<16 | 15<<8 | 2) && avc->log_level_offset_offset && level>=LENT_LOG_FATAL)
        level += *(int*)(((uint8_t*)avcl) + avc->log_level_offset_offset);
    lent_vlog(avcl, level, fmt, vl);
    va_end(vl);
}

void lent_vlog(void* avcl, int level, const char *fmt, va_list vl)
{
    lent_log_callback(avcl, level, fmt, vl);
}

int lent_log_get_level(void)
{
    return lent_log_level;
}

void lent_log_set_level(int level)
{
    lent_log_level = level;
}

void lent_log_set_flags(int arg)
{
    flags= arg;
}

void lent_log_set_callback(void (*callback)(void*, int, const char*, va_list))
{
    lent_log_callback = callback;
}
