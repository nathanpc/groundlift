/**
 * glsend.c
 * GroundLift's command-line sender client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <tchar.h>
	#ifdef _DEBUG
		#include <crtdbg.h>
	#endif /* _DEBUG */
	#include "../win32/cvtutf/Unicode.h"
#endif /* _WIN32 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>

#include "defaults.h"
#include "logging.h"
#include "sockets.h"
#include "request.h"
#include "utils.h"

/**
 * Configuration options passed as command line arguments.
 */
typedef struct {
	const char *addr;
	const char *port;
	const char *fpath;
	size_t len;
	char type;
} opts_t;

/* Private functions. */
void cancel_request(void);
bool send_url(const char *addr, const char *port, const char *url);
bool send_file(const char *addr, const char *port, const char *fpath);
bool send_text(const char *addr, const char *port, const char *text,
               size_t len);
reply_t *process_server_reply(const sockfd_t *sockfd);
size_t client_file_transfer(const sockfd_t *sockfd, const reqline_t *reqline,
                            const char *fpath);
size_t client_text_transfer(const sockfd_t *sockfd, const char *text,
                            size_t len);
bool perform_request(const char *addr, const char *port, reqline_t *reqline,
                     reply_t **reply);
void print_reply_error(const reply_t *reply);
void print_transfer_error(const char *type);
void sigint_handler(int sig);
void usage(const char *prog);
#ifdef _WIN32
BOOL WINAPI ConsoleSignalHandler(DWORD dwCtrlType);
#endif /* _WIN32 */

/* State variables. */
static sockfd_t sockfd_client;
static bool running;
static opts_t opts;

/**
 * Program's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Command line arguments passed.
 *
 * @return Application's return code.
 */
