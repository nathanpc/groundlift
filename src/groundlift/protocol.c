/**
 * protocol.c
 * GroundLift's glproto implementation and helper functions.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <utils/logging.h>

#include "conf.h"

/* Public variables. */
glproto_msg_t *glproto_invalid_msg;

/* Private variables. */
static glproto_msg_t void_msg;

/* Private methods. */
size_t glproto_msg_length(glproto_msg_t *msg);
uint8_t *glproto_msg_buf(glproto_msg_t *msg);

/**
 * Initializes the GroundLift protocol helper subsystem.
 */
void glproto_init(void) {
	glproto_invalid_msg = &void_msg;
	void_msg.type = GLPROTO_TYPE_INVALID;
	void_msg.length = 0;
	memset(void_msg.glupi, 0, 8);
	void_msg.device[0] = '\0';
	void_msg.hostname = NULL;
}

/**
 * Allocates a brand new message object to contain a message received from a
 * peer.
 *
 * @warning This function allocates memory that must be freed by you.
 *
 * @param type Message type.
 *
 * @return Newly allocated message object.
 *
 * @see glproto_msg_free
 */
glproto_msg_t *glproto_msg_new(glproto_type_t type) {
	/* Allocate the memory for the object. */
	glproto_msg_t *msg = (glproto_msg_t *)malloc(glproto_msg_sizeof(type));

	/* Ensure a sane invalid default for the common bits. */
	msg->type = type;
	msg->length = 0;
	memset(void_msg.glupi, 0, 8);
	msg->device[0] = '\0';
	msg->hostname = NULL;
	msg->sock = NULL;

	/* Ensure the same sane defaults specific to some message types. */
	switch (msg->type) {
		case GLPROTO_TYPE_DISCOVERY:
			/* No need to do anything else at this point. */
			break;
		case GLPROTO_TYPE_FILE:
			((glproto_file_req_msg_t *)msg)->fb = NULL;
			break;
		default:
			gl_error_push(ERR_TYPE_GL, GL_ERR_NOT_IMPLEMENTED,
				EMSG("Message object doesn't have new implemented"));
			break;
	}

	return msg;
}

/**
 * Allocates a message object for ourselves.
 *
 * @warning This function allocates memory that must be freed by you.
 *
 * @param type Message type.
 *
 * @return Allocated message object.
 *
 * @see glproto_msg_free
 */
glproto_msg_t *glproto_msg_new_our(glproto_type_t type) {
	glproto_msg_t *msg;

	/* Allocate the memory for the object. */
	msg = glproto_msg_new(type);

	/* Ensure a sane invalid default for the common bits. */
	msg->length = 0;
	memcpy(msg->glupi, conf_get_glupi(), 8);
	conf_get_devtype(msg->device);
	msg->hostname = strdup(conf_get_hostname());
	msg->sock = NULL;

	/* Ensure the same sane defaults specific to some message types. */
	switch (msg->type) {
		case GLPROTO_TYPE_DISCOVERY:
			/* No need to do anything else at this point. */
			break;
		case GLPROTO_TYPE_FILE:
			((glproto_file_req_msg_t *)msg)->fb = NULL;
			break;
		default:
			gl_error_push(ERR_TYPE_GL, GL_ERR_NOT_IMPLEMENTED,
				EMSG("Message object doesn't have new implemented"));
			break;
	}

	return msg;
}

/**
 * Frees up the memory allocated by a message object.
 *
 * @warning Remember to set the object to NULL afterwards to avoid issues with
 *          realloc().
 *
 * @param msg Message object to be freed.
 */
