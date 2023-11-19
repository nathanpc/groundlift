/**
 * server.c
 * GroundLift command-line server.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "server.h"

#include <utils/logging.h>

/* Public variables. */
server_handle_t *g_server;

/* Server event handlers. */
void event_started(const sock_handle_t *sock, void *arg);
void event_stopped(void *arg);

/**
 * Starts up the server and wait for it to be shutdown.
 *
 * @param ip   Server's IP to send the data to.
 * @param port Server port to talk to.
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
gl_err_t *server_run(const char *ip, uint16_t port) {
	gl_err_t *err = NULL;

	/* Construct the server handle object. */
	g_server = gl_server_new();
	if (g_server == NULL) {
		return gl_error_push_errno(ERR_TYPE_GL, GL_ERR_SERVER,
			EMSG("Failed to allocate the server handle object."));
	}

	/* Set up event handlers. */
	gl_server_evt_start_set(g_server, event_started, NULL);
	gl_server_evt_stop_set(g_server, event_stopped, NULL);

	/* Set up the server. */
	err = gl_server_setup(g_server, ip, port);
	if (err)
		goto cleanup;

	/* Start the server up. */
	err = gl_server_start(g_server);
	if (err == NULL)
		return NULL;

cleanup:
	/* Free up any resources. */
	gl_server_free(g_server);

	return err;
}

/**
 * Handles the server started event.
 *
 * @param sock Server socket handle object.
 * @param arg  Optional data set by the event handler setup.
 */
void event_started(const sock_handle_t *sock, void *arg) {
	char *ipaddr;

	/* Ignore unused arguments. */
	(void)arg;

	/* Print some information about the current state of the server. */
	socket_tostr(&ipaddr, sock);
	printf("Server listening on %s\n", ipaddr);

	free(ipaddr);
}

/**
 * Handles the server stopped event.
 *
 * @param arg Optional data set by the event handler setup.
 */
void event_stopped(void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	printf("Server stopped\n");
}

#if 0
/**
 * Handles the server client connection requested event.
 *
 * @param req Information about the client and its request.
 * @param arg Optional data set by the event handler setup.
 *
 * @return 0 to refuses the request. Anything else will be treated as accepting.
 */
int event_conn_req(const gl_server_conn_req_t *req, void *arg) {
	int c;
	float fsize;
	char prefix;

	/* Ignore unused arguments. */
	(void)arg;

	/* Get a human-readable file size. */
	fsize = file_size_readable(req->fb->size, &prefix);

	/* Ask the user for permission to accept the transfer of the file. */
	if (prefix != 'B') {
		printf("%s wants to send you the file '%s' (%.2f %cB). Accept? [y/n]? ",
			   req->hostname, req->fb->base, fsize, prefix);
	} else {
		printf("%s wants to send you the file '%s' (%.0f bytes). Accept? "
			   "[y/n]? ", req->hostname, req->fb->base, fsize);
	}

	/* Wait for the user to reply. */
	do {
		c = getc(stdin);
	} while ((c != 'y') && (c != 'Y') && (c != 'n') && (c != 'N'));

	return (c != 'n') && (c != 'N');
}

/**
 * Handles the server connection accepted event.
 *
 * @param conn Client connection handle object.
 * @param arg  Optional data set by the event handler setup.
 */
void event_conn_accepted(const server_conn_t *conn, void *arg) {
	char *ipstr;

	/* Ignore unused arguments. */
	(void)arg;

	/* Print out some client information. */
	ipstr = tcp_server_conn_get_ipstr(conn);
	printf("Client at %s connection accepted\n", ipstr);

	free(ipstr);
}

/**
 * Handles the server connection download progress event.
 *
 * @param progress Structure containing all the information about the progress.
 * @param arg      Optional data set by the event handler setup.
 */
void event_conn_download_progress(const gl_server_conn_progress_t *progress,
								  void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	printf("Receiving file... (%u/%lu)\n", progress->recv_bytes,
		   progress->fb->size);
}

/**
 * Handles the server connection download finished event.
 *
 * @param fb  File bundle of the downloaded file.
 * @param arg Optional data set by the event handler setup.
 */
void event_conn_download_success(const file_bundle_t *fb, void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	printf("Finished receiving %s\n", fb->base);
}

/**
 * Handles the server connection closed event.
 *
 * @param arg Optional data set by the event handler setup.
 */
void event_conn_closed(void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	printf("Client connection closed\n");
}
#endif
