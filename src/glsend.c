/**
 * glsend.c
 * GroundLift's command-line sender client.
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

#include "defaults.h"
#include "logging.h"
#include "sockets.h"
#include "request.h"
#include "utils.h"

/* Private functions. */
bool send_file(const char *addr, uint16_t port, const char *fpath);
reply_t *process_server_reply(const sockfd_t *sockfd);
size_t client_file_transfer(const sockfd_t *sockfd, const reqline_t *reqline,
                            const char *fpath);
void sigint_handler(int sig);

/* State variables. */
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
	const char *fpath;
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
	socket_init();

	/* Catch the interrupt signal from the console. */
	signal(SIGINT, sigint_handler);

	/* Check if we have the right number of command-line arguments. */
	if (argc < 4) {
		fprintf(stderr, "usage: %s ip port file\n", argv[0]);
		ret = 1;
		goto cleanup;
	}

	/* Populate our options. */
	addr = argv[1];
	port = atoi(argv[2]);
	fpath = argv[3];

	/* Send the file to the server. */
	send_file(addr, port, fpath);

cleanup:
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
 * Handles the entire process of sending a file to a server.
 *
 * @param addr  Address of the server to send the file to.
 * @param port  Port used to communicate with the server.
 * @param fpath Path of the file to be sent.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
bool send_file(const char *addr, uint16_t port, const char *fpath) {
	reqline_t *reqline;
	reply_t *reply;
	bool ret;

	/* Initialize variables. */
	reply = NULL;
	ret = true;

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
	sockfd_client = socket_new_client(AF_INET, addr, port);
	if (sockfd_client == SOCKERR) {
		ret = false;
		goto cleanup;
	}
	log_printf(LOG_INFO, "Connected to the server on %s:%u", addr, port);

	/* Send request line. */
	if (reqline_send(sockfd_client, reqline) == 0) {
		ret = false;
		goto cleanup;
	}
	log_printf(LOG_INFO, "Sent %s request line", reqline->stype);

	/* Wait and process the server reply. */
	reply = process_server_reply(&sockfd_client);
	if (reply == NULL) {
		ret = false;
		goto cleanup;
	}

#ifdef _DEBUG
	/* Print parsed reply for debugging. */
	log_printf(LOG_INFO, "Parsed server reply: (%u) [%s] \"%s\"", reply->code,
	           reply->type, reply->msg);
#endif /* _DEBUG */

	/* Check if the server replied with an error. */
	if (reply->code >= 300) {
		switch (reply->code) {
			case ERR_CODE_REQ_REFUSED:
				log_printf(LOG_NOTICE, "User refused the request");
				break;
			default:
				log_printf(LOG_ERROR, "Server replied with error: [%u %s] %s",
				           reply->code, reply->type, reply->msg);
				break;
		}

		ret = false;
		goto cleanup;
	} else if (reply->code != 100) {
		/* Server replied with a code that means we shouldn't continue. */
		goto cleanup;
	}

	/* Send the file contents in case of a file request. */
	if (reqline->type == REQ_TYPE_FILE) {
		if (!client_file_transfer(&sockfd_client, reqline, fpath)) {
			log_printf(LOG_NOTICE, "File transfer failed");
			ret = false;
			goto cleanup;
		}
	} else {
		log_printf(LOG_ERROR, "Server replied with CONTINUE, but the %s "
			"request type hasn't yet been implemented", reqline->stype);
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
 * Dumps the contents of a file through a TCP socket connection.
 *
 * @param sockfd  Socket connection to a server that's ready to
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
	fprintf(stderr, "%s (%lu/%lu)", reqline->name, acclen, reqline->size);
	while ((len = fread(buf, sizeof(uint8_t), SEND_BUF_LEN, fh)) > 0) {
		if (send(*sockfd, buf, len, 0) < 0) {
			/* An error occurred during sending. */
			fprintf(stderr, "\n");
#ifdef _WIN32
			log_sockerr(LOG_ERROR, "Failed to pipe contents of file to socket");
#else
			if (sockerrno == EPIPE) {
				log_sockerr(LOG_WARNING, "Server closed connection before file "
					"transfer finished");
			} else {
				log_sockerr(LOG_ERROR, "Failed to pipe contents of file to "
					"socket");
			}
#endif /* _WIN32 */

			acclen = 0;
			break;
		}

		/* Increment the accumulated length and display the progress. */
		acclen += len;
		fprintf(stderr, "\r%s (%lu/%lu)", reqline->name, acclen, reqline->size);
	}

	/* Ensure we go to a new line before continuing to preserve the progress. */
	if (acclen > 0)
		fprintf(stderr, "\n");

	/* Close the file handle and return. */
	fclose(fh);
	return acclen;
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

	/* Don't let the signal propagate. */
	signal(sig, SIG_IGN);
}