void glproto_msg_free(glproto_msg_t *msg) {
	/* Do we even need to free anything? */
	if (msg == NULL)
		return;

	/* Free the hostname. */
	if (msg->hostname) {
		free(msg->hostname);
		msg->hostname = NULL;
	}

	/* Free the socket information. */
	if (msg->sock) {
		socket_free(msg->sock);
		msg->sock = NULL;
	}

	/* Free things specific to some message types. */
	switch (msg->type) {
		case GLPROTO_TYPE_DISCOVERY:
			/* No need to do anything else at this point. */
			break;
		case GLPROTO_TYPE_FILE: {
			glproto_file_req_msg_t *req_msg = ((glproto_file_req_msg_t *)msg);
			file_bundle_free(req_msg->fb);
			req_msg->fb = NULL;
			break;
		}
		default:
			gl_error_push(ERR_TYPE_GL, GL_ERR_NOT_IMPLEMENTED,
				EMSG("Message object doesn't have free implemented"));
			break;
	}

	/* Free the whole rest of the thing. */
	free(msg);
}

/**
 * Checks if a message head is actually valid.
 *
 * @warning This function won't perform any sanity checks for size or NULL, so
 *          ensure that you've sanitized the buffer before passing it.
 *
 * @param head Array of 6 bytes containing the message head.
 *
 * @return TRUE if the head is valid or FALSE if it isn't.
 */
bool glproto_msg_head_isvalid(const uint8_t *head) {
	return (head[0] == 'G') && (head[1] == 'L') && (head[2] != '\0') &&
		   (head[3] == '\0');
}

/**
 * Parses a message from a network buffer.
 *
 * @param msg  Returns a parsed message object.
 * @param rbuf Buffer with data received from the network.
 * @param len  Length of the entire message in bytes.
 *
 * @return Error report or NULL if the operation was successful.
 */
gl_err_t *glproto_msg_parse(glproto_msg_t **msg, const void *rbuf, size_t len) {
	uint8_t const *buf;
	glproto_type_t type;
	glproto_msg_t *nmsg;
	uint8_t i;

	/* Initialize some defaults. */
	buf = (uint8_t const *)rbuf;
	type = GLPROTO_MSG_TYPE(buf);

	/* Allocate the memory needed to hold our parsed message. */
	*msg = glproto_msg_new(type);
	nmsg = *msg;
	nmsg->length = (uint16_t)len;

	/* Parse the GLUPI. */
	buf += GLPROTO_MSG_HEADER_VAL_OFFSET;
	i = 0;
	do {
		nmsg->glupi[i++] = *buf++;
	} while (i < 8);

	/* Parse the device identifier. */
	buf++;
	i = 0;
	do {
		nmsg->device[i++] = (char)*buf++;
	} while (i < 3);
	nmsg->device[3] = '\0';
	buf++;

	/* Parse hostname. */
	buf++;
	i = *buf++;
	nmsg->hostname = realloc(nmsg->hostname, i * sizeof(char));
	strncpy(nmsg->hostname, (const char *)buf, i);
	nmsg->hostname[i - 1] = '\0';
	buf += i;

	/* Parse any message type-specific headers. */
	switch (type) {
		case GLPROTO_TYPE_DISCOVERY:
			/* No need to do anything else at this point. */
			break;
		case GLPROTO_TYPE_FILE: {
			glproto_file_req_msg_t *frm = (glproto_file_req_msg_t *)nmsg;

			/* Parse TCP transfer port. */
			buf++;
			frm->port = 0;
			frm->port |= (uint16_t)((*buf++) << 8);
			frm->port |= (uint16_t)((*buf++) & 0xFF);

			/* Parse file length. */
			buf++;
			frm->fb = file_bundle_new_empty();
			frm->fb->size = 0;
			frm->fb->size |= (fsize_t)(*buf++) << 56;
			frm->fb->size |= (fsize_t)(*buf++) << 48;
			frm->fb->size |= (fsize_t)(*buf++) << 40;
			frm->fb->size |= (fsize_t)(*buf++) << 32;
			frm->fb->size |= (fsize_t)(*buf++) << 24;
			frm->fb->size |= (fsize_t)(*buf++) << 16;
			frm->fb->size |= (fsize_t)(*buf++) << 8;
			frm->fb->size |= (fsize_t)((*buf++) & 0xFF);

			/* Parse file name. */
			buf++;
			i = *buf++;
			frm->fb->base = (char *)malloc(i * sizeof(char));
			strncpy(frm->fb->base, (const char *)buf, i);
			frm->fb->base[i - 1] = '\0';
			buf += i;

			break;
		}
		default:
			return gl_error_push(ERR_TYPE_GL, GL_ERR_NOT_IMPLEMENTED,
				EMSG("Message received doesn't have a parser implemented"));
	}

	/* Ignore any value set but not read warnings. */
	(void)buf;

	return NULL;
}

