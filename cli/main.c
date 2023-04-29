/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <groundlift/client.h>
#include <groundlift/conf.h>
#include <groundlift/defaults.h>
#include <groundlift/server.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private methods. */
gl_err_t *client_send(const char *ip, uint16_t port, char *fname);
gl_err_t *client_list_peers(void);
void sigint_handler(int sig);

/* Server event handlers. */
void server_event_started(const server_t *server);
int server_event_conn_req(const char *fname, uint32_t fsize);
void server_event_stopped(void);

/* Server client's connection event handlers. */
void server_conn_event_accepted(const server_conn_t *conn);
void server_conn_event_download_progress(const gl_server_conn_progress_t *progress);
void server_conn_event_download_success(const char *fpath);
void server_conn_event_closed(void);

/* Client event handlers. */
void client_event_connected(const tcp_client_t *client);
void client_event_conn_req_resp(const char *fname, bool accepted);
void client_event_send_progress(const gl_client_progress_t *progress);
void client_event_send_success(const char *fname);
void client_event_disconnected(const tcp_client_t *client);

/**
 * Program's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Command line arguments passed.
 *
 * @return Application's return code.
 */
int main(int argc, char **argv) {
	gl_err_t *err;
	int ret;

	/* Setup our defaults. */
	ret = 0;
	err = NULL;

	/* Catch the interrupt signal from the console. */
	signal(SIGINT, sigint_handler);

	/* Initialize the configuration module. */
	conf_init();

	/* Start as server or as client. */
	if ((argc < 2) || (argv[1][0] == 's')) {
		/* Initialize the server. */
		if (!gl_server_init(NULL, TCPSERVER_PORT)) {
			fprintf(stderr, "Failed to initialize the server.\n");

			ret = 1;
			goto cleanup;
		}

		/* Setup callbacks. */
		gl_server_evt_start_set(server_event_started);
		gl_server_evt_client_conn_req_set(server_event_conn_req);
		gl_server_evt_stop_set(server_event_stopped);
		gl_server_conn_evt_accept_set(server_conn_event_accepted);
		gl_server_conn_evt_download_progress_set(
			server_conn_event_download_progress);
		gl_server_conn_evt_download_success_set(
			server_conn_event_download_success);
		gl_server_conn_evt_close_set(server_conn_event_closed);

		/* Start the main server up. */
		if (!gl_server_start()) {
			fprintf(stderr, "Failed to start the server.\n");

			ret = 1;
			goto cleanup;
		}

		/* Start the discovery server. */
		if (!gl_server_discovery_start(UDPSERVER_PORT)) {
			fprintf(stderr, "Failed to start the discovery server.\n");

			ret = 1;
			goto cleanup;
		}

		/* Wait for it to return. */
		if (!gl_server_discovery_thread_join()) {
			fprintf(stderr, "Discovery server thread returned with errors.\n");

			ret = 1;
			goto cleanup;
		}

		/* Wait for it to return. */
		if (!gl_server_thread_join()) {
			fprintf(stderr, "Server thread returned with errors.\n");

			ret = 1;
			goto cleanup;
		}
	} else if ((argc == 5) && (argv[1][0] == 'c')) {
		/* Exchange some information with the server. */
		err = client_send(argv[2], (uint16_t)atoi(argv[3]), argv[4]);
	} else if ((argc < 2) || (argv[1][0] == 'l')) {
		/* List peers on the network. */
		err = client_list_peers();
	} else {
		printf("Unknown mode or invalid number of arguments.\n");
		return 1;
	}

cleanup:
	/* Check if we had any errors to report. */
	gl_error_print(err);
	if (err != NULL)
		ret = 1;

	/* Free up any resources. */
	gl_server_free();
	gl_client_free();
	gl_error_free(err);
	conf_free();

	return ret;
}

