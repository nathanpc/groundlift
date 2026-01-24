/**
 * glrecvd.c
 * GroundLift's command-line receiver server.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#ifdef DEBUG
		#include <crtdbg.h>
	#endif /* DEBUG */
#endif /* _WIN32 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>

#include "defaults.h"
#include "logging.h"
#include "sockets.h"
#include "request.h"
#include "utils.h"

/* Server status flags */
#define SERVER_RUNNING   0x01
#define CLIENT_CONNECTED 0x02

/**
 * Configuration options passed as command line arguments.
 */
typedef struct {
	const char *addr;
	const char *port;
	bool accept_all;
} opts_t;

/* Private functions. */
bool server_start(const char *addr, const char *port);
void server_stop(void);
void server_loop(int af, sockfd_t server);
void server_process_request(sockfd_t *sock);
bool process_file_req(const sockfd_t *sockfd, const reqline_t *reqline);
bool process_url_req(const sockfd_t *sockfd, const reqline_t *reqline);
bool process_text_req(const sockfd_t *sockfd, const reqline_t *reqline);
void sigint_handler(int sig);
void usage(const char *prog);
#ifdef _WIN32
BOOL WINAPI ConsoleSignalHandler(DWORD dwCtrlType);
#endif /* _WIN32 */

/* State variables. */
static uint8_t server_status;
static sockfd_t sockfd_server;
static sockfd_t sockfd_client;
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
	int ret;
	int opt;

#ifdef _WIN32
#ifdef _DEBUG
	/* Initialize memory leak detection. */
	_CrtMemState snapBegin;
	_CrtMemState snapEnd;
	_CrtMemState snapDiff;
	_CrtMemCheckpoint(&snapBegin);
#endif /* _DEBUG */

	/* Setup console signal handler. */
	SetConsoleCtrlHandler(&ConsoleSignalHandler, TRUE);
