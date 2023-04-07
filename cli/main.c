/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <groundlift/client.h>
#include <groundlift/defaults.h>
#include <groundlift/obex.h>
#include <groundlift/server.h>
#include <groundlift/utf16utils.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private methods. */
void sigint_handler(int sig);

/**
 * Program's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Command line arguments passed.
 *
 * @return Application's return code.
 */
int main(int argc, char **argv) {
	int ret;

	/* Setup our defaults. */
	ret = 0;

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
	} else if ((argc == 4) && (argv[1][0] == 'c')) {
		/* Initialize the client. */
		if (!gl_client_init(argv[2], (uint16_t)atoi(argv[3]))) {
			fprintf(stderr, "Failed to initialize the client.\n");

			ret = 1;
			goto cleanup;
		}

		/* Connect to the server. */
		if (!gl_client_connect()) {
			fprintf(stderr, "Failed to connect the client to the server.\n");

			ret = 1;
			goto cleanup;
		}

		/* Wait for it to return. */
		if (!gl_client_thread_join()) {
			fprintf(stderr, "Client thread returned with errors.\n");

			ret = 1;
			goto cleanup;
		}
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
	/* Free up any resources. */
	gl_server_free();
	gl_client_free();

	return ret;
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
