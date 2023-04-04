/**
 * server.h
 * GroundLift server common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_SERVER_H
#define _GL_SERVER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialization and destruction. */
bool gl_server_init(const char *addr, uint16_t port);
void gl_server_free(void);

/* Server lifecycle. */
bool gl_server_start(void);
bool gl_server_stop(void);
tcp_err_t gl_server_conn_destroy(void);
bool gl_server_thread_join(void);

/* Getters and setters. */
server_t *gl_server_get(void);

#ifdef __cplusplus
}
#endif

#endif /* _GL_SERVER_H */