#endif /* _WIN32 */

	/* Initialize defaults and subsystems. */
	ret = 0;
	server_status = 0;
	sockfd_server = SOCKERR;
	sockfd_client = SOCKERR;
	if (!socket_init()) {
		ret = 1;
		goto cleanup;
	}

	/* Catch the interrupt signal from the console. */
	signal(SIGINT, sigint_handler);

	/* Populates the command line options object with defaults. */
	opts.addr = "0.0.0.0";
	opts.port = GL_SERVER_PORT;
	opts.accept_all = false;

	/* Handle command line arguments. */
	while ((opt = getopt(argc, argv, "l:p:yh")) != -1) {
		switch (opt) {
			case 'l':
				opts.addr = optarg;
				break;
			case 'p':
				opts.port = optarg;
				break;
			case 'y':
				opts.accept_all = true;
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
	while (optind < argc) {
		fprintf(stderr, "%s: unknown argument -- %s (ignored)\n", argv[0],
			argv[optind++]);
	}

	/* Run the server. */
	if (!server_start(opts.addr, opts.port)) {
		ret = 2;
		goto cleanup;
	}
	server_loop(AF_INET, sockfd_server);

cleanup:
	/* Stop our server. */
	server_stop();

#ifdef _WIN32
	/* Clean up Winsock stuff. */
	WSACleanup();

#ifdef _DEBUG
	/* Detect memory leaks. */
	_CrtMemCheckpoint(&snapEnd);
	if (_CrtMemDifference(&snapDiff, &snapBegin, &snapEnd)) {
		OutputDebugString("*********** MEMORY LEAKS DETECTED ***********\r\n");
		OutputDebugString("----------- _CrtMemDumpStatistics ---------\r\n");
		_CrtMemDumpStatistics(&snapDiff);
		OutputDebugString("----------- _CrtMemDumpAllObjectsSince ---------\r\n");
		_CrtMemDumpAllObjectsSince(&snapBegin);
		OutputDebugString("----------- _CrtDumpMemoryLeaks ---------\r\n");
		_CrtDumpMemoryLeaks();
	} else {
		OutputDebugString("No memory leaks detected. Congratulations!\r\n");
	}
#endif /* _DEBUG */
#endif /* _WIN32 */

	return ret;
}

/**
 * Starts up the server and listens for client requests.
 *
 * @param addr IP address to bind ourselves to.
 * @param port Port to bind ourselves to.
 *
 * @return TRUE if the startup was successful.
 */
bool server_start(const char *addr, const char *port) {
	/* Get the listening socket for our server. */
	sockfd_server = socket_new_server(addr, port);
	if (sockfd_server == SOCKERR)
		return false;

	/* Indicate that the server has started. */
	log_printf(LOG_INFO, "Server started on %s:%s", addr, port);
	server_status |= SERVER_RUNNING;

	return true;
}

/**
 * Stops the server immediately.
 */
void server_stop(void) {
	/* Do we even have anything to do? */
	if (!(server_status & SERVER_RUNNING))
		return;

	/* Stop the server. */
	log_printf(LOG_NOTICE, "Stopping the server...");
	server_status &= ~SERVER_RUNNING;
	if ((sockfd_server != SOCKERR) && (socket_close(sockfd_server, false)
			== SOCKERR)) {
		log_sockerr(LOG_ERROR, "Failed to close server socket");
	}
	sockfd_server = SOCKERR;

	/* Close client connection. */
	if (server_status & CLIENT_CONNECTED) {
		server_status &= ~CLIENT_CONNECTED;
		if (sockfd_client != SOCKERR)
			socket_close(sockfd_client, false);
		sockfd_client = SOCKERR;
	}
}

/**
 * Server listening loop.
 *
 * @param af     Socket address family.
 * @param server Socket in which the server is listening on.
 */
void server_loop(int af, sockfd_t server) {
	while (server_status & SERVER_RUNNING) {
		struct sockaddr_storage csa;
		sockfd_t *sock;
		socklen_t socklen;
		char addrstr[IPADDR_STRLEN];

		/* Accept the client connection. */
		sock = &sockfd_client;
		socklen = sizeof(csa);
		*sock = accept(server, (struct sockaddr*)&csa, &socklen);
		if (*sock == SOCKERR) {
			if ((server_status & SERVER_RUNNING) && (sockerrno != EWOULDBLOCK))
				log_sockerr(LOG_ERROR, "Server failed to accept a connection");
			server_status &= ~CLIENT_CONNECTED;
			continue;
		}
		server_status |= CLIENT_CONNECTED;

		/* Get client address string and announce connection. */
		if (inet_addr_str(af, &csa, addrstr) == NULL) {
			log_sockerr(LOG_ERROR, "Failed to get client address string");
		} else {
			log_printf(LOG_INFO, "Client connected from %s", addrstr);
		}

		/* Process the client's request. */
		server_process_request(sock);
	}
}

/**
 * Processes a client connection.
 *
 * @param sock Pointer to a client connection socket descriptor.
 */
void server_process_request(sockfd_t *sock) {
	char line[GL_REQLINE_MAX + 1];
	reqline_t *reqline;
	ssize_t len;
	int i;

	/* Initialize some defaults. */
	reqline = NULL;

	/* Read the line from client's request. */
	if ((len = recv(*sock, line, GL_REQLINE_MAX, 0)) < 0) {
		if (server_status & SERVER_RUNNING) {
			log_sockerr(LOG_ERROR, "Server failed to receive request line");
			send_error(*sock, ERR_CODE_INTERNAL);
		}
		goto close_conn;
	}
	line[len] = '\0';

	/* Ensure the request wasn't too long. */
	if (len >= GL_REQLINE_MAX) {
		log_printf(LOG_WARNING, "Request line unusually long, closing "
			"connection.");
		send_error(*sock, ERR_CODE_REQ_LONG);
		goto close_conn;
	}

	/* Terminate the request line before CRLF. */
	for (i = 0; i < len; i++) {
		if ((line[i] == '\r') || (line[i] == '\n')) {
			line[i] = '\0';
			break;
		}
	}

	/* Parse the request line. */
	reqline = reqline_parse(line);
	if (reqline == NULL) {
		log_printf(LOG_NOTICE, "Invalid request line. Ignored.");
		send_error(*sock, ERR_CODE_REQ_BAD);
		goto close_conn;
	}

#ifdef _DEBUG
	log_printf(LOG_INFO, "Parsed request line:");
	reqline_dump(reqline);
#endif /* _DEBUG */

	/* Reply to the client and accept the contents if the type requires. */
	switch (reqline->type) {
		case REQ_TYPE_FILE:
			process_file_req(sock, reqline);
			break;
		case REQ_TYPE_URL:
			process_url_req(sock, reqline);
			break;
		case REQ_TYPE_TEXT:
			process_text_req(sock, reqline);
			break;
		default:
			log_printf(LOG_ERROR, "Unknown transfer type '%c' %s",
				reqline->type, reqline->stype);
			send_error(*sock, ERR_CODE_UNKNOWN);
			break;
	}

close_conn:
	/* Free our request line object. */
	reqline_free(reqline);

	/* Close the client connection and signal that we are finished here. */
	if (*sock != SOCKERR) {
		socket_close(*sock, false);
		log_printf(LOG_INFO, "Closed client connection");
	}
	*sock = SOCKERR;
	server_status &= ~CLIENT_CONNECTED;
}

/**
 * Processes and replies to the client that sent a file transfer request.
 *
 * @param sockfd  Client's socket handle used to reply.
 * @param reqline Request line object.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
bool process_file_req(const sockfd_t *sockfd, const reqline_t *reqline) {
	uint8_t buf[RECV_BUF_LEN];
	char *fname;
	size_t acclen;
	ssize_t len;
	FILE *fh;
	bool ret;

	/* Initialize some variables. */
	fh = NULL;
	ret = true;

	/* Sanitize filename. */
	fname = strdup(reqline->name);
	if (fname_sanitize(fname)) {
		log_printf(LOG_INFO, "Filename \"%s\" contained malicious characters "
			"and was sanitized to \"%s\"", reqline->name, fname);
	}

	/* Ensure we are not overwriting any existing files. */
	while (file_exists(fname)) {
		char *nf;
		size_t slen;

		/* Check if we are just incrementing an existing prefix. */
		nf = fname;
		slen = strlen(fname);
		if ((slen > 1) && (*nf >= '0') && (*nf < '9') && (nf[1] == '_')) {
			*nf = (char)(*nf + 1);
			continue;
		}

		/* Allocate the new filename string. */
		nf = (char *)malloc((slen + 4) * sizeof(char));
		if (nf == NULL) {
			log_syserr(LOG_CRIT, "Failed to allocate new string for filename");
			send_error(*sockfd, ERR_CODE_INTERNAL);
			return false;
		}

		/* Build up the new filename and switch them. */
		sprintf(nf, "1_%s", fname);
		free(fname);
		fname = nf;
	}

	/* Ask the user if they want to accept the transfer. */
	if (!opts.accept_all &&
	    !ask_yn("Do you want to receive the file \"%s\"?", fname)) {
		goto refuse;
	}

	/* Open the file for writing. */
	fh = fopen(fname, "wb");
	if (fh == NULL) {
		log_printf(LOG_ERROR, "Failed to open file \"%s\" for writing", fname);
		goto refuse;
	}
	send_continue(*sockfd);

	/* Pipe the contents of the file from the network. */
	acclen = 0;
	while ((len = recv(*sockfd, buf, RECV_BUF_LEN, 0)) > 0) {
		/* Deal with the transfer size. */
		acclen += len;
		if (acclen > reqline->size) {
			fprintf(stderr, "\n");
			log_printf(LOG_ERROR, "Received file is bigger than expected");
			goto refuse;
		}

		/* Show transfer progress and write to the file. */
		buffered_progress(fname, acclen, reqline->size);
		fwrite(buf, sizeof(uint8_t), len, fh);

		/* Detect if we have finished transferring the file. */
		if (acclen == reqline->size) {
			send_ok(*sockfd);
			break;
		}
	}

	/* Check if the connection ended before the file finished transferring. */
	if (len <= 0) {
		fprintf(stderr, "\n");
		log_sockerr(LOG_ERROR, "The client has closed the connection before "
			"the file \"%s\" finished transferring", fname);
		ret = false;
	} else {
		fprintf(stderr, "\n");
	}

	/* Free up resources. */
	free(fname);
	fname = NULL;
	fclose(fh);
	fh = NULL;

	return ret;

refuse:
	if (fname)
		free(fname);
	if (fh)
		fclose(fh);
	send_refused(*sockfd);
	return false;
}

