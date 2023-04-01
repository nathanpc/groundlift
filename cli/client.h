/**
 * client.h
 * GroundLift client common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_CLIENT_H
#define _GL_CLIENT_H

#include <groundlift/tcp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialization and destruction. */
bool gl_client_init(const char *addr, uint16_t port);
void gl_client_free(void);

/* Client connection lifecycle. */
bool gl_client_connect(void);
bool gl_client_disconnect(void);
bool gl_client_thread_join(void);

/* Getters and setters. */
tcp_client_t *gl_client_get(void);

#ifdef __cplusplus
}
#endif

#endif /* _GL_CLIENT_H */
