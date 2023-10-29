/**
 * logging.h
 * GroundLift's logging and log reporting utility.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_UTILS_LOGGING_H
#define _GL_UTILS_LOGGING_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log levels.
 */
typedef enum {
	LOG_FATAL = 0,
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_DEBUG
} log_level_t;

/* Logging functions. */
void log_printf(log_level_t level, const char *format, ...);
void log_msg(log_level_t level, const char *msg);
void log_errno(log_level_t level, const char *msg);
void log_sockerrno(log_level_t level, const char *msg, int err);

#ifdef __cplusplus
}
#endif

#endif /* _GL_UTILS_LOGGING_H */