/**
 * Processes and replies to the client that sent an URL request.
 *
 * @param sockfd  Client's socket handle used to reply.
 * @param reqline Request line object.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
bool process_url_req(const sockfd_t *sockfd, const reqline_t *reqline) {
	const char *url;
	char *cmd;

	/* Check if the URL may be malicious and refuse instantly. */
	url = reqline->name;
	if (strncmp(url, "file://", 7) == 0) {
		send_refused(*sockfd);
		log_printf(LOG_NOTICE, "Blocked malicious URL request for \"%s\"", url);
		return false;
	}

	/* Print out the URL for piping. */
	printf("%s\n", url);

	/* Open in the OS's default browser. */
	if (ask_yn("Do you want to open the above URL?")) {
#ifdef _WIN32
		/* Windows */
		ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#else
		/* UNIX */
		size_t len;
#if defined(__APPLE__) || defined(__MACH__)
		/* Mac OS X */
		len = 8 + strlen(url);
		cmd = (char *)malloc(len * sizeof(char));
		snprintf(cmd, len, "open '%s'", url);
		cmd[len - 1] = '\0';
#else
		/* UNIX / XDG */
		len = 12 + strlen(url);
		cmd = (char *)malloc(len * sizeof(char));
		snprintf(cmd, len, "xdg-open '%s'", url);
		cmd[len - 1] = '\0';
#endif /* __APPLE__ || __MACH__ */

		/* Execute open command and free the command line string. */
		system(cmd);
		free(cmd);
#endif /* _WIN32 */
	} else {
		/* Reply with refused. */
		send_refused(*sockfd);
		return false;
	}

	/* Send OK and stop processing the request. */
	send_ok(*sockfd);
	return true;
}

