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

#include "sockets.h"

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
 * OBEX function status return codes.
 */
typedef enum {
	OBEX_OK = 0,
	OBEX_ERR_ENCODE,
	OBEX_ERR_PACKET_RECV,
	OBEX_ERR_NO_PACKET,
	OBEX_ERR_PACKET_NEW,
	OBEX_ERR_HEADER_NOT_FOUND,
	OBEX_ERR_UNKNOWN
} obex_err_t;

/**
 * Return codes for high level GroundLift functions.
 */
typedef enum {
	GL_OK = 0,
	GL_ERR_DISCV_START,
	GL_ERR_UNHANDLED_STATE,
	GL_ERR_INVALID_STATE_OPCODE,
	GL_ERR_INVALID_HANDLE,
	GL_ERR_FSIZE,
	GL_ERR_FOPEN,
	GL_ERR_FREAD,
	GL_ERR_FWRITE,
	GL_ERR_FCLOSE,
	GL_ERR_SOCKET,
	GL_ERR_THREAD,
	GL_ERR_UNKNOWN
} gl_ret_t;

/**
 * Error types enumeration.
 */
typedef enum {
	ERR_TYPE_UNKNOWN = 0,
	ERR_TYPE_TCP,
	ERR_TYPE_GL,
	ERR_TYPE_OBEX
} err_type_t;

/**
 * Fully-featured, all knowing, detailed error reporting structure.
 */
typedef struct {
	err_type_t type;
	union {
		int8_t generic;

		tcp_err_t tcp;
		obex_err_t obex;
		gl_ret_t gl;
	} error;

	char *msg;
} gl_err_t;

/* Initialization and destruction. */
gl_err_t *gl_error_new(err_type_t type, int8_t err, const char *msg);
gl_err_t *gl_error_new_prefixed(err_type_t type, int8_t err, const char *prefix,
								const char *msg);
gl_err_t *gl_error_new_errno(err_type_t type, int8_t err, const char *prefix);
void gl_error_free(gl_err_t *err);

/* Report manipulation. */
gl_err_t *gl_error_subst_msg(gl_err_t *err, const char *msg);

/* Debugging */
void gl_error_print(gl_err_t *err);

/* Logging */
void log_sockerrno(const char *msg, int err);

#ifdef __cplusplus
}
#endif

#endif /* _GL_ERROR_H */
