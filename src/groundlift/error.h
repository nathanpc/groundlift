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
 * Socket error codes.
 */
typedef enum {
	SOCK_EVT_TIMEOUT = -3,
	SOCK_EVT_CONN_SHUTDOWN,
	SOCK_EVT_CONN_CLOSED,
	SOCK_OK,
	SOCK_ERR_UNKNOWN,
	SOCK_ERR_ESOCKET,
	SOCK_ERR_ESETSOCKOPT,
	SOCK_ERR_EBIND,
	SOCK_ERR_ELISTEN,
	SOCK_ERR_ECLOSE,
	SOCK_ERR_ESEND,
	SOCK_ERR_ERECV,
	SOCK_ERR_ECONNECT,
	SOCK_ERR_ESHUTDOWN,
	SOCK_ERR_EIOCTL,
#ifndef SINGLE_IFACE_MODE
	IFACE_ERR_GETIFADDR
#endif /* !SINGLE_IFACE_MODE */
} sock_err_t;

/**
 * Return codes for high level GroundLift functions.
 */
typedef enum {
	GL_OK = 0,
	GL_ERR_UNKNOWN,
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
	GL_ERR_THREAD
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
	char *msg;

	union {
		int8_t generic;

		sock_err_t sock;
		gl_ret_t gl;
	} code;

	void *prev;
} gl_err_t;

/* Initialization and destruction. */
void gl_error_init(void);
gl_err_t *gl_error_push(err_type_t type, int8_t err, const char *msg);
gl_err_t *gl_error_push_prefix(err_type_t type, int8_t err, const char *prefix,
							   const char *msg);
gl_err_t *gl_error_push_errno(err_type_t type, int8_t err, const char *prefix);
gl_err_t *gl_error_pop(gl_err_t *err);
void gl_error_clear(void);

/* Report manipulation. */
gl_err_t *gl_error_subst_msg(gl_err_t *err, const char *msg);

/* Debugging */
void gl_error_print(gl_err_t *err);

#ifdef __cplusplus
}
#endif

#endif /* _GL_ERROR_H */
