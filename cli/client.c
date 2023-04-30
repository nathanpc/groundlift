/**
 * client.c
 * GroundLift command-line client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "client.h"

#include <groundlift/defaults.h>

/* Client event handlers. */
void event_connected(const tcp_client_t *client);
void event_conn_req_resp(const char *fname, bool accepted);
void event_send_progress(const gl_client_progress_t *progress);
void event_send_success(const char *fname);
void event_disconnected(const tcp_client_t *client);
void event_peer_discovered(const char *name, const struct sockaddr *addr);

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
	err = gl_client_init(ip, port);
	if (err)
		goto cleanup;

	/* Setup event handlers. */
	gl_client_evt_conn_set(event_connected);
	gl_client_evt_conn_req_resp_set(event_conn_req_resp);
	gl_client_evt_put_progress_set(event_send_progress);
	gl_client_evt_put_succeed_set(event_send_success);
	gl_client_evt_disconn_set(event_disconnected);

	/* Connect to the server. */
	err = gl_client_connect(fname);
	if (err)
		goto cleanup;

	/* Wait for it to return. */
	err = gl_client_thread_join();
	if (err) {
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

	/* Setup event handlers. */
	gl_client_evt_discovery_peer_set(event_peer_discovered);

	/* Send discovery broadcast and listen to replies. */
	printf("Sending discovery broadcast...\n");
	err = gl_client_discover_peers(UDPSERVER_PORT);
	if (err)
		return err;

	/* Wait for the discovery thread to return. */
	err = gl_client_discovery_thread_join();
	if (err)
		return err;
	printf("Finished trying to discover peers.\n");

	return NULL;
}

/**
 * Handles the client connected event.
 *
 * @param client Client connection handle object.
 */
void event_connected(const tcp_client_t *client) {
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
void event_conn_req_resp(const char *fname, bool accepted) {
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
void event_send_progress(const gl_client_progress_t *progress) {
	printf("Sending %s (%u/%lu)\n", progress->bname,
		   progress->sent_chunk * progress->csize, progress->fsize);
}

/**
 * Handles the client's file upload succeeded event.
 *
 * @param fname Name of the file that was uploaded.
 */
void event_send_success(const char *fname) {
	printf("Finished sending %s\n", fname);
}

/**
 * Handles the client disconnection event.
 *
 * @param client Client connection handle object.
 */
void event_disconnected(const tcp_client_t *client) {
	char *ipstr;

	/* Print some information about the disconnection. */
	ipstr = tcp_client_get_ipstr(client);
	printf("Client disconnected from server at %s\n", ipstr);

	free(ipstr);
}

/**
 * Handles the peer discovered event.
 *
 * @param name Hostname of the peer.
 * @param addr IP address information of the peer.
 */
void event_peer_discovered(const char *name, const struct sockaddr *addr) {
	const struct sockaddr_in *addr_in;

	/* Convert our address structure. */
	addr_in = (const struct sockaddr_in *)addr;

	/* Display the discovered peer. */
	printf("Discovered peer '%s' (%s)\n", name, inet_ntoa(addr_in->sin_addr));
}
