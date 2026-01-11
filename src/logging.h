/**
 * logging.h
 * Logging and log reporting utility.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_LOGGING_H
#define _GL_LOGGING_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Log levels. */
typedef enum {
	LOG_CRIT = 0,
	LOG_ERROR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO
} log_level_t;

/* Logging and debugging. */
void log_vprintf(log_level_t level, const char *format, va_list ap);
void log_printf(log_level_t level, const char *format, ...);
void log_syserr(log_level_t level, const char *format, ...);
void log_sockerr(log_level_t level, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* _GL_LOGGING_H */