/**
 * Handles the incoming data and automatically filters out invalid packets.
 *
 * @warning This function allocates msg and client which must be freed by you.
 *
 * @param sock Server handle object.
 * @param type Returns the received message type.
 * @param msg  Returns a parsed message object.
 * @param serr Pointer to store the socket error/status or NULL to ignore.
 *
 * @return Error report or NULL if an unrecoverable error occurred.
 */
gl_err_t *glproto_recvfrom(const sock_handle_t *sock, glproto_type_t *type,
                           glproto_msg_t **msg, sock_err_t *serr) {
	size_t len;
	size_t rlen;
	uint8_t peek[6];
	void *buf;
	gl_err_t *err;
	sock_handle_t *client;

	/* Allocate the client socket handle object. */
	client = socket_new();
	client->addr_len = sizeof(struct sockaddr_storage);
	client->addr = malloc(client->addr_len);

	/* Get the message head. */
	len = 0;
	err = socket_recvfrom(sock, peek, 6, client->addr, &client->addr_len, &len,
	                      true, serr);
	if (err || (len == 0)) {
		socket_free(client);
		return err;
	}

	/* Check if the message head is valid. */
	if (len != 6) {
		log_printf(LOG_DEBUG, "Invalid message length received %ld expected 6",
		           len);
		*type = GLPROTO_TYPE_INVALID;
		*msg = glproto_invalid_msg;

		/* Skip the peeked bytes. */
		socket_recvfrom(sock, peek, 6, NULL, NULL, &len, false, serr);
		socket_free(client);

		return NULL;
	} else if (!glproto_msg_head_isvalid(peek)) {
		log_printf(LOG_DEBUG, "Invalid message head received");
		*type = GLPROTO_TYPE_INVALID;
		*msg = glproto_invalid_msg;

		/* Skip the peeked bytes. */
		socket_recvfrom(sock, peek, 6, NULL, NULL, &len, false, serr);
		socket_free(client);

		return NULL;
	}

	/* Allocate a buffer and get the full message. */
	len = ntohs((uint16_t)((peek[4] << 8) | peek[5]));
	rlen = 0;
	buf = malloc(len);
	err = socket_recvfrom(sock, buf, len, NULL, NULL, &rlen, false, serr);
	if (err)
		goto cleanup;
	if (rlen != len) {
		err = gl_error_push(ERR_TYPE_SOCKET, SOCK_ERR_ERECV,
			EMSG("Number of bytes for message expected differ from read"));
		socket_free(client);

		goto cleanup;
	}

	/* Parse the message. */
	*type = peek[2];
	err = glproto_msg_parse(msg, buf, len);
	(*msg)->sock = client;

#ifdef DEBUG
	if (err == NULL) {
		/* Print out the message for debugging. */
		glproto_msg_print(*msg, "> ");
	}
#endif /* DEBUG */

cleanup:
	/* Free up resources. */
	free(buf);
	buf = NULL;

	return err;
}

/**
 * Sends a message using UDP via a specified socket connection.
 *
 * @param sock Socket handle to send the message to.
 * @param msg  Message to be sent.
 *
 * @return Error report or NULL if the operation was successful.
 */
gl_err_t *glproto_msg_sendto(const sock_handle_t *sock, glproto_msg_t *msg) {
	uint8_t *buf;
	gl_err_t *err;

	/* Allocate and populate the buffer for sending. */
	buf = glproto_msg_buf(msg);
	if (buf == NULL)
		return gl_error_last();

#ifdef DEBUG
	/* Print out the message for debugging. */
	glproto_msg_print(msg, "< ");
#endif /* DEBUG */

	/* Send the message over the network. */
	err = socket_sendto(sock, buf, msg->length, sock->addr, sock->addr_len,
	                    NULL);

	/* Free up any allocated resources. */
	free(buf);

	return err;
}

