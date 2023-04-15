/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <groundlift/client.h>
#include <groundlift/defaults.h>
#include <groundlift/error.h>
#include <groundlift/obex.h>
#include <groundlift/server.h>
#include <groundlift/tcp.h>
#include <groundlift/utf16utils.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private methods. */
gl_err_t *client_send(const char *ip, uint16_t port, char *fname);
void sigint_handler(int sig);
void server_event_started(const server_t *server);
void server_conn_event_accepted(const server_conn_t *conn);
void server_conn_event_closed(void);
void server_event_stopped(void);
void client_event_connected(const tcp_client_t *client);
void client_event_conn_closed(const tcp_client_t *client);
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
		gl_server_conn_evt_accept_set(server_conn_event_accepted);
		gl_server_conn_evt_close_set(server_conn_event_closed);
		gl_server_evt_stop_set(server_event_stopped);

		/* Start it up. */
		if (!gl_server_start()) {
			fprintf(stderr, "Failed to start the server.\n");

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
	} else if (argv[1][0] == 'o') {
		obex_header_t *header;
		wchar_t *wbuf;

		/* Byte */
		header = obex_header_new(OBEX_HEADER_ACTION_ID);
		header->value.byte = 0x23;
		obex_print_header(header, true);
		obex_header_free(header);

		/* Word 64-bit */
		header = obex_header_new(OBEX_HEADER_COUNT);
		header->value.word32 = 1234567890;
		obex_print_header(header, false);
		obex_header_free(header);

		/* String */
		header = obex_header_new(OBEX_HEADER_TYPE);
		obex_header_string_copy(header, "text/plain");
		obex_print_header(header, false);
		obex_header_free(header);

		/* UTF-16 String */
		header = obex_header_new(OBEX_HEADER_NAME);
		wbuf = (wchar_t *)malloc((strlen("jumar.txt") + 1) * sizeof(wchar_t));
		/* Simulating a UTF-16 string being put into a 32-bit wchar_t. */
		wbuf[0] = ('j' << 16) | 'u';
		wbuf[1] = ('m' << 16) | 'a';
		wbuf[2] = ('r' << 16) | '.';
		wbuf[3] = ('t' << 16) | 'x';
		wbuf[4] = ('t' << 16) | 0;
		utf16_wchar32_fix(wbuf);
		obex_header_wstring_copy(header, wbuf);
		obex_print_header(header, false);
		obex_header_free(header);

		printf("\n");
	} else if (argv[1][0] == 'p') {
		obex_packet_t *packet;
		void *buf;

		/* Create a new packet. */
		packet = obex_packet_new_connect();

		/* Convert the packet into a network buffer. */
		if (!obex_packet_encode(packet, &buf)) {
			fprintf(stderr, "Failed to encode the packet for transferring.\n");
			obex_packet_free(packet);

			return 1;
		}

		/* Print all the information we can get. */
		obex_print_packet(packet);
		printf("Network buffer:\n");
		tcp_print_net_buffer(buf, packet->size);
		printf("\n");

		obex_packet_free(packet);
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
	gl_error_free(err);

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
	gl_client_evt_close_set(client_event_conn_closed);
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
	printf("Server listening on %s port %u\n", ipstr,
		   ntohs(server->addr_in.sin_port));

	free(ipstr);
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
 * Handles the server connection closed event.
 */
void server_conn_event_closed(void) {
	printf("Client connection closed\n");
}

/**
 * Handles the server stopped event.
 */
void server_event_stopped(void) {
	printf("Server stopped\n");
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

	free(ipstr);
}

/**
 * Handles the client connection closed event.
 *
 * @param client Client connection handle object.
 */
void client_event_conn_closed(const tcp_client_t *client) {
	char *ipstr;

	/* Print some information about the connection closing. */
	ipstr = tcp_client_get_ipstr(client);
	printf("Connection closed by the server at %s:%u\n", ipstr,
		   ntohs(client->addr_in.sin_port));

	free(ipstr);
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
