/**
 * request.c
 * Helps to encode and decode information for GroundLift's requests.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "request.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "logging.h"

/* Private functions. */
char *struntil(const char *begin, char token, const char **end);
bool parse_num(const char *str, long *num);

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
	send(sockfd, "\t", 1, 0);
	send(sockfd, "ERROR\t", 7, 0);

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
				} else {
					log_printf(LOG_NOTICE, "Unknown request type '%s' from "
						"request line \"%s\"", buf, line);
					goto parse_failed;
				}
				break;
			case 1:
				/* File name or URL. */
				reqline->name = buf;
				break;
			case 2:
				/* File size. */
				if (reqline->type != REQ_TYPE_FILE) {
					log_printf(LOG_NOTICE, "Third field present in non-FILE "
						"type request from \"%s\"", line);
					free(buf);
					goto skip_parsing;
				}

				/* Try to get a number from the string. */
				if (!parse_num(buf, &reqline->size)) {
					log_syserr(LOG_NOTICE, "Failed to convert file size '%s' to"
						" number from request line \"%s\"", buf, line);
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
 * Dumps the content of a request line object to STDOUT for debugging purposes.
 *
 * @param reqline Request line object to be dumped.
 */
void reqline_dump(reqline_t *reqline) {
	/* Do we have anything to dump? */
	if (reqline == NULL) {
		printf("(Request line object is NULL)\n");
		return;
	}

	/* Dump object contents. */
	printf("Type: %s ('%c')\n", reqline->stype, reqline->type);
	printf("Name: \"%s\" (%ld bytes)\n", reqline->name, reqline->size);
}

/**
 * Gets a string from begin to token without including the token.
 *
 * If the end of the string is reached before the token is found, the entire
 * string is returned as a new allocated string (equivalent to strdup).
 *
 * @warning This function allocates the return string that must later be freed.
 *
 * @param begin Beginning of the string to search for the token.
 * @param token Token to search for in the string.
 * @param end   Optional. Returns the position in begin right after the token
 *              was found (skips the token), unless it's the NUL terminator.
 *
 * @return Newly allocated, NUL terminated, string with the contents from begin
 *         to token. Returns NULL when begin is the end of the string.
 */
char *struntil(const char *begin, char token, const char **end) {
	const char *cur;
	const char *ctmp;
	char *buf;
	char *btmp;

	/* Have we reached the end? */
	if (*begin == '\0')
		return NULL;

	/* Search for the token or the end of the string. */
	cur = begin;
	while ((*cur != token) && (*cur != '\0'))
		cur++;

	/* Return the position where the token was found. */
	if (end != NULL)
		*end = (*cur == '\0') ? cur : cur + 1;

	/* Allocate memory for our new string. */
	buf = (char *)malloc((cur - begin + 1) * sizeof(char));
	if (buf == NULL) {
		log_syserr(LOG_CRIT, "Failed to allocate memory for token string");
		return NULL;
	}

	/* Copy our string over. */
	btmp = buf;
	ctmp = begin;
	while (ctmp != cur) {
		*btmp = *ctmp;
		ctmp++;
		btmp++;
	}
	*btmp = '\0';

	return buf;
}

/**
 * Converts a string to a number and indicates in case of a failure.
 *
 * @remark This function will set errno on failure according to atol or strtol
 *         on your system.
 *
 * @param str String to be converted to a number.
 * @param num Number to return if the conversion is successful.
 *
 * @return TRUE if the conversion was successful, FALSE otherwise.
 */
bool parse_num(const char *str, long *num) {
#ifdef WITHOUT_STRTOL
	*num = atol(str);
#else
	*num = strtol(str, NULL, 10);
#endif /* WITHOUT_STRTOL */

	return (*num != LONG_MIN) && (*num != LONG_MAX) && ((*num != 0) ||
		(strcmp(str, "0") == 0));
}
