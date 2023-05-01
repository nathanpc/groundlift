/**
 * client.h
 * GroundLift client common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_CLIENT_H
#define _GL_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "error.h"
#include "fileutils.h"
#include "obex.h"
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
 * Client connected to server event callback function pointer type definition.
 *
 * @param client Client connection handle object.
 */
typedef void (*gl_client_evt_conn_func)(const tcp_client_t *client);

/**
 * Client disconnected to server event callback function pointer type
 * definition.
 *
 * @param client Client connection handle object.
 */
typedef void (*gl_client_evt_disconn_func)(const tcp_client_t *client);

/**
 * Client connection request reply received event callback function pointer type
 * definition.
 *
 * @param fb       File bundle that was uploaded.
 * @param accepted Has the connection request been accepted?
 */
typedef void (*gl_client_evt_conn_req_resp_func)(const file_bundle_t *fb,
												 bool accepted);

/**
 * Client file upload progress event callback function pointer type definition.
 *
 * @param progress Structure containing all the information about the progress.
 */
typedef void (*gl_client_evt_put_progress_func)(
	const gl_client_progress_t *progress);

/**
 * Client file upload succeeded event callback function pointer type
 * definition.
 *
 * @param fb File bundle that was uploaded.
 */
typedef void (*gl_client_evt_put_succeed_func)(const file_bundle_t *fb);

/**
 * Discovered peers event callback function pointer type definition.
 *
 * @param name Hostname of the peer.
 * @param addr IP address information of the peer.
 */
typedef void (*gl_client_evt_discovery_peer_func)(const char *name,
												  const struct sockaddr *addr);

/**
 * Client handle object.
 */
typedef struct {
	tcp_client_t *client;
	pthread_t *thread;

	file_bundle_t *fb;
	bool running;

	/* Mutexes */
	struct {
		pthread_mutex_t *client;
		pthread_mutex_t *send;
	} mutexes;

	/* Event handlers. */
	struct {
		gl_client_evt_conn_func connected;
		gl_client_evt_disconn_func disconnected;
		gl_client_evt_conn_req_resp_func request_response;
		gl_client_evt_put_progress_func put_progress;
		gl_client_evt_put_succeed_func put_succeeded;
	} events;
} client_handle_t;

/**
 * Peer discovery client handle object.
 */
typedef struct {
	pthread_t *thread;
	sock_bundle_t sock;

	/* Event handlers. */
	struct {
		gl_client_evt_discovery_peer_func discovered_peer;
	} events;
} discovery_client_t;

/* Initialization and destruction. */
client_handle_t *gl_client_new(void);
gl_err_t *gl_client_setup(client_handle_t *handle, const char *addr,
						  uint16_t port, const char *fname);
gl_err_t *gl_client_free(client_handle_t *handle);

/* Client connection lifecycle. */
gl_err_t *gl_client_connect(client_handle_t *handle);
gl_err_t *gl_client_disconnect(client_handle_t *handle);
gl_err_t *gl_client_thread_join(client_handle_t *handle);

/* Discovery service. */
discovery_client_t *gl_client_discovery_new(void);
gl_err_t *gl_client_discovery_setup(discovery_client_t *handle, uint16_t port);
gl_err_t *gl_client_discover_peers(discovery_client_t *handle);
gl_err_t *gl_client_discovery_thread_join(discovery_client_t *handle);
gl_err_t *gl_client_discovery_free(discovery_client_t *handle);

/* Event handler setters. */
void gl_client_evt_conn_set(client_handle_t *handle,
							gl_client_evt_conn_func func);
void gl_client_evt_disconn_set(client_handle_t *handle,
							   gl_client_evt_disconn_func func);
void gl_client_evt_conn_req_resp_set(client_handle_t *handle,
									 gl_client_evt_conn_req_resp_func func);
void gl_client_evt_put_progress_set(client_handle_t *handle,
									gl_client_evt_put_progress_func func);
void gl_client_evt_put_succeed_set(client_handle_t *handle,
								   gl_client_evt_put_succeed_func func);
void gl_client_evt_discovery_peer_set(discovery_client_t *handle,
									  gl_client_evt_discovery_peer_func func);

#ifdef __cplusplus
}
#endif

#endif /* _GL_CLIENT_H */
