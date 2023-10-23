/**
 * server.h
 * GroundLift server request handling components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_SERVER_H
#define _GL_SERVER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <utils/filesystem.h>
#include <utils/threads.h>

#include "error.h"
#include "sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Server started event callback function pointer type definition.
 *
 * @param sock Server socket handle object.
 * @param arg  Optional data set by the event handler setup.
 */
typedef void (*gl_server_evt_start_func)(const sock_handle_t *sock, void *arg);

/**
 * Server stopped closed event callback function pointer type definition.
 *
 * @param arg Optional data set by the event handler setup.
 */
typedef void (*gl_server_evt_stop_func)(void *arg);

/**
 * Server handle object.
 */
typedef struct {
	sock_handle_t *sock;

	/* Threads */
	struct {
		thread_t main;
	} threads;

	/* Mutexes */
	struct {
		thread_mutex_t main;
	} mutexes;

	/* Event handlers. */
	struct {
		gl_server_evt_start_func started;
		gl_server_evt_stop_func stopped;
	} events;

	/* Event handler arguments. */
	struct {
		void *started;
		void *stopped;
	} event_args;
} server_handle_t;

/* Initialization and destruction. */
server_handle_t *gl_server_new(void);
gl_err_t *gl_server_setup(server_handle_t *handle, const char *addr,
						  uint16_t port);
gl_err_t *gl_server_free(server_handle_t *handle);

/* Server lifecycle. */
gl_err_t *gl_server_start(server_handle_t *handle);
gl_err_t *gl_server_stop(server_handle_t *handle);

/* Callbacks */
void gl_server_evt_start_set(server_handle_t *handle,
							 gl_server_evt_start_func func, void *arg);
void gl_server_evt_stop_set(server_handle_t *handle,
							gl_server_evt_stop_func func, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _GL_SERVER_H */
