/**
 * server.c
 * GroundLift command-line server.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "server.h"

#include <groundlift/defaults.h>

/* Server event handlers. */
void event_started(const server_t *server);
int event_conn_req(const char *fname, uint32_t fsize);
void event_stopped(void);

/* Server client's connection event handlers. */
void event_conn_accepted(const server_conn_t *conn);
void event_conn_download_progress(const gl_server_conn_progress_t *progress);
void event_conn_download_success(const char *fpath);
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
	/* TODO: Start using err instead of printing errors. */

	/* Initialize the server. */
	if (!gl_server_init(ip, port)) {
		fprintf(stderr, "Failed to initialize the server.\n");
		goto cleanup;
	}

	/* Setup event handlers. */
	gl_server_evt_start_set(event_started);
	gl_server_evt_client_conn_req_set(event_conn_req);
	gl_server_evt_stop_set(event_stopped);
	gl_server_conn_evt_accept_set(event_conn_accepted);
	gl_server_conn_evt_download_progress_set(
		event_conn_download_progress);
	gl_server_conn_evt_download_success_set(
		event_conn_download_success);
	gl_server_conn_evt_close_set(event_conn_closed);

	/* Start the main server up. */
	if (!gl_server_start()) {
		fprintf(stderr, "Failed to start the server.\n");
		goto cleanup;
	}

	/* Start the discovery server. */
	if (!gl_server_discovery_start(UDPSERVER_PORT)) {
		fprintf(stderr, "Failed to start the discovery server.\n");
		goto cleanup;
	}

	/* Wait for it to return. */
	if (!gl_server_discovery_thread_join()) {
		fprintf(stderr, "Discovery server thread returned with errors.\n");
		goto cleanup;
	}

	/* Wait for it to return. */
	if (!gl_server_thread_join()) {
		fprintf(stderr, "Server thread returned with errors.\n");
		goto cleanup;
	}

cleanup:
	/* Free up any resources. */
	gl_server_free();

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
 * @return 0 to refuses the request. Anything else will be treated as accepting.
 */
int event_conn_req(const char *fname, uint32_t fsize) {
	int c;

	/* Ask the user for permission to accept the transfer of the file. */
	printf("Client wants to send a file '%s' with %u bytes. Accept? [y/n]? ",
		   fname, fsize);
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
	printf("Receiving file... (%u/%u)\n", progress->recv_bytes,
		   progress->fsize);
}

/**
 * Handles the server connection download finished event.
 *
 * @param fpath Path of the downloaded file.
 */
void event_conn_download_success(const char *fpath) {
	printf("Finished receiving %s\n", fpath);
}

/**
 * Handles the server connection closed event.
 */
void event_conn_closed(void) {
	printf("Client connection closed\n");
}
