/**
 * client.c
 * GroundLift command-line client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "client.h"

#include <string.h>
#include <groundlift/error.h>
#include <groundlift/defaults.h>

/* Public variables. */
client_handle_t *g_client;

/**
 * Perform an entire send exchange with the server.
 *
 * @param ip    Server's IP to send the data to.
 * @param fname Path to a file to be sent to the server.
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
gl_err_t *client_send(const char *ip, const char *fname) {
	gl_err_t *err = NULL;

	/* Construct the client handle object. */
	g_client = gl_client_new();
	if (g_client == NULL) {
		return gl_error_push_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to construct the client handle object"));
	}

	/* Set up event handlers. */
#if 0
	gl_client_evt_conn_set(g_client, event_connected, NULL);
	gl_client_evt_conn_req_resp_set(g_client, event_conn_req_resp, NULL);
	gl_client_evt_put_progress_set(g_client, event_send_progress, NULL);
	gl_client_evt_put_succeed_set(g_client, event_send_success, NULL);
	gl_client_evt_disconn_set(g_client, event_disconnected, NULL);
#endif

	/* Set up the client. */
	err = gl_client_connect(g_client, ip, GL_SERVER_MAIN_PORT);
	if (err)
		goto cleanup;

	/* Send the file request and wait. */
	err = gl_client_send_file(g_client, fname);
	if (err)
		goto cleanup;

cleanup:
	/* Free up any resources. */
	gl_client_free(g_client);
	g_client = NULL;

	return err;
}

/**
 * Lists the peers throughout all of the network interfaces.
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
gl_err_t *client_list_peers(void) {
	gl_peer_list_t *peers;
	gl_err_t *err;
	uint16_t i;
	uint8_t max_host_len;

	/* Get the peer list. */
	peers = NULL;
	err = gl_client_discover_peers(&peers);
	if (err)
		return err;

	/* Get the length of the longest hostname. */
	max_host_len = 0;
	for (i = 0; i < peers->count; i++) {
		size_t hlen = strlen(peers->list[i]->head.hostname);
		if (hlen > max_host_len)
			max_host_len = (uint8_t)hlen;
	}

	/* Print the table header. */
	printf("%-*s   %-15s   Type\n", max_host_len, "Hostname", "IP Address");

	/* Go through the peer list. */
	for (i = 0; i < peers->count; i++) {
		char *ipaddr;
		glproto_discovery_msg_t *peer = peers->list[i];

		/* Get human-readable IP address. */
		socket_tostr(&ipaddr, peer->head.sock);
		printf("%-*s   %-15s   %s\n", max_host_len, peer->head.hostname,
		       ipaddr, peer->head.device);
		free(ipaddr);
	}

	/* Free up any resources. */
	gl_peer_list_free(peers);

	return NULL;
}

#if 0
/**
 * Handles the client connected event.
 *
 * @param client Client connection handle object.
 * @param arg    Optional data set by the event handler setup.
 */
void event_connected(const tcp_client_t *client, void *arg) {
	char *ipstr;

	/* Ignore unused arguments. */
	(void)arg;

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
 * @param fb       File bundle that was uploaded.
 * @param accepted Has the connection been accepted by the server?
 * @param arg      Optional data set by the event handler setup.
 */
void event_conn_req_resp(const file_bundle_t *fb, bool accepted, void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	if (accepted) {
		printf("Server accepted receiving %s\n", fb->base);
	} else {
		printf("Server refused receiving %s\n", fb->base);
	}
}

/**
 * Handles the client's file upload progress event.
 *
 * @param progress Structure containing all the information about the progress.
 * @param arg      Optional data set by the event handler setup.
 */
void event_send_progress(const gl_client_progress_t *progress, void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	printf("Sending %s (%u/%lu)\n", progress->fb->base, progress->sent_bytes,
		   progress->fb->size);
}

/**
 * Handles the client's file upload succeeded event.
 *
 * @param fb  File bundle that was uploaded.
 * @param arg Optional data set by the event handler setup.
 */
void event_send_success(const file_bundle_t *fb, void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	printf("Finished sending %s\n", fb->base);
}

/**
 * Handles the client disconnection event.
 *
 * @param client Client connection handle object.
 * @param arg    Optional data set by the event handler setup.
 */
void event_disconnected(const tcp_client_t *client, void *arg) {
	char *ipstr;

	/* Ignore unused arguments. */
	(void)arg;

	/* Print some information about the disconnection. */
	ipstr = tcp_client_get_ipstr(client);
	printf("Client disconnected from server at %s\n", ipstr);

	free(ipstr);
}

/**
 * Handles the peer discovered event.
 *
 * @param peer Discovered peer object.
 * @param arg  Optional data set by the event handler setup.
 */
void event_peer_discovered(const gl_discovery_peer_t *peer, void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	/* Display the discovered peer. */
	printf("Discovered peer '%s' (%s)\n", peer->name,
		   inet_ntoa(peer->sock->addr_in.sin_addr));
}
#endif
