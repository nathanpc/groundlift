/**
 * client.h
 * GroundLift client request making components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_CLIENT_H
#define _GL_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <utils/filesystem.h>
#include <utils/threads.h>

#include "error.h"
#include "protocol.h"
#include "sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure holding all of the information about a transfer's current progress.
 */
typedef struct {
	file_bundle_t *fb;

	uint32_t sent_bytes;
	uint32_t chunks;
	uint32_t sent_chunk;
	uint16_t csize;
} gl_client_progress_t;

/**
 * Structure holding a list of responses to the discovery message from the
 * peers on the network.
 */
typedef struct {
	uint16_t count;
	glproto_discovery_msg_t **list;
} gl_peer_list_t;

/**
 * Client handle object.
 */
typedef struct {
	sock_handle_t *sock;

	/* File sending stuff. */
	file_bundle_t *fb;
	bool running;
} client_handle_t;

/* Initialization and destruction. */
client_handle_t *gl_client_new(void);
gl_err_t *gl_client_free(client_handle_t *handle);
void gl_peer_list_free(gl_peer_list_t *peers);

/* Client connection lifecycle. */
gl_err_t *gl_client_connect(client_handle_t *handle, const char *addr,
                            uint16_t port);
gl_err_t *gl_client_disconnect(client_handle_t *handle);

/* Client services. */
gl_err_t *gl_client_discover_peers(gl_peer_list_t **peers);
gl_err_t *gl_client_discover_peers_inaddr(gl_peer_list_t **peers,
                                          in_addr_t inaddr);
gl_err_t *gl_client_send_file(client_handle_t *handle, const char *fpath);

#ifdef __cplusplus
}
#endif

#endif /* _GL_CLIENT_H */