/**
 * Processes and replies to the client that sent a text transfer request.
 *
 * @param sockfd  Client's socket handle used to reply.
 * @param reqline Request line object.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
bool process_text_req(const sockfd_t *sockfd, const reqline_t *reqline) {
	uint8_t buf[RECV_BUF_LEN];
	size_t acclen;
	ssize_t len;
	bool ret;

	/* Ask the user if they want to accept the transfer. */
	if ((reqline->size > RECV_TEXT_THRESHOLD) && !opts.accept_all &&
	    !ask_yn("Do you want to receive %u bytes of text?", reqline->size)) {
		send_refused(*sockfd);
		return false;
	}

	/* Begin the transfer. */
	fputs("----------BEGIN TEXT BLOCK----------\n", stderr);
	fflush(stderr);
	send_continue(*sockfd);
	ret = true;

	/* Pipe the text content to stdout. */
	acclen = 0;
	while ((len = recv(*sockfd, buf, RECV_BUF_LEN, 0)) > 0) {
		/* Deal with the transfer size. */
		acclen += len;
		if (acclen > reqline->size) {
			fprintf(stderr, "\n");
			log_printf(LOG_ERROR, "Received text is bigger than expected");
			send_refused(*sockfd);
			return false;
		}

		/* Show transfer progress and write to the file. */
		fwrite(buf, sizeof(uint8_t), len, stdout);
		fflush(stdout);

		/* Detect if we have finished transferring the file. */
		if (acclen == reqline->size) {
			send_ok(*sockfd);
			break;
		}
	}

	/* End the text block. */
	if ((acclen > 0) && (buf[acclen - 1] != '\n'))
		fputc('\n', stderr);
	fflush(stderr);
	fputs("-----------END TEXT BLOCK-----------\n", stderr);

	/* Check if the connection ended before the file finished transferring. */
	if (len <= 0) {
		log_sockerr(LOG_ERROR, "The client has closed the connection before "
			"the text contents finished transferring");
		ret = false;
	}

	return ret;
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

	/* Stop our server. */
	server_stop();

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
			server_stop();
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
	printf("usage: %s [-l addr] [-p port] [-y]\n\n", prog);
	puts("options:");
	puts("    -h         Displays this message");
	puts("    -l addr    Server should listen on the specified address");
	puts("    -p port    Port the server should listen on");
	puts("    -y         Automatically accept all requests without asking");
	puts("");
	puts(GL_COPYRIGHT);
}
