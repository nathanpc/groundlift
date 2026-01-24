/**
 * request.c
 * Helps to encode and decode information for GroundLift's requests.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "request.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "defaults.h"
#include "logging.h"
#include "utils.h"

/**
 * Sends a OK reply to a client, terminating the exchange.
 *
 * @param sockfd Socket handle used to reply.
 */
void send_ok(sockfd_t sockfd) {
	send(sockfd, "200\tOK\r\n", 8, 0);
}

/**
 * Sends a REFUSED reply to a client.
 *
 * @param sockfd Socket handle used to reply.
 */
void send_refused(sockfd_t sockfd) {
	send(sockfd, "403\tREFUSED\tUser refused the transfer\r\n", 39, 0);
}

/**
 * Sends a CONTINUE reply to a client, requesting it to send the contents of the
 * transaction it requested in the first place.
 *
 * @param sockfd Socket handle used to reply.
 */
void send_continue(sockfd_t sockfd) {
	send(sockfd, "100\tCONTINUE\tReady to accept content\r\n", 38, 0);
}

/**
 * Replies to a client with an error message.
 *
 * @param sockfd Socket handle used to reply.
 * @param code   Error code to notify.
 */
void send_error(sockfd_t sockfd, error_code_t code) {
	char strcode[4];

	/* Convert error code to string. */
	snprintf(strcode, 4, "%u", code);
	strcode[3] = '\0';

	/* Send the error header and code. */
	send(sockfd, strcode, 3, 0);
	send(sockfd, "\tERROR\t", 7, 0);

	/* Send the error message. */
	switch (code) {
		case ERR_CODE_REQ_BAD:
			send(sockfd, "Failed to parse request line", 28, 0);
			break;
		case ERR_CODE_REQ_LONG:
			send(sockfd, "Request line too long", 21, 0);
			break;
		case ERR_CODE_INTERNAL:
			send(sockfd, "Internal server error", 21, 0);
			break;
		default:
			send(sockfd, "Unknown error", 13, 0);
			break;
	}

	/* End with a CRLF. */
	send(sockfd, "\r\n", 2, 0);
}

/**
 * Allocates a new request line object.
 *
 * @warning This function allocates memory that must later be freed.
 *
 * @return A brand new request line object, or NULL if an error occurred.
 *
 * @see reqline_free
 */
reqline_t *reqline_new(void) {
	reqline_t *reqline;

	/* Allocate the object. */
	reqline = (reqline_t *)malloc(sizeof(reqline_t));
	if (reqline == NULL) {
		log_syserr(LOG_CRIT, "Failed to allocate memory for request line "
			"object");
		return NULL;
	}

	/* Initialize the object with some sane defaults. */
	reqline->type = REQ_TYPE_UNKNOWN;
	reqline->stype = NULL;
	reqline->name = NULL;
	reqline->size = 0;

	return reqline;
}

/**
 * Frees up memory and invalidates a request line object.
 *
 * @param reqline Request line object to be freed.
 */
void reqline_free(reqline_t *reqline) {
	if (reqline == NULL)
		return;

	reqline->type = REQ_TYPE_UNKNOWN;
	reqline->size = 0;

	if (reqline->stype) {
		free(reqline->stype);
		reqline->stype = NULL;
	}

	if (reqline->name) {
		free(reqline->name);
		reqline->name = NULL;
	}

	free(reqline);
}

/**
 * Parses a request line and populates new request line object.
 *
 * @warning This function allocates memory that must later be freed.
 *
 * @return A new, fully populated, request line object, or NULL if an error
 *         occurred.
 *
 * @see reqline_free
 */
reqline_t *reqline_parse(const char *line) {
	reqline_t *reqline;
	const char *cur;
	char *buf;
	uint8_t step;

	/* Allocate a brand new request line object. */
	reqline = reqline_new();
	if (reqline == NULL)
		return NULL;

	/* Parse each field of the request line and assign it to the object. */
	step = 0;
	cur = line;
	while ((buf = struntil(cur, '\t', &cur)) != NULL) {
		switch (step) {
			case 0:
				/* Request type. */
				reqline->stype = buf;
				if (strcmp(buf, "FILE") == 0) {
					reqline->type = REQ_TYPE_FILE;
				} else if (strcmp(buf, "URL") == 0) {
					reqline->type = REQ_TYPE_URL;
				} else if (strcmp(buf, "TEXT") == 0) {
					reqline->type = REQ_TYPE_TEXT;
				} else {
					log_printf(LOG_ERROR, "Unknown request type '%s' from "
						"request line \"%s\"", buf, line);
					goto parse_failed;
				}
				break;
			case 1:
				/* File name or URL. */
				reqline->name = buf;
				break;
			case 2:
				/* Content size. */
				if (!parse_size(buf, &reqline->size)) {
					log_syserr(LOG_NOTICE, "Failed to convert content size "
						"'%s' to number from request line \"%s\"", buf, line);
					free(buf);
					goto skip_parsing;
				}
				break;
			default:
				log_printf(LOG_NOTICE, "Client sent more information than "
					"needed in request line \"%s\"", line);
				free(buf);
				goto skip_parsing;
		}

		step++;
	}

skip_parsing:
	return reqline;

parse_failed:
	/* In case of an error... */
	reqline_free(reqline);
	reqline = NULL;
	return NULL;
}

