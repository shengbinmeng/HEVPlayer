//**********************************************
//log.h
//Unipipy @2011
//Logging functions of the decoder
//**********************************************


#ifndef UTILS_LOG_H
#define UTILS_LOG_H

#include <stdarg.h>
#include "utils.h"

/**
 * Describe the class of an AVClass context structure. That is an
 * arbitrary struct of which the first field is a pointer to an
 * AVClass struct (e.g. AVCodecContext, AVFormatContext etc.).
 */
typedef struct {
    /**
     * The name of the class; usually it is the same name as the
     * context structure type to which the AVClass is associated.
     */
    const char* class_name;

    /**
     * A pointer to a function which returns the name of a context
     * instance ctx associated with the class.
     */
    const char* (*item_name)(void* ctx);

    /**
     * a pointer to the first option specified in the class if any or NULL
     *
     * @see av_set_default_options()
     */
    const struct AVOption *option;

    /**
     * LIBAVUTIL_VERSION with which this structure was created.
     * This is used to allow fields to be added without requiring major
     * version bumps everywhere.
     */

    int version;

    /**
     * Offset in the structure where log_level_offset is stored.
     * 0 means there is no such variable
     */
    int log_level_offset_offset;

    /**
     * Offset in the structure where a pointer to the parent context for loging is stored.
     * for example a decoder that uses eval.c could pass its AVCodecContext to eval as such
     * parent context. And a av_log() implementation could then display the parent context
     * can be NULL of course
     */
    int parent_log_context_offset;
} LentClass;

/* av_log API */

#define LENT_LOG_QUIET    -8

/**
 * Something went really wrong and we will crash now.
 */
#define LENT_LOG_PANIC     0

/**
 * Something went wrong and recovery is not possible.
 * For example, no header was found for a format which depends
 * on headers or an illegal combination of parameters is used.
 */
#define LENT_LOG_FATAL     8

/**
 * Something went wrong and cannot losslessly be recovered.
 * However, not all future data is affected.
 */
#define LENT_LOG_ERROR    16

/**
 * Something somehow does not look correct. This may or may not
 * lead to problems. An example would be the use of '-vstrict -2'.
 */
#define LENT_LOG_WARNING  24

#define LENT_LOG_INFO     32
#define LENT_LOG_VERBOSE  40

/**
 * Stuff which is only useful for libav* developers.
 */
#define LENT_LOG_DEBUG    48

/**
 * Send the specified message to the log if the level is less than or equal
 * to the current av_log_level. By default, all logging messages are sent to
 * stderr. This behavior can be altered by setting a different av_vlog callback
 * function.
 *
 * @param avcl A pointer to an arbitrary struct of which the first field is a
 * pointer to an AVClass struct.
 * @param level The importance level of the message, lower values signifying
 * higher importance.
 * @param fmt The format string (printf-compatible) that specifies how
 * subsequent arguments are converted to output.
 * @see av_vlog
 */
#ifdef __GNUC__
void lent_log(void *avcl, int level, const char *fmt, ...) __attribute__ ((__format__ (__printf__, 3, 4)));
#else
void lent_log(void *avcl, int level, const char *fmt, ...);
#endif

void lent_vlog(void *avcl, int level, const char *fmt, va_list);
int lent_log_get_level(void);
void lent_log_set_level(int);
void lent_log_set_callback(void (*)(void*, int, const char*, va_list));
void lent_log_default_callback(void* ptr, int level, const char* fmt, va_list vl);
const char* lent_default_item_name(void* ctx);

/**
 * av_dlog macros
 * Useful to print debug messages that shouldn't get compiled in normally.
 */

#if defined(DEBUG) || defined(_DEBUG)
#    define lent_dlog(pctx, ...) lent_log(pctx, LENT_LOG_DEBUG, __VA_ARGS__)
#else
#    define lent_dlog(pctx, ...)
#endif

/**
 * Skip repeated messages, this requires the user app to use av_log() instead of
 * (f)printf as the 2 would otherwise interfere and lead to
 * "Last message repeated x times" messages below (f)printf messages with some
 * bad luck.
 * Also to receive the last, "last repeated" line if any, the user app must
 * call av_log(NULL, AV_LOG_QUIET, ""); at the end
 */
#define AV_LOG_SKIP_REPEATED 1
void lent_log_set_flags(int arg);



#endif