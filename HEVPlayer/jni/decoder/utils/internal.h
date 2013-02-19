//**********************************************
//internal.h
//Unipipy @2011
//
//**********************************************


#ifndef UTILS_INTERNAL_H
#define UTILS_INTERNAL_H

#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "config.h"
#include "attributes.h"


#define malloc please_use_lent_malloc
#undef  free
#define free please_use_lent_free
#undef  realloc
#define realloc please_use_lent_realloc

#undef  sprintf
#define sprintf sprintf_is_forbidden_due_to_security_issues_use_snprintf
#undef  printf
#define printf please_use_lent_log_instead_of_printf
#undef  fprintf
#define fprintf please_use_lent_log_instead_of_fprintf
#undef  puts
#define puts please_use_lent_log_instead_of_puts


#define LENT_ALLOC_OR_GOTO(ctx, p, size, label)\
{\
    p = lent_malloc(size);\
    if (p == NULL && (size) != 0) {\
        lent_log(ctx, LENT_LOG_ERROR, "Cannot allocate memory.\n");\
        goto label;\
    }\
}

#define LENT_ALLOCZ_OR_GOTO(ctx, p, size, label)\
{\
    p = lent_mallocz(size);\
    if (p == NULL && (size) != 0) {\
        lent_log(ctx, LENT_LOG_ERROR, "Cannot allocate memory.\n");\
        goto label;\
    }\
}

#define LENT_ALLOCN_OR_GOTO(ctx, p, size, label)\
{\
    p = lent_mallocn(size);\
    if (p == NULL && (size) != 0) {\
        lent_log(ctx, LENT_LOG_ERROR, "Cannot allocate memory.\n");\
        goto label;\
    }\
}

#endif /* AVUTIL_INTERNAL_H */