int main(int argc, char **argv) {
	char *text;
	int ret;
	int opt;

#if defined(WIN32) && defined(_DEBUG)
	/* Initialize memory leak detection. */
	_CrtMemState snapBegin;
	_CrtMemState snapEnd;
	_CrtMemState snapDiff;
	_CrtMemCheckpoint(&snapBegin);
#endif /* _WIN32 & _DEBUG */

	/* Initialize defaults and subsystems. */
	ret = 0;
	text = NULL;
	running = false;
	if (!socket_init()) {
		ret = 1;
		goto cleanup;
	}

#ifdef _WIN32
	/* Setup console signal handler. */
	SetConsoleCtrlHandler(&ConsoleSignalHandler, TRUE);

	/* Ensure we have a sane Unicode environment. */
	if (!UnicodeAssumptionsCheck()) {
		ret = 1;
		goto cleanup;
	}
#endif /* _WIN32 */

	/* Catch the interrupt signal from the console. */
	signal(SIGINT, sigint_handler);

	/* Populates the command line options object with defaults. */
	opts.addr = NULL;
	opts.port = GL_SERVER_PORT;
	opts.fpath = NULL;
	opts.len = 0;
	opts.type = REQ_TYPE_FILE;

	/* Handle command line arguments. */
	while ((opt = getopt(argc, argv, "p:uth")) != -1) {
		switch (opt) {
			case 'p':
				opts.port = optarg;
				break;
			case 'u':
				opts.type = REQ_TYPE_URL;
				break;
			case 't':
				opts.type = REQ_TYPE_TEXT;
				break;
			case '?':
				ret = 1;
				/* fallthrough */
			case 'h':
				usage(argv[0]);
				goto cleanup;
			default:
				log_printf(LOG_ERROR, "Something unexpected happened while "
					"parsing command line arguments (%c/%c)", opt, optopt);
				ret = 1;
				goto cleanup;
		}
	}

	/* Warn user about ignored arguments. */
	opt = 0;
	while (optind < argc) {
		switch (opt) {
			case 0:
				opts.addr = argv[optind++];
				break;
			case 1:
				opts.fpath = argv[optind++];
				opts.len = strlen(opts.fpath);
				break;
			default:
				fprintf(stderr, "%s: unknown argument -- %s (ignored)\n",
					argv[0], argv[optind++]);
				ret = 1;
		}

		opt++;
	}

	/* Check if we have all the required arguments. */
	if (opt < 2) {
		ret = 1;
		usage(argv[0]);
		goto cleanup;
	}

	/* Check if the user wants us to read from STDIN. */
	if ((opts.len == 1) && (*opts.fpath == '-')) {
		opts.len = read_stdin(&text, opts.type != REQ_TYPE_TEXT);
		opts.fpath = text;
	}

	/* Send request to the server. */
	switch (opts.type) {
		case REQ_TYPE_FILE:
			send_file(opts.addr, opts.port, opts.fpath);
			break;
		case REQ_TYPE_URL:
			send_url(opts.addr, opts.port, opts.fpath);
			break;
		case REQ_TYPE_TEXT:
			send_text(opts.addr, opts.port, opts.fpath, opts.len);
			break;
		default:
			log_printf(LOG_ERROR, "Unknown request type to send to server");
			ret = 1;
			break;
	}

cleanup:
	/* Clean up temporary stuff. */
	running = false;
	if (text) {
		free(text);
		text = NULL;
	}

#ifdef _WIN32
	/* Clean up Winsock stuff. */
	WSACleanup();

#ifdef _DEBUG
	/* Detect memory leaks. */
	_CrtMemCheckpoint(&snapEnd);
	if (_CrtMemDifference(&snapDiff, &snapBegin, &snapEnd)) {
		OutputDebugString(_T("*********** MEMORY LEAKS DETECTED ***********\r\n"));
		OutputDebugString(_T("----------- _CrtMemDumpStatistics ---------\r\n"));
		_CrtMemDumpStatistics(&snapDiff);
		OutputDebugString(_T("----------- _CrtMemDumpAllObjectsSince ---------\r\n"));
		_CrtMemDumpAllObjectsSince(&snapBegin);
		OutputDebugString(_T("----------- _CrtDumpMemoryLeaks ---------\r\n"));
		_CrtDumpMemoryLeaks();
	} else {
		OutputDebugString(_T("No memory leaks detected. Congratulations!\r\n"));
	}
#endif /* _DEBUG */
#endif /* _WIN32 */

	return ret;
}

/**
 * Cancels a request in the middle of it.
 */
void cancel_request(void) {
	/* Do we even have anything to do? */
	if (!running)
		return;

	/* Close the socket forcefully. */
	socket_close(sockfd_client, true);
	sockfd_client = SOCKERR;
	running = false;
}

/**
 * Handles the entire process of sending an URL to a server.
 *
 * @param addr Address of the server to send the file to.
 * @param port Port used to communicate with the server.
 * @param url  URL to be sent.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
bool send_url(const char *addr, const char *port, const char *url) {
	reqline_t *reqline;
	reply_t *reply;
	bool ret;

	/* Initialize variables. */
	reply = NULL;

	/* Build request line object. */
	reqline = reqline_new();
	if (reqline == NULL)
		return false;
	reqline_type_set(reqline, REQ_TYPE_URL);
	reqline->name = strdup(url);
	reqline->size = strlen(url);

	/* Connect to the server. */
	ret = perform_request(addr, port, reqline, &reply);
	if (!ret)
		goto cleanup;

	/* Check if the server replied with an error. */
	if (reply->code != 200) {
		print_reply_error(reply);
		ret = false;
		goto cleanup;
	}

cleanup:
	/* Free request line object and close the socket. */
	reqline_free(reqline);
	reply_free(reply);
	if (sockfd_client != SOCKERR) {
		socket_close(sockfd_client, true);
		sockfd_client = SOCKERR;
	}
	running = false;

	return ret;
}

