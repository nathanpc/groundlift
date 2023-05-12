/**
 * server.c
 * GroundLift command-line server.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "server.h"

#include <groundlift/defaults.h>
#include <utils/filesystem.h>
#include <utils/logging.h>

/* Public variables. */
server_handle_t *g_server;

/* Server event handlers. */
void event_started(const server_t *server);
int event_conn_req(const file_bundle_t *fb);
void event_stopped(void);

/* Server client's connection event handlers. */
void event_conn_accepted(const server_conn_t *conn);
void event_conn_download_progress(const gl_server_conn_progress_t *progress);
void event_conn_download_success(const file_bundle_t *fb);
void event_conn_closed(void);

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
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to construct the server handle object."));
	}

	/* Setup event handlers. */
	gl_server_evt_start_set(g_server, event_started);
	gl_server_evt_client_conn_req_set(g_server, event_conn_req);
	gl_server_evt_stop_set(g_server, event_stopped);
	gl_server_conn_evt_accept_set(g_server, event_conn_accepted);
	gl_server_conn_evt_download_progress_set(g_server,
											 event_conn_download_progress);
	gl_server_conn_evt_download_success_set(g_server,
											event_conn_download_success);
	gl_server_conn_evt_close_set(g_server, event_conn_closed);

	/* Initialize the server. */
	err = gl_server_setup(g_server, ip, port);
	if (err) {
		log_printf(LOG_ERROR, "Server setup failed.\n");
		goto cleanup;
	}

	/* Start the main server up. */
	err = gl_server_start(g_server);
	if (err) {
		log_printf(LOG_ERROR, "Server thread failed to start.\n");
		goto cleanup;
	}

	/* Start the discovery server. */
	err = gl_server_discovery_start(g_server, UDPSERVER_PORT);
	if (err) {
		log_printf(LOG_ERROR, "Discovery server thread failed to start.\n");
		goto cleanup;
	}

	/* Wait for the discovery server thread to return. */
	err = gl_server_discovery_thread_join(g_server);
	if (err) {
		log_printf(LOG_ERROR,
				   "Discovery server thread returned with errors.\n");
		goto cleanup;
	}

	/* Wait for the main server thread to return. */
	err = gl_server_thread_join(g_server);
	if (err) {
		log_printf(LOG_ERROR, "Server thread returned with errors.\n");
		goto cleanup;
	}

cleanup:
	/* Free up any resources. */
	gl_server_free(g_server);

	return err;
}

/**
 * Handles the server started event.
 *
 * @param server Server handle object.
 */
void event_started(const server_t *server) {
	char *ipstr;

	/* Print some information about the current state of the server. */
	ipstr = tcp_server_get_ipstr(server);
	printf("Server listening on %s port %u (discovery %u)\n", ipstr,
		   ntohs(server->tcp.addr_in.sin_port),
		   ntohs(server->udp.addr_in.sin_port));

	free(ipstr);
}

/**
 * Handles the server client connection requested event.
 *
 * @param fb File bundle of what is supposed to be received.
 *
 * @return 0 to refuses the request. Anything else will be treated as accepting.
 */
int event_conn_req(const file_bundle_t *fb) {
	int c;

	/* Ask the user for permission to accept the transfer of the file. */
	printf("Client wants to send the file '%s' with %lu bytes. Accept? [y/n]? ",
		   fb->base, fb->size);
	do {
		c = getc(stdin);
	} while ((c != 'y') && (c != 'Y') && (c != 'n') && (c != 'N'));

	return (c != 'n') && (c != 'N');
}

/**
 * Handles the server stopped event.
 */
void event_stopped(void) {
	printf("Server stopped\n");
}

/**
 * Handles the server connection accepted event.
 *
 * @param conn Client connection handle object.
 */
void event_conn_accepted(const server_conn_t *conn) {
	char *ipstr;

	/* Print out some client information. */
	ipstr = tcp_server_conn_get_ipstr(conn);
	printf("Client at %s connection accepted\n", ipstr);

	free(ipstr);
}

/**
 * Handles the server connection download progress event.
 *
 * @param progress Structure containing all the information about the progress.
 */
void event_conn_download_progress(const gl_server_conn_progress_t *progress) {
	printf("Receiving file... (%u/%lu)\n", progress->recv_bytes,
		   progress->fb->size);
}

/**
 * Handles the server connection download finished event.
 *
 * @param fb File bundle of the downloaded file.
 */
void event_conn_download_success(const file_bundle_t *fb) {
	printf("Finished receiving %s\n", fb->base);
}

/**
 * Handles the server connection closed event.
 */
void event_conn_closed(void) {
	printf("Client connection closed\n");
}