/**
 * Populates a buffer with all of the necessary data to send a message over the
 * network.
 *
 * @param msg Message to be sent.
 * @param buf Buffer to hold network message. This must already be allocated.
 *
 * @return Error report or NULL if the operation was successful.
 */
uint8_t *glproto_msg_buf(glproto_msg_t *msg) {
	const char *str;
	uint8_t i;
	uint16_t len;
	uint8_t *buf;
	uint8_t *tmp;

	/* Calculate the message length and allocate a buffer to send it. */
	len = (uint16_t)glproto_msg_length(msg);
	buf = malloc(len);
	tmp = buf;

	/* Identifier bits. */
	*tmp++ = 'G';
	*tmp++ = 'L';
	*tmp++ = (uint8_t)msg->type;
	*tmp++ = '\0';

	/* Message length. */
	len = htons(msg->length);
	*tmp++ = (uint8_t)(len >> 8);
	*tmp++ = (uint8_t)(len & 0xFF);

	/* GLUPI */
	*tmp++ = '|';
	for (i = 0; i < (uint8_t)sizeof(msg->glupi); i++) {
		*tmp++ = msg->glupi[i];
	}

	/* Device identifier. */
	*tmp++ = '|';
	for (i = 0; i < (uint8_t)sizeof(msg->device); i++) {
		*tmp++ = msg->device[i];
	}

	/* Hostname */
	*tmp++ = '|';
	*tmp++ = (uint8_t)strlen(msg->hostname) + 1;
	str = msg->hostname;
	while (*str != '\0') {
		*tmp++ = *str++;
	}
	*tmp++ = '\0';

	/* Encode any message type-specific headers. */
	switch (msg->type) {
		case GLPROTO_TYPE_DISCOVERY:
			/* No need to do anything else at this point. */
			break;
		case GLPROTO_TYPE_FILE: {
			size_t slen;
			const char *sbuf;
			const glproto_file_req_msg_t *frm =
			        ((const glproto_file_req_msg_t *)msg);

			/* TCP port for file transfer. */
			*tmp++ = '|';
			*tmp++ = (uint8_t)((frm->port >> 8) & 0xFF);
			*tmp++ = (uint8_t)(frm->port & 0xFF);

			/* File length. */
			*tmp++ = '|';
			*tmp++ = (uint8_t)((frm->fb->size >> 56) & 0xFF);
			*tmp++ = (uint8_t)((frm->fb->size >> 48) & 0xFF);
			*tmp++ = (uint8_t)((frm->fb->size >> 40) & 0xFF);
			*tmp++ = (uint8_t)((frm->fb->size >> 32) & 0xFF);
			*tmp++ = (uint8_t)((frm->fb->size >> 24) & 0xFF);
			*tmp++ = (uint8_t)((frm->fb->size >> 16) & 0xFF);
			*tmp++ = (uint8_t)((frm->fb->size >> 8) & 0xFF);
			*tmp++ = (uint8_t)(frm->fb->size & 0xFF);

			/* File name. */
			*tmp++ = '|';
			slen = strlen(frm->fb->base) + 1;
			if (slen > 254) {
				gl_error_push(ERR_TYPE_GL, GL_ERR_WARNING,
					EMSG("File name is longer than 255 chars, will truncate"));
			}
			*tmp++ = (uint8_t)(slen & 0xFF);
			sbuf = frm->fb->base;
			for (i = 0; i < 0xFF; i++) {
				if (*sbuf == '\0')
					break;

				*tmp++ = *sbuf++;
			}
			*tmp++ = '\0';

			break;
		}
		default:
			gl_error_push(ERR_TYPE_GL, GL_ERR_NOT_IMPLEMENTED,
				EMSG("Message to send doesn't have an encoder implemented"));
			break;
	}

	return buf;
}

/**
 * Gets the amount of memory that should be allocated to hold the message object
 * of the desired type.
 *
 * @param type Type of message.
 *
 * @return Number of bytes necessary to hold the message object.
 */
size_t glproto_msg_sizeof(glproto_type_t type) {
	switch (type) {
		case GLPROTO_TYPE_DISCOVERY:
			return sizeof(glproto_discovery_msg_t);
		case GLPROTO_TYPE_FILE:
			return sizeof(glproto_file_req_msg_t);
		default:
			gl_error_push(ERR_TYPE_GL, GL_ERR_PROTOCOL,
						  EMSG("Unknown message type to get sizeof"));
			exit(GL_ERR_PROTOCOL);
	}
}

/**
 * Prints out the contents of a message in a human-readable way. Extremely
 * useful for debugging.
 *
 * @param msg    Message to be printed out.
 * @param prefix String that will prefix each line printed out. Use NULL if no
 *               prefix is required.
 */
void glproto_msg_print(const glproto_msg_t *msg, const char *prefix) {
	/* Print the prefix? */
	if (prefix)
		printf("%s", prefix);

	/* Message type. */
	switch (msg->type) {
		case GLPROTO_TYPE_DISCOVERY:
			printf("Discovery");
			break;
		case GLPROTO_TYPE_URL:
			printf("URL");
			break;
		case GLPROTO_TYPE_FILE:
			printf("File");
			break;
		default:
			printf("Unknown");
			break;
	}

	/* Print out the rest of the common part of the message. */
	printf(" (%u bytes) from %s [%s]\n", msg->length, msg->hostname,
		   msg->device);
	printf("%sGLUPI: %x/%x/%x/%x/%x/%x/%x/%x\n", (prefix) ? prefix : "",
	       msg->glupi[0], msg->glupi[1], msg->glupi[2], msg->glupi[3],
	       msg->glupi[4], msg->glupi[5], msg->glupi[6], msg->glupi[7]);

	/* Print specific stuff to a message type. */
	switch (msg->type) {
		case GLPROTO_TYPE_DISCOVERY:
			/* No need to do anything else at this point. */
			break;
		case GLPROTO_TYPE_FILE: {
			const glproto_file_req_msg_t *fr =
			        ((const glproto_file_req_msg_t *)msg);
			char mag;
			float hsize;

			/* Convert file size to human-readable format. */
			hsize = file_size_readable(fr->fb->size, &mag);

			printf("%sTransfer Port: %u\n", (prefix) ? prefix : "", fr->port);
			printf("%sFile: %s ", (prefix) ? prefix : "", fr->fb->base);
			if (mag == 'B') {
				/* Very small files. */
				printf("(%llu bytes)\n", fr->fb->size);
			} else {
				/* Bigger files. */
				char magstr[6];

				magstr[0] = mag;
				magstr[1] = 'B';
				magstr[2] = '\0';

				printf("(%.3f %s / %llu bytes)\n", hsize, magstr,
				       fr->fb->size);
			}

			break;
		}
		default:
			gl_error_push(ERR_TYPE_GL, GL_ERR_NOT_IMPLEMENTED,
				EMSG("Message type doesn't have its print implemented"));
			break;
	}

	printf("\n");
}

/**
 * Calculates the length in bytes needed to send a message.
 *
 * @warning This function will set the length parameter of the message.
 *
 * @param msg Message to have its total length calculated and field updated.
 *
 * @return Length in bytes needed to transfer the entirety of the message.
 */
size_t glproto_msg_length(glproto_msg_t *msg) {
	size_t len = 0;

	/* Calculate length of the common part. */
	len += 22 + strlen(msg->hostname) + 1;

	/* Calculate the length of specific parts. */
	switch (msg->type) {
		case GLPROTO_TYPE_DISCOVERY:
			/* No need to do anything else at this point. */
			break;
		case GLPROTO_TYPE_FILE: {
			const glproto_file_req_msg_t *fr = ((glproto_file_req_msg_t *)msg);
			len += 14 + strlen(fr->fb->base) + 1;
			break;
		}
		default:
			gl_error_push(ERR_TYPE_GL, GL_ERR_NOT_IMPLEMENTED,
				EMSG("Message type doesn't have its print implemented"));
			break;
	}

	/* Update the length field in the message. */
	msg->length = (uint16_t)len;

	return len;
}
