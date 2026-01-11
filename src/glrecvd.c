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

#include "logging.h"
#include "sockets.h"

/* Server's default port. */
#define GL_SERVER_PORT 1650

/* Server status flags */
#define SERVER_RUNNING   0x01
#define CLIENT_CONNECTED 0x02

/* Private methods. */
bool server_start(const char *addr, uint16_t port);
void server_stop(void);
void server_loop(int af, sockfd_t sockfd);
void server_process_request(sockfd_t *sock);
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
	if ((sockfd_server != SOCKERR) && (sockclose(sockfd_server) == SOCKERR))
		log_sockerr(LOG_ERROR, "Failed to close server socket");
	sockfd_server = SOCKERR;

	/* Close client connection. */
	if (server_status & CLIENT_CONNECTED) {
		server_status &= ~CLIENT_CONNECTED;
		if (sockfd_client != SOCKERR)
			sockclose(sockfd_client);
		sockfd_client = SOCKERR;
	}
}

/**
 * Server listening loop.
 *
 * @param af     Socket address family.
 * @param sockfd Socket in which the server is listening on.
 */
void server_loop(int af, sockfd_t sockfd) {
	while (server_status & SERVER_RUNNING) {
		struct sockaddr_storage csa;
		sockfd_t *sock;
		socklen_t socklen;
		char addrstr[IPADDR_STRLEN];

		/* Accept the client connection. */
		sock = &sockfd_client;
		socklen = sizeof(csa);
		*sock = accept(sockfd_server, (struct sockaddr*)&csa, &socklen);
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
	char reqline[GL_REQLINE_MAX + 1];
	ssize_t len;
	int i;

	/* Read the selector from client's request. */
	if ((len = recv(*sock, reqline, GL_REQLINE_MAX, 0)) < 0) {
		if (server_status & SERVER_RUNNING)
			log_sockerr(LOG_ERROR, "Server failed to receive request line");
		goto close_conn;
	}
	reqline[len] = '\0';

	/* Ensure the request wasn't too long. */
	if (len >= GL_REQLINE_MAX) {
		log_printf(LOG_WARNING, "Request line unusually long, closing "
			"connection.");
		// TODO: client_send_error(conn, "Request line longer than 255 characters");
		goto close_conn;
	}

	/* Terminate the request line before CRLF. */
	for (i = 0; i < len; i++) {
		if ((reqline[i] == '\r') || (reqline[i] == '\n')) {
			reqline[i] = '\0';
			break;
		}
	}

	/* Print the request line and reply with OK. */
	log_printf(LOG_INFO, "Client requested '%s'", reqline);
	send(*sock, "OK\r\n", 4, 0);

close_conn:
	/* Close the client connection and signal that we are finished here. */
	if (*sock != SOCKERR)
		sockclose(*sock);
	*sock = SOCKERR;
	server_status &= ~CLIENT_CONNECTED;
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