/**
 * Handles the entire process of sending a file to a server.
 *
 * @param addr  Address of the server to send the file to.
 * @param port  Port used to communicate with the server.
 * @param fpath Path of the file to be sent.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
bool send_file(const char *addr, const char *port, const char *fpath) {
	reqline_t *reqline;
	reply_t *reply;
	bool ret;

	/* Initialize variables. */
	reply = NULL;

	/* Check if the file actually exists. */
	if (!file_exists(fpath)) {
		log_printf(LOG_ERROR, "File \"%s\" does not exist", fpath);
		return false;
	}

	/* Build request line object. */
	reqline = reqline_new();
	if (reqline == NULL)
		return false;
	reqline_type_set(reqline, REQ_TYPE_FILE);
	reqline->size = file_size(fpath);
	reqline->name = path_basename(fpath);

	/* Connect to the server. */
	ret = perform_request(addr, port, reqline, &reply);
	if (!ret)
		goto cleanup;

	/* Check if the server replied with an error. */
	if (reply->code != 100) {
		print_reply_error(reply);
		ret = false;
		goto cleanup;
	}

	/* Send the file contents. */
	if (!client_file_transfer(&sockfd_client, reqline, fpath)) {
		log_printf(LOG_NOTICE, "File transfer %s", (running) ? "failed" :
			"canceled");
		ret = false;
		goto cleanup;
	}

cleanup:
	/* Free request line object and close the socket. */
	reqline_free(reqline);
	reply_free(reply);
	if (sockfd_client != SOCKERR) {
		socket_close(sockfd_client, true);
		sockfd_client = SOCKERR;
	}
	running = false;

	return ret;
}

/**
 * Handles the entire process of sending text to a server.
 *
 * @param addr Address of the server to send the file to.
 * @param port Port used to communicate with the server.
 * @param text Text content to be sent.
 * @param len  Length of the text to be sent over, not including the NUL
 *             terminator.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
bool send_text(const char *addr, const char *port, const char *text,
               size_t len) {
	reqline_t *reqline;
	reply_t *reply;
	bool ret;

	/* Initialize variables. */
	reply = NULL;

	/* Build request line object. */
	reqline = reqline_new();
	if (reqline == NULL)
		return false;
	reqline_type_set(reqline, REQ_TYPE_TEXT);
	reqline->size = len;
	reqline->name = NULL;

	/* Connect to the server. */
	ret = perform_request(addr, port, reqline, &reply);
	if (!ret)
		goto cleanup;

	/* Check if the server replied with an error. */
	if (reply->code != 100) {
		print_reply_error(reply);
		ret = false;
		goto cleanup;
	}

	/* Send text contents to server. */
	if (!client_text_transfer(&sockfd_client, text, len)) {
		log_printf(LOG_NOTICE, "Text transfer %s", (running) ? "failed" :
			"canceled");
		ret = false;
		goto cleanup;
	}

cleanup:
	/* Free request line object and close the socket. */
	reqline_free(reqline);
	reply_free(reply);
	if (sockfd_client != SOCKERR) {
		socket_close(sockfd_client, true);
		sockfd_client = SOCKERR;
	}
	running = false;

	return ret;
}

/**
 * Processes the server's reply to a request.
 *
 * @param sockfd Socket connected to a server.
 *
 * @return Server reply object if the operation was successful, or NULL if any
 *         errors occurred during the transfer.
 */
reply_t *process_server_reply(const sockfd_t *sockfd) {
	char line[GL_REPLYLINE_MAX + 1];
	ssize_t len;
	int i;

	/* Read the reply from the server. */
	if ((len = recv(*sockfd, line, GL_REPLYLINE_MAX, 0)) < 0)
		return NULL;
	line[len] = '\0';

	/* Ensure the reply wasn't too long. */
	if (len >= GL_REPLYLINE_MAX) {
		log_printf(LOG_WARNING, "Reply from server unusually long. Aborting");
		return NULL;
	}

	/* Terminate the reply before the CRLF. */
	for (i = 0; i < len; i++) {
		if ((line[i] == '\r') || (line[i] == '\n')) {
			line[i] = '\0';
			break;
		}
	}

#ifdef _DEBUG
	/* Print reply for debugging. */
	log_printf(LOG_INFO, "Server reply: %s", line);
#endif /* _DEBUG */

	/* Parse reply from server and return it. */
	return reply_parse(line);
}

