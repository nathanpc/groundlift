/**
 * request.h
 * Helps to encode and decode information for GroundLift's requests.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_REQUEST_H
#define _GL_REQUEST_H

#include <stdint.h>

#include "sockets.h"

/**
 * Receive request line's maximum length.
 */
#define GL_REQLINE_MAX 400

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Request error codes.
 */
typedef enum {
	ERR_CODE_REQ_BAD     = 400,
	ERR_CODE_REQ_REFUSED = 403,
	ERR_CODE_REQ_LONG    = 417,
	ERR_CODE_UNKNOWN     = 418,
	ERR_CODE_INTERNAL    = 500
} error_code_t;

/**
 * Types of requests that are acceptable.
 */
typedef enum {
	REQ_TYPE_UNKNOWN = '?',
	REQ_TYPE_FILE    = 'F',
	REQ_TYPE_URL     = 'U'
} reqtype_t;

/**
 * Information that's contained in the request line of a GL transaction.
 */
typedef struct {
	char *stype;
	char *name;
	long size;
	reqtype_t type;
} reqline_t;

/* Request replies. */
void send_ok(sockfd_t sockfd);
void send_continue(sockfd_t sockfd);
void send_refused(sockfd_t sockfd);
void send_error(sockfd_t sockfd, error_code_t code);

/* Request Line */
reqline_t *reqline_new(void);
reqline_t *reqline_parse(const char *line);
void reqline_free(reqline_t *reqline);
void reqline_dump(reqline_t *reqline);

#ifdef __cplusplus
}
#endif

#endif /* _GL_REQUEST_H */
