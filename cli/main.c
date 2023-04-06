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
		obex_header_t header;

		/* Byte */
		header.identifier.id = OBEX_HEADER_ACTION_ID;
		header.value.byte = 0x23;
		obex_print_header(&header);
		printf("\n");

		/* Word 64-bit */
		header.identifier.id = OBEX_HEADER_COUNT;
		header.value.word64 = 1234567890;
		obex_print_header(&header);
		printf("\n");

		/* String */
		header.identifier.id = OBEX_HEADER_TYPE;
		header.value.string.fhlength = strlen("text/plain") + 1;
		header.value.string.text =
			(char *)malloc(header.value.string.fhlength * sizeof(char));
		header.value.string.fhlength += 3;
		strcpy(header.value.string.text, "text/plain");
		obex_print_header(&header);
		free(header.value.string.text);
		header.value.string.text = NULL;
		printf("\n");

		/* UTF-16 String */
		header.identifier.id = OBEX_HEADER_NAME;
		header.value.wstring.fhlength = strlen("jumar.txt") + 1;
		header.value.wstring.text =
			(wchar_t *)malloc(header.value.wstring.fhlength * sizeof(wchar_t));
		/* Simulating a UTF-16 string being put into a 32-bit wchar_t. */
		header.value.wstring.text[0] = ('j' << 16) | 'u';
		header.value.wstring.text[1] = ('m' << 16) | 'a';
		header.value.wstring.text[2] = ('r' << 16) | '.';
		header.value.wstring.text[3] = ('t' << 16) | 'x';
		header.value.wstring.text[4] = ('t' << 16) | 0;
		utf16_wchar32_fix(header.value.wstring.text);
		header.value.wstring.fhlength *= 2;
		header.value.wstring.fhlength += 3;
		obex_print_header(&header);
		free(header.value.wstring.text);
		header.value.wstring.text = NULL;
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
