/**
 * client.c
 * GroundLift command-line client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "client.h"

#include <groundlift/client.h>
#include <groundlift/defaults.h>
#include <groundlift/error.h>
#include <utils/logging.h>

/* Public variables. */
client_handle_t *g_client;
discovery_client_t *g_discovery_client;

/* Client event handlers. */
void event_connected(const tcp_client_t *client, void *arg);
void event_conn_req_resp(const file_bundle_t *fb, bool accepted, void *arg);
void event_send_progress(const gl_client_progress_t *progress, void *arg);
void event_send_success(const file_bundle_t *fb, void *arg);
void event_disconnected(const tcp_client_t *client, void *arg);
void event_peer_discovered(const gl_discovery_peer_t *peer, void *arg);

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
gl_err_t *client_send(const char *ip, uint16_t port, const char *fname) {
	gl_err_t *err = NULL;

	/* Construct the client handle object. */
	g_client = gl_client_new();
	if (g_client == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to construct the client handle object"));
	}

	/* Setup event handlers. */
	gl_client_evt_conn_set(g_client, event_connected, NULL);
	gl_client_evt_conn_req_resp_set(g_client, event_conn_req_resp, NULL);
	gl_client_evt_put_progress_set(g_client, event_send_progress, NULL);
	gl_client_evt_put_succeed_set(g_client, event_send_success, NULL);
	gl_client_evt_disconn_set(g_client, event_disconnected, NULL);

	/* Setup the client. */
	err = gl_client_setup(g_client, ip, port, fname);
	if (err) {
		log_printf(LOG_ERROR, "Client setup failed.\n");
		goto cleanup;
	}

	/* Connect to the server. */
	err = gl_client_connect(g_client);
	if (err) {
		log_printf(LOG_ERROR, "Client failed to start.\n");
		goto cleanup;
	}

	/* Wait for it to return. */
	err = gl_client_thread_join(g_client);
	if (err) {
		log_printf(LOG_ERROR, "Client thread returned with errors.\n");
		goto cleanup;
	}

cleanup:
	/* Free up any resources. */
	gl_client_free(g_client);
	g_client = NULL;

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

	/* Construct the discovery client handle object. */
	g_discovery_client = gl_client_discovery_new();
	if (g_discovery_client == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to construct the discovery client handle object"));
	}

	/* Setup event handlers and the discovery socket. */
	gl_client_evt_discovery_peer_set(g_discovery_client, event_peer_discovered,
									 NULL);
	err = gl_client_discovery_setup(g_discovery_client, UDPSERVER_PORT);
	if (err) {
		log_printf(LOG_ERROR, "Discovery service setup failed.\n");
		goto cleanup;
	}

	/* Send discovery broadcast and listen to replies. */
	printf("Sending discovery broadcast...\n");
	err = gl_client_discover_peers(g_discovery_client);
	if (err) {
		log_printf(LOG_ERROR, "Discovery service thread failed to start.\n");
		goto cleanup;
	}

	/* Wait for the discovery thread to return. */
	err = gl_client_discovery_thread_join(g_discovery_client);
	if (err)
		goto cleanup;
	printf("Finished trying to discover peers.\n");

cleanup:
	/* Clean things up. */
	gl_client_discovery_free(g_discovery_client);
	g_discovery_client = NULL;

	return err;
}

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