/**
 * Connects to the server, sends the request line, waits, and processes the
 * server's reply.
 *
 * @param addr    Address of the server to connect to.
 * @param port    Port to connect to the server on.
 * @param reqline Request line object to be sent over to the server.
 * @param reply   Where to store the parsed reply from the server. May be NULL
 *                if an error occurred during parsing.
 *
 * @return TRUE if the entire operation was successful, FALSE otherwise.
 */
bool perform_request(const char *addr, const char *port, reqline_t *reqline,
                     reply_t **reply) {
	bool ret = true;

	/* Check if we have a valid reply object pointer. */
	if (reply == NULL) {
		log_printf(LOG_CRIT, "Reply object pointer cannot be NULL");
		return false;
	}

	/* Connect to the server. */
	running = true;
	sockfd_client = socket_new_client(addr, port);
	if (sockfd_client == SOCKERR)
		return false;
	log_printf(LOG_INFO, "Connected to the server on %s:%s", addr, port);

	/* Send request line. */
	if (reqline_send(sockfd_client, reqline) == 0)
		return false;
	log_printf(LOG_INFO, "Sent %s request", reqline->stype);

	/* Wait and process the server reply. */
	*reply = process_server_reply(&sockfd_client);
	if (*reply == NULL)
		return false;

#ifdef _DEBUG
	/* Print parsed reply for debugging. */
	log_printf(LOG_INFO, "Parsed server reply: (%u) [%s] \"%s\"",
		(*reply)->code, (*reply)->type, (*reply)->msg);
#endif /* _DEBUG */

	return ret;
}

/**
 * Prints out error messages based on a server's reply.
 *
 * @param reply Server reply object.
 */
void print_reply_error(const reply_t *reply) {
	switch (reply->code) {
		case ERR_CODE_REQ_REFUSED:
			log_printf(LOG_NOTICE, "User refused the request");
			break;
		default:
			log_printf(LOG_ERROR, "Server replied with error: [%u %s] %s",
				reply->code, reply->type, reply->msg);
			break;
	}
}

/**
 * Dumps the contents of a file through a TCP socket connection.
 *
 * @param sockfd  Socket connection to a server that's ready to receive this.
 * @param reqline Request line object sent to the server.
 * @param fpath   Path to the file to be dumped over a socket.
 *
 * @return Number of bytes transferred or 0 in case of an error.
 */
size_t client_file_transfer(const sockfd_t *sockfd, const reqline_t *reqline,
                            const char *fpath) {
	FILE *fh;
	size_t len;
	size_t acclen;
	uint8_t buf[SEND_BUF_LEN];

	/* Open file for reading. */
	fh = fopen(fpath, "rb");
	if (fh == NULL) {
		log_printf(LOG_ERROR, "Failed to open file \"%s\" for sending", fpath);
		return 0;
	}

	/* Pipe file contents straight to socket. */
	acclen = 0;
	buffered_progress(reqline->name, acclen, reqline->size);
	while ((len = fread(buf, sizeof(uint8_t), SEND_BUF_LEN, fh)) > 0) {
		if (send(*sockfd, buf, len, 0) < 0) {
			print_transfer_error("file");
			acclen = 0;
			break;
		}

		/* Increment the accumulated length and display the progress. */
		acclen += len;
		buffered_progress(reqline->name, acclen, reqline->size);
	}

	/* Ensure we go to a new line before continuing to preserve the progress. */
	if (acclen > 0)
		fprintf(stderr, "\n");

	/* Close the file handle and return. */
	fclose(fh);
	return acclen;
}

