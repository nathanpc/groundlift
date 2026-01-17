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

#include "defaults.h"
#include "logging.h"
#include "sockets.h"
#include "request.h"
#include "utils.h"

/* Server status flags */
#define SERVER_RUNNING   0x01
#define CLIENT_CONNECTED 0x02

/* Private functions. */
bool server_start(const char *addr, uint16_t port);
void server_stop(void);
void server_loop(int af, sockfd_t server);
void server_process_request(sockfd_t *sock);
bool process_file_req(const sockfd_t *sockfd, const reqline_t *reqline);
bool process_url_req(const sockfd_t *sockfd, const reqline_t *reqline);
void sigint_handler(int sig);

/* State variables. */
static uint8_t server_status;
static sockfd_t sockfd_server;
static sockfd_t sockfd_client;

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
	const char *addr;
	uint16_t port;

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
	socket_init();

	/* Catch the interrupt signal from the console. */
	signal(SIGINT, sigint_handler);

	/* Check if we have the right number of command-line arguments. */
	if (argc > 3) {
		fprintf(stderr, "usage: %s listen_ip port\n", argv[0]);
		ret = 1;
		goto cleanup;
	}

	/* Populate our options. */
	addr = (argc > 1) ? argv[1] : NULL;
	port = (argc > 2) ? atoi(argv[2]) : GL_SERVER_PORT;

	/* Run the server. */
	if (!server_start((addr == NULL) ? "0.0.0.0" : addr, port))
		goto cleanup;
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
bool server_start(const char *addr, uint16_t port) {
	/* Get the listening socket for our server. */
	sockfd_server = socket_new_server(AF_INET, addr, port);
	if (sockfd_server == SOCKERR)
		return false;

	/* Indicate that the server has started. */
	log_printf(LOG_NOTICE, "Server started on %s:%u", addr, port);
	server_status |= SERVER_RUNNING;

	return true;
}

/**
 * Stops the server immediately.
 */
void server_stop(void) {
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
		default:
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
		uint8_t i;

		/* Allocate the new filename string. */
		i = 1;
		nf = (char *)malloc((strlen(fname) + 4) * sizeof(char));
		if (nf == NULL) {
			log_syserr(LOG_CRIT, "Failed to allocate new string for filename");
			send_error(*sockfd, ERR_CODE_INTERNAL);
			return false;
		}

		/* Build up the new filename and switch them. */
		sprintf(nf, "%u_%s", i, fname);
		free(fname);
		fname = nf;
	}

	/* Ask the user if they want to accept the transfer. */
	if (!ask_yn("Do you want to receive the file \"%s\"?", fname))
		goto refuse;

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
		fprintf(stderr, "\r%s (%lu/%lu)", fname, acclen, reqline->size);
		fwrite(buf, sizeof(uint8_t), len, fh);

		/* Detect if we have finished transferring the file. */
		if (acclen == reqline->size) {
			send_ok(*sockfd);
			break;
		}
	}

	/* Check if the connection ended before the file finished transferring. */
	if (len <= 0) {
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
