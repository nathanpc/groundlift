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
#include <utils/filesystem.h>
#include <utils/threads.h>

#include "error.h"
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
 * Structure holding all of the information about a discovered peer on the
 * network.
 */
typedef struct {
	const char *name;
	const sock_bundle_t *sock;
} gl_discovery_peer_t;

/**
 * Client connected to server event callback function pointer type definition.
 *
 * @param client Client connection handle object.
 * @param arg    Optional data set by the event handler setup.
 */
typedef void (*gl_client_evt_conn_func)(const tcp_client_t *client, void *arg);

/**
 * Client disconnected to server event callback function pointer type
 * definition.
 *
 * @param client Client connection handle object.
 * @param arg    Optional data set by the event handler setup.
 */
typedef void (*gl_client_evt_disconn_func)(const tcp_client_t *client,
										   void *arg);

/**
 * Client connection request reply received event callback function pointer type
 * definition.
 *
 * @param fb       File bundle that was uploaded.
 * @param accepted Has the connection request been accepted?
 * @param arg      Optional data set by the event handler setup.
 */
typedef void (*gl_client_evt_conn_req_resp_func)(const file_bundle_t *fb,
												 bool accepted, void *arg);

/**
 * Client file upload progress event callback function pointer type definition.
 *
 * @param progress Structure containing all the information about the progress.
 * @param arg      Optional data set by the event handler setup.
 */
typedef void (*gl_client_evt_put_progress_func)(
	const gl_client_progress_t *progress, void *arg);

/**
 * Client file upload succeeded event callback function pointer type
 * definition.
 *
 * @param fb  File bundle that was uploaded.
 * @param arg Optional data set by the event handler setup.
 */
typedef void (*gl_client_evt_put_succeed_func)(const file_bundle_t *fb,
											   void *arg);

/**
 * Discovered peers event callback function pointer type definition.
 *
 * @param peer Discovered peer object.
 * @param arg  Optional data set by the event handler setup.
 */
typedef void (*gl_client_evt_discovery_peer_func)(
	const gl_discovery_peer_t *peer, void *arg);

/**
 * Peer discovery finished event callback function pointer type definition.
 *
 * @param arg Optional data set by the event handler setup.
 */
typedef void (*gl_client_evt_discovery_end_func)(void *arg);

/**
 * Client handle object.
 */
typedef struct {
	tcp_client_t *client;
	thread_t thread;

	file_bundle_t *fb;
	bool running;

	/* Mutexes */
	struct {
		thread_mutex_t client;
		thread_mutex_t send;
	} mutexes;

	/* Event handlers. */
	struct {
		gl_client_evt_conn_func connected;
		gl_client_evt_disconn_func disconnected;
		gl_client_evt_conn_req_resp_func request_response;
		gl_client_evt_put_progress_func put_progress;
		gl_client_evt_put_succeed_func put_succeeded;
	} events;

	/* Event handler arguments. */
	struct {
		void *connected;
		void *disconnected;
		void *request_response;
		void *put_progress;
		void *put_succeeded;
	} event_args;
} client_handle_t;

/**
 * Peer discovery client handle object.
 */
typedef struct {
	thread_t thread;
	sock_bundle_t sock;

	/* Mutexes */
	struct {
		thread_mutex_t client;
	} mutexes;

	/* Event handlers. */
	struct {
		gl_client_evt_discovery_peer_func discovered_peer;
		gl_client_evt_discovery_end_func finished;
	} events;

	/* Event handler arguments. */
	struct {
		void *discovered_peer;
		void *finished;
	} event_args;
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
gl_err_t *gl_client_discovery_setup(discovery_client_t *handle,
									in_addr_t in_addr, uint16_t port);
gl_err_t *gl_client_discover_peers(discovery_client_t *handle);
gl_err_t *gl_client_discovery_abort(discovery_client_t *handle);
gl_err_t *gl_client_discovery_thread_join(discovery_client_t *handle);
gl_err_t *gl_client_discovery_free(discovery_client_t *handle);

/* Event handler setters. */
void gl_client_evt_conn_set(client_handle_t *handle,
							gl_client_evt_conn_func func, void *arg);
void gl_client_evt_disconn_set(client_handle_t *handle,
							   gl_client_evt_disconn_func func, void *arg);
void gl_client_evt_conn_req_resp_set(client_handle_t *handle,
									 gl_client_evt_conn_req_resp_func func,
									 void *arg);
void gl_client_evt_put_progress_set(client_handle_t *handle,
									gl_client_evt_put_progress_func func,
									void *arg);
void gl_client_evt_put_succeed_set(client_handle_t *handle,
								   gl_client_evt_put_succeed_func func,
								   void *arg);
void gl_client_evt_discovery_peer_set(discovery_client_t *handle,
									  gl_client_evt_discovery_peer_func func,
									  void *arg);
void gl_client_evt_discovery_end_set(discovery_client_t *handle,
									 gl_client_evt_discovery_end_func func,
									 void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _GL_CLIENT_H */