/**
 * Dumps text through a TCP socket connection.
 *
 * @param sockfd Socket connection to a server that's ready to receive this.
 * @param text   Text content to be dumped over a socket.
 * @param len    Length of the text to be sent over, not including the NUL
 *               terminator.
 *
 * @return Number of bytes transferred or 0 in case of an error.
 */
size_t client_text_transfer(const sockfd_t *sockfd, const char *text,
                            size_t len) {
	size_t slen;
	size_t acclen;
	const char *buf;

	/* Pipe text contents straight to socket. */
	acclen = 0;
	buf = text;
	buffered_progress("Text", acclen, len);
	while (*buf != '\0') {
		/* Determine the length of string to send. */
		slen = SEND_BUF_LEN;
		if ((acclen + slen) > len)
			slen = len - acclen;

		/* Send the string over. */
		if (send(*sockfd, buf, slen, 0) < 0) {
			print_transfer_error("text");
			acclen = 0;
			break;
		}

		/* Accumulate length, move cursor forward, and display the progress. */
		acclen += slen;
		buf += slen;
		buffered_progress("Text", acclen, len);
	}

	/* Ensure we go to a new line before continuing to preserve the progress. */
	if (acclen > 0)
		fprintf(stderr, "\n");

	return acclen;
}

/**
 * Prints out transfer error messages.
 *
 * @param type Transfer type to be concatenated within the error message.
 */
void print_transfer_error(const char *type) {
	fprintf(stderr, "\n");

	/* Check if the transfer has been canceled. */
	if (!running) {
#ifdef _DEBUG
		log_sockerr(LOG_ERROR, "Transfer canceled. Suppressed error");
#endif /* _DEBUG */
		return;
	}

	/* An error occurred during sending. */
#ifdef _WIN32
	log_sockerr(LOG_ERROR, "Failed to pipe contents of %s to socket", type);
#else
	if (sockerrno == EPIPE) {
		log_sockerr(LOG_WARNING, "Server closed connection before %s transfer "
			"finished", type);
	} else {
		log_sockerr(LOG_ERROR, "Failed to pipe contents of %s to socket", type);
	}
#endif /* _WIN32 */
}

/**
 * Handles the SIGINT interrupt event.
 *
 * @param sig Signal handle that generated this interrupt.
 */
void sigint_handler(int sig) {
#ifdef _DEBUG
	log_printf(LOG_INFO, "Received a SIGINT");
#endif /* _DEBUG */

	/* Cancel the request. */
	cancel_request();

	/* Don't let the signal propagate. */
	signal(sig, SIG_IGN);
}

#ifdef _WIN32
/**
 * Handles console control signals, similar to signal_handler, but for the
 * command prompt window in Windows.
 *
 * @param dwCtrlType Control signal type.
 *
 * @return TRUE if we handled the control signal, FALSE otherwise.
 */
BOOL WINAPI ConsoleSignalHandler(DWORD dwCtrlType) {
	switch (dwCtrlType) {
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			cancel_request();
			return TRUE;
	}

	return FALSE;
}
#endif /* _WIN32 */

/**
 * Displays the usage help message.
 *
 * @param prog Program's name from argv[0].
 */
void usage(const char *prog) {
	printf("usage: %s [-p port] [-u] [-t] addr attach\n\n", prog);
	puts("arguments:");
	puts("    addr       Address where the server is listening on");
	puts("    attach     File, URL or text to send to the server. If a '-' "
	     "(dash) is ");
	puts("               supplied, the content is read from STDIN until EOF");
	puts("");
	puts("options:");
	puts("    -h         Displays this message");
	puts("    -p port    Port the server is listening on");
	puts("    -t         Send text instead of a file");
	puts("    -u         Send a URL instead of a file");
	puts("");
	puts(GL_COPYRIGHT);
}