/**
 * Perform an entire send exchange with the server.
 *
 * @param ip    Server's IP to send the data to.
 * @param port  Server port to talk to.
 * @param fname Path to a file to be sent to the server.
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
gl_err_t *client_send(const char *ip, uint16_t port, char *fname) {
	gl_err_t *err = NULL;

	/* Initialize the client. */
	if (!gl_client_init(ip, port)) {
		fprintf(stderr, "Failed to initialize the client.\n");
		goto cleanup;
	}

	/* Setup callbacks. */
	gl_client_evt_conn_set(client_event_connected);
	gl_client_evt_conn_req_resp_set(client_event_conn_req_resp);
	gl_client_evt_put_progress_set(client_event_send_progress);
	gl_client_evt_put_succeed_set(client_event_send_success);
	gl_client_evt_disconn_set(client_event_disconnected);

	/* Connect to the server. */
	if (!gl_client_connect(fname)) {
		fprintf(stderr, "Failed to connect the client to the server.\n");
		goto cleanup;
	}

	/* Wait for it to return. */
	if (!gl_client_thread_join()) {
		fprintf(stderr, "Client thread returned with errors.\n");
		goto cleanup;
	}

cleanup:
	/* Free up any resources. */
	gl_client_free();

	return err;
}

/**
 * Lists the peers on the network.
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
gl_err_t *client_list_peers(void) {
	gl_err_t *err;

	/* Send discovery broadcast and listen to replies. */
	printf("Sending discovery broadcast...\n");
	err = gl_client_discover_peers(UDPSERVER_PORT);
	if (err)
		return err;

	/* TODO: Thread join and clean up resources. */

	printf("Stopped trying to discover peers.\n");

	return NULL;
}

/**
 * Handles the SIGINT interrupt event.
 *
 * @param sig Signal handle that generated this interrupt.
 */
void sigint_handler(int sig) {
	printf("Got a Ctrl-C\n");

	/* Disconnect the client. */
	gl_client_disconnect();

	/* Stop the server. */
	gl_server_stop();

	/* Don't let the signal propagate. */
	signal(sig, SIG_IGN);
}

/**
 * Handles the server started event.
 *
 * @param server Server handle object.
 */
void server_event_started(const server_t *server) {
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
int server_event_conn_req(const char *fname, uint32_t fsize) {
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
void server_event_stopped(void) {
	printf("Server stopped\n");
}

/**
 * Handles the server connection accepted event.
 *
 * @param conn Client connection handle object.
 */
void server_conn_event_accepted(const server_conn_t *conn) {
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
void server_conn_event_download_progress(const gl_server_conn_progress_t *progress) {
	printf("Receiving file... (%u/%u)\n", progress->recv_bytes,
		   progress->fsize);
}

/**
 * Handles the server connection download finished event.
 *
 * @param fpath Path of the downloaded file.
 */
void server_conn_event_download_success(const char *fpath) {
	printf("Finished receiving %s\n", fpath);
}

/**
 * Handles the server connection closed event.
 */
void server_conn_event_closed(void) {
	printf("Client connection closed\n");
}

/**
 * Handles the client connected event.
 *
 * @param client Client connection handle object.
 */
void client_event_connected(const tcp_client_t *client) {
	char *ipstr;

	/* Print some information about the current state of the connection. */
	ipstr = tcp_client_get_ipstr(client);
	printf("Client connected to server on %s port %u\n", ipstr,
		   ntohs(client->addr_in.sin_port));
	printf("Waiting for the server to accept the file...\n");

	free(ipstr);
}

/**
 * Handles the client's connection request response event.
 *
 * @param fname    Name of the file that was uploaded.
 * @param accepted Has the connection been accepted by the server?
 */
void client_event_conn_req_resp(const char *fname, bool accepted) {
	if (accepted) {
		printf("Server accepted receiving %s\n", fname);
	} else {
		printf("Server refused receiving %s\n", fname);
	}
}

/**
 * Handles the client's file upload progress event.
 *
 * @param progress Structure containing all the information about the progress.
 */
void client_event_send_progress(const gl_client_progress_t *progress) {
	printf("Sending %s (%u/%lu)\n", progress->bname,
		   progress->sent_chunk * progress->csize, progress->fsize);
}

/**
 * Handles the client's file upload succeeded event.
 *
 * @param fname Name of the file that was uploaded.
 */
void client_event_send_success(const char *fname) {
	printf("Finished sending %s\n", fname);
}

/**
 * Handles the client disconnection event.
 *
 * @param client Client connection handle object.
 */
void client_event_disconnected(const tcp_client_t *client) {
	char *ipstr;

	/* Print some information about the disconnection. */
	ipstr = tcp_client_get_ipstr(client);
	printf("Client disconnected from server at %s\n", ipstr);

	free(ipstr);
}
