/**
 * protocol.h
 * GroundLift's glproto implementation and helper functions.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_PROTOCOL_H
#define _GL_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "error.h"
#include "sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets the type of a message from a generic message object.
 *
 * @param msg Message object.
 *
 * @return Message type.
 */
#define GLPROTO_MSG_TYPE(msg) ((msg)[2])

/**
 * Address of the first header value in a message. Already skips the header
 * separator marker.
 */
#define GLPROTO_MSG_HEADER_VAL_OFFSET 7

/**
 * Valid message types.
 */
typedef enum {
	GLPROTO_TYPE_INVALID = 0,
	GLPROTO_TYPE_DISCOVERY = 'D',
	GLPROTO_TYPE_URL = 'U',
	GLPROTO_TYPE_FILE = 'F'
} glproto_type_t;

/**
 * Common message structure.
 */
typedef struct {
	glproto_type_t type;
	uint16_t length;
	uint8_t glupi[8];
	char device[4];
	char *hostname;

	sock_handle_t *sock;
} glproto_msg_t;

/**
 * Discovery message structure.
 */
typedef struct {
	glproto_msg_t head;
} glproto_discovery_msg_t;

/* Special messages. */
extern glproto_msg_t *glproto_invalid_msg;

/* Initialization */
void glproto_init(void);

/* Constructor and destructor. */
glproto_msg_t *glproto_msg_new(glproto_type_t type);
glproto_msg_t *glproto_msg_new_our(glproto_type_t type);
void glproto_msg_free(glproto_msg_t *msg);

/* Validation and parsing. */
bool glproto_msg_head_isvalid(const uint8_t *head);
gl_err_t *glproto_msg_parse(glproto_msg_t **msg, const void *buf, size_t len);

/* Sending and receiving. */
gl_err_t *glproto_recvfrom(const sock_handle_t *sock, glproto_type_t *type,
                           glproto_msg_t **msg, sock_err_t *serr);
gl_err_t *glproto_msg_sendto(const sock_handle_t *sock, glproto_msg_t *msg);

/* Misc. */
size_t glproto_msg_sizeof(glproto_type_t type);

/* Debugging */
void glproto_msg_print(const glproto_msg_t *msg, const char *prefix);

#ifdef __cplusplus
}
#endif

#endif /* _GL_PROTOCOL_H */
