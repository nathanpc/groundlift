/**
 * server.c
 * GroundLift server request handling components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "server.h"

#include <stdbool.h>
#include <string.h>

#include "conf.h"
#include "protocol.h"

/* Private methods. */
static void *server_thread_func(void *handle_ptr);

/**
 * Allocates a brand new server handle object and populates it with some sane
 * defaults.
 *
 * @warning This function allocates memory that must be free'd by you.
 *
 * @return Newly allocated server handle object or NULL if an error occurred.
 *
 * @see gl_server_setup
 * @see gl_server_free
 */
server_handle_t *gl_server_new(void) {
	server_handle_t *handle;

	/* Allocate some memory for our handle object. */
	handle = (server_handle_t *)malloc(sizeof(server_handle_t));
	if (handle == NULL)
		return NULL;

	/* Set some sane defaults. */
	handle->sock = NULL;

	/* Threads */
	handle->threads.main = NULL;

	/* Mutexes */
	handle->mutexes.main = thread_mutex_new();

	/* Event handlers. */
	handle->events.started = NULL;
	handle->events.stopped = NULL;

	/* Event handler arguments. */
	handle->event_args.started = NULL;
	handle->event_args.stopped = NULL;

	return handle;
}

/**
 * Sets everything up for the server handle to be usable.
 *
 * @param handle Server handle object.
 * @param addr   Network address to bind to as a string or NULL to use
 *               INADDR_ANY.
 * @param port   Port to listen on.
 *
 * @return Error report or NULL if the operation was successful.
 *
 * @see gl_server_start
 */
gl_err_t *gl_server_setup(server_handle_t *handle, const char *addr,
						  uint16_t port) {
	gl_err_t *err = NULL;

	/* Get a socket handle. */
	handle->sock = socket_new();
	if (handle->sock == NULL) {
		return gl_error_push_errno(ERR_TYPE_GL, GL_ERR_SERVER,
			EMSG("Failed to allocate the server socket"));
	}

	/* Setup the socket. */
	socket_setaddr(handle->sock, addr, port);
	err = socket_setup_udp(handle->sock, true, 0);

	return err;
}

/**
 * Frees up any resources allocated by the server. Shutting down the server
 * previously to calling this function isn't required and will be performed by
 * this function if needed.
 *
 * @param handle Server handle object.
 *
 * @return Error report or NULL if the operation was successful.
 *
 * @see gl_server_stop
 */
gl_err_t *gl_server_free(server_handle_t *handle) {
	gl_err_t *err;

	/* Do we even have a handle? */
	if (handle == NULL)
		return NULL;

	/* Stop the server and free up any allocated socket resources. */
	gl_server_stop(handle);
	socket_free(handle->sock);
	handle->sock = NULL;

	/* Join the server thread back into us. */
	if (thread_join(&handle->threads.main, (void **)&err) > THREAD_OK) {
		err = gl_error_push_errno(ERR_TYPE_SYS, SYS_ERR_THREAD,
								  EMSG("Main server thread join failed"));
	}

	/* Free our mutexes. */
	thread_mutex_free(&handle->mutexes.main);

	return err;
}

/**
 * Starts the server up.
 *
 * @param handle Server handle object.
 *
 * @return Error report or NULL if the operation was successful.
 *
 * @see gl_server_stop
 */
gl_err_t *gl_server_start(server_handle_t *handle) {
	int ret;

	/* Check if we already have a server running. */
	if (handle->threads.main != NULL) {
		return gl_error_push(ERR_TYPE_GL, GL_ERR_WARNING,
							 EMSG("Server's main thread already created"));
	}

	/* Allocate some memory for our main server thread. */
	handle->threads.main = thread_new();
	if (handle->threads.main == NULL) {
		return gl_error_push_errno(ERR_TYPE_SYS, SYS_ERR_MALLOC,
			EMSG("Failed to allocate the server thread"));
	}

	/* Create the server thread. */
	ret = thread_create(handle->threads.main, server_thread_func,
						(void *)handle);
	if (ret) {
		handle->threads.main = NULL;
		return gl_error_push_errno(ERR_TYPE_SYS, SYS_ERR_THREAD,
								   EMSG("Failed to start the server thread"));
	}

	return NULL;
}

/**
 * Waits for the server thread to finish.
 *
 * @param handle Server handle object.
 *
 * @return Error report or NULL if the operation was successful.
 *
 * @see gl_server_stop
 */
gl_err_t *gl_server_loop(server_handle_t *handle) {
	gl_err_t *err;

	/* Join the server thread back into us. */
	if (thread_join(&handle->threads.main, (void **)&err) > THREAD_OK) {
		err = gl_error_push_errno(ERR_TYPE_SYS, SYS_ERR_THREAD,
		                          EMSG("Main server thread join failed"));
	}

	return err;
}

/**
 * Stops the running server.
 *
 * @param handle Server handle object.
 *
 * @return Error report or NULL if the operation was successful.
 *
 * @see gl_server_free
 */
gl_err_t *gl_server_stop(server_handle_t *handle) {
	gl_err_t *err = NULL;

	/* Do we even need to stop it? */
	if ((handle == NULL) || (handle->sock == NULL))
		return NULL;

	/* Shut the thing down. */
	thread_mutex_lock(handle->mutexes.main);
	err = socket_shutdown(handle->sock);
	handle->sock = NULL;
	thread_mutex_unlock(handle->mutexes.main);

	/* Check for errors so far. */
	if (err != NULL) {
		err = gl_error_push(ERR_TYPE_GL, GL_ERR_SERVER,
			EMSG("Failed to shutdown the main server socket"));
	}

	/* Wait for the server thread to stop. */
	err = gl_server_loop(handle);

	return err;
}

/**
 * Server thread function.
 *
 * @param handle_ptr Server handle object.
 *
 * @return Error report or NULL if the operation was successful.
 */
void *server_thread_func(void *handle_ptr) {
	server_handle_t *handle;
	gl_err_t *err;
	sock_err_t serr;
	glproto_type_t type;
	glproto_msg_t *msg;

	/* Initialize variables. */
	handle = (server_handle_t *)handle_ptr;
	err = NULL;

	/* Trigger the server started event. */
	if (handle->events.started != NULL)
		handle->events.started(handle->sock, handle->event_args.started);

	/* Listen for messages. */
	msg = NULL;
	while ((err = glproto_recvfrom(handle->sock, &type, &msg, &serr)) == NULL) {
		/* Check if the message should be ignored. */
		if ((type == GLPROTO_TYPE_INVALID) || (serr == SOCK_EVT_TIMEOUT))
			continue;

		glproto_msg_print(msg);

		/* Free up any allocated resources. */
		glproto_msg_free(msg);
		msg = NULL;
	}

	return (void *)err;
}

/**
 * Sets the Started event callback function.
 *
 * @param handle Server handle object.
 * @param func   Started event callback function.
 * @param arg    Optional parameter to be sent to the event handler.
 */
void gl_server_evt_start_set(server_handle_t *handle,
							 gl_server_evt_start_func func, void *arg) {
	handle->events.started = func;
	handle->event_args.started = arg;
}

/**
 * Sets the Stopped event callback function.
 *
 * @param handle Server handle object.
 * @param func   Stopped event callback function.
 * @param arg    Optional parameter to be sent to the event handler.
 */
void gl_server_evt_stop_set(server_handle_t *handle,
							gl_server_evt_stop_func func, void *arg) {
	handle->events.stopped = func;
	handle->event_args.stopped = arg;
}
