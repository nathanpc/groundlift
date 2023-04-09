/**
 * error.h
 * GroundLift's error handling and reporting utility.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_ERROR_H
#define _GL_ERROR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Decorate the error message with more information. */
#ifdef DEBUG
	#define STRINGIZE(x) STRINGIZE_WRAPPER(x)
	#define STRINGIZE_WRAPPER(x) #x
	#define EMSG(msg) msg " [" __FILE__ ":" STRINGIZE(__LINE__) "]"
	#define DEBUG_LOG(msg) \
		printf("[DEBUG] \"%s\" [" __FILE__ ":" STRINGIZE(__LINE__) "]\n", (msg))
#else
	#define EMSG(msg) msg
	#define DEBUG_LOG(msg) (void)0
#endif /* DEBUG */

/**
 * Return codes for high level GroundLift functions.
 */
typedef enum {
	GL_OK = 0,
	GL_CONN_REFUSED
} gl_ret_t;

/**
 * Error types enumeration.
 */
typedef enum {
	ERR_TYPE_UNKNOWN = 0,
	ERR_TYPE_TCP,
	ERR_TYPE_GL
} err_type_t;

/**
 * Fully-featured, all knowing, detailed error reporting structure.
 */
typedef struct {
	err_type_t type;
	union {
		int8_t generic;

		tcp_err_t tcp;
		gl_ret_t gl;
	} error;

	char *msg;
} gl_err_t;

/* Initialization and destruction. */
gl_err_t *gl_error_new(err_type_t type, int8_t err, const char *msg);
void gl_error_free(gl_err_t *err);

/* Report manipulation. */
gl_err_t *gl_error_subst_msg(gl_err_t *err, const char *msg);

/* Debugging */
void gl_error_print(gl_err_t *err);

#ifdef __cplusplus
}
#endif

#endif /* _GL_ERROR_H */