/**
 * Sends a request line object to a server.
 *
 * @param sockfd  Socket handle already connected to the server.
 * @param reqline Request line object to be sent.
 *
 * @return Number of bytes sent to the server or 0 in case of an error.
 */
size_t reqline_send(sockfd_t sockfd, reqline_t *reqline) {
	char buf[GL_REQLINE_MAX + 1];
	size_t llen;
	ssize_t tlen;

	/* Build up the request line. */
	snprintf(buf, GL_REQLINE_MAX, "%s\t%s\t%lu\r\n", reqline->stype,
		(reqline->name) ? reqline->name : "", reqline->size);
	buf[GL_REQLINE_MAX] = '\0';
	llen = strlen(buf);

	/* Send the request line over. */
	tlen = send(sockfd, buf, llen, 0);
	if (tlen != llen) {
		log_sockerr(LOG_ERROR, "Failed to send the request line to the server");
		return 0;
	}

	return tlen;
}

/**
 * Sets the request type in a request line object.
 *
 * @param reqline Request line object.
 * @param type    Type to be set.
 */
void reqline_type_set(reqline_t *reqline, reqtype_t type) {
	reqline->type = type;
	if (reqline->stype)
		free(reqline->stype);

	switch (type) {
		case REQ_TYPE_FILE:
			reqline->stype = strdup("FILE");
			break;
		case REQ_TYPE_URL:
			reqline->stype = strdup("URL");
			break;
		case REQ_TYPE_TEXT:
			reqline->stype = strdup("TEXT");
			break;
		default:
			reqline->stype = NULL;
			log_printf(LOG_ERROR, "Setting request line type to unknown value");
			break;
	}
}

/**
 * Dumps the content of a request line object to STDOUT for debugging purposes.
 *
 * @param reqline Request line object to be dumped.
 */
void reqline_dump(reqline_t *reqline) {
	/* Do we have anything to dump? */
	if (reqline == NULL) {
		fprintf(stderr, "(Request line object is NULL)\n");
		return;
	}

	/* Dump object contents. */
	fprintf(stderr, "Type: %s ('%c')\n", reqline->stype, reqline->type);
	fprintf(stderr, "Name: \"%s\" (%ld bytes)\n", reqline->name, reqline->size);
}

/**
 * Parses a reply line from the server and allocates a new object.
 *
 * @warning This function allocates memory that must be freed later.
 *
 * @param line Line from the server ending before the CRLF.
 *
 * @return Parsed line into a server reply object or NULL if an error occurred.
 *
 * @see reply_free
 */
reply_t *reply_parse(const char *line) {
	reply_t *reply;
	const char *cur;
	char *buf;
	uint8_t step;

	/* Allocate memory for the reply object. */
	reply = (reply_t *)malloc(sizeof(reply_t));
	if (reply == NULL) {
		log_syserr(LOG_CRIT, "Failed to allocate memory for reply object");
		return NULL;
	}
	reply->type = NULL;
	reply->msg = NULL;

	/* Parse each part of the reply line into the object. */
	step = 0;
	cur = line;
	while ((buf = struntil(cur, '\t', &cur)) != NULL) {
		switch (step) {
			case 0:
				/* Code */
				reply->code = atoi(buf);
				if (reply->code == 0) {
					log_printf(LOG_ERROR, "Failed to parse reply status code");
					free(buf);
					goto parse_failed;
				}
				break;
			case 1:
				/* Type */
				reply->type = buf;
				break;
			case 2:
				/* Message */
				reply->msg = buf;
				break;
			default:
				log_printf(LOG_NOTICE, "Server replied with more information "
					"than expected \"%s\"", line);
				free(buf);
				goto skip_parsing;
		}

		step++;
	}

skip_parsing:
	return reply;

parse_failed:
	/* In case of an error... */
	reply_free(reply);
	reply = NULL;
	return NULL;
}

/**
 * Frees up memory allocated by a server reply object.
 *
 * @param reply Server reply object to be freed.
 */
void reply_free(reply_t *reply) {
	if (reply == NULL)
		return;

	reply->code = 0;
	if (reply->type)
		free(reply->type);
	if (reply->msg)
		free(reply->msg);
	free(reply);
}
