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
static glproto_msg_t *glproto_msg_realloc(glproto_msg_t *msg,
										  glproto_type_t type);

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
	glproto_msg_t *msg = malloc(glproto_msg_sizeof(type));

	/* Ensure a sane invalid default for the common bits. */
	msg->length = 0;
	memset(void_msg.glupi, 0, 8);
	msg->device[0] = '\0';
	msg->hostname = NULL;

	return msg;
}

/**
 * (Re)allocates a message object for ourselves.
 *
 * @warning This function allocates memory that must be freed by you.
 *
 * @param msg  Common message object that's being reused for ourselves.
 * @param type Message type.
 *
 * @return Reallocated message object.
 *
 * @see glproto_msg_free
 */
glproto_msg_t *glproto_msg_new_our(glproto_msg_t *msg, glproto_type_t type) {
	bool new = msg == NULL;

	/* Allocate the memory for the object. */
	msg = glproto_msg_realloc(msg, type);

	/* Ensure a sane invalid default for the common bits. */
	msg->length = 0;
	if (new) {
		memcpy(msg->glupi, conf_get_glupi(), 8);
		conf_get_devtype(msg->device);
		msg->hostname = strdup(conf_get_hostname());
	}

	return msg;
}

/**
 * Reallocates the message object if needed. This function allows us to reuse
 * the same object over and over, reducing the need for constantly freeing and
 * allocating memory.
 *
 * @param msg  Old message to be reallocated.
 * @param type New message type.
 *
 * @return Reallocated message object.
 */
glproto_msg_t *glproto_msg_realloc(glproto_msg_t *msg, glproto_type_t type) {
	glproto_msg_t *ret;

	/* Ensure we don't rely on realloc() behaviour. */
	if (msg == NULL) {
		ret = malloc(glproto_msg_sizeof(type));
		goto finalize;
	}

	/* Does this even require reallocation? */
	if (msg->type == type)
		return msg;

	/* Reallocate our message. */
	ret = realloc(msg, glproto_msg_sizeof(type));

finalize:
	ret->type = type;
	return ret;
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

	/* Free the common parts. */
	if (msg->hostname) {
		free(msg->hostname);
		msg->hostname = NULL;
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
gl_err_t *glproto_msg_parse(glproto_msg_t **msg, void *rbuf, size_t len) {
	uint8_t *buf;
	glproto_type_t type;
	glproto_msg_t *nmsg;
	uint8_t i;

	/* Initialize some defaults. */
	buf = (uint8_t *)rbuf;
	type = GLPROTO_MSG_TYPE(buf);

	/* Allocate the memory needed to hold our parsed message. */
	*msg = glproto_msg_realloc(*msg, type);
	nmsg = *msg;
	nmsg->length = len;

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
		nmsg->device[i++] = *buf++;
	} while (i < 3);
	nmsg->device[3] = '\0';

	/* Parse hostname. */
	buf++;
	i = *buf++;
	nmsg->hostname = realloc(nmsg->hostname, i);
	strncpy(nmsg->hostname, (const char *)buf, i);
	nmsg->hostname[i - 1] = '\0';

	/* Parse any message type-specific headers. */
	switch (type) {
		default:
			break;
	}

	return NULL;
}



/**
 * Handles the incoming data and automatically filters out invalid packets.
 *
 * @param sock Server handle object.
 * @param type Returns the received message type.
 * @param msg  Returns a parsed message object.
 *
 * @return Error report or NULL if an unrecoverable error occurred.
 */
gl_err_t *glproto_recvfrom(sock_handle_t *sock, glproto_type_t *type,
						   glproto_msg_t **msg) {
	size_t len;
	size_t rlen;
	uint8_t peek[6];
	void *buf;
	gl_err_t *err;
	struct sockaddr_storage client;
	socklen_t client_len;

	/* Get the message head. */
	len = 0;
	err = socket_recvfrom(handle->sock, peek, 6, (struct sockaddr *)&client,
	                      &client_len, &len, true);
	if (err)
		return err;

	/* Check if the message head is valid. */
	if (len != 6) {
		log_printf(LOG_DEBUG, "Invalid message length received %ld expected 6",
		           len);
		*type = GLPROTO_TYPE_INVALID;
		*msg = glproto_invalid_msg;

		/* Skip the peeked bytes. */
		socket_recvfrom(handle->sock, peek, 6, NULL, NULL, &len, false);

		return NULL;
	} else if (!glproto_msg_head_isvalid(peek)) {
		log_printf(LOG_DEBUG, "Invalid message head received");
		*type = GLPROTO_TYPE_INVALID;
		*msg = glproto_invalid_msg;

		/* Skip the peeked bytes. */
		socket_recvfrom(handle->sock, peek, 6, NULL, NULL, &len, false);

		return NULL;
	}

	/* Allocate a buffer and get the full message. */
	len = ntohs(*(uint16_t *)(peek + 4));
	rlen = 0;
	buf = malloc(len);
	err = socket_recvfrom(handle->sock, buf, len, NULL, NULL, &rlen, false);
	if (err)
		goto cleanup;
	if (rlen != len) {
		err = gl_error_push(ERR_TYPE_SOCKET, SOCK_ERR_ERECV,
		                    EMSG("Number of bytes for message expected differ from read"));
		goto cleanup;
	}

	/* Parse the message. */
	*type = peek[2];
	err = glproto_msg_parse(msg, buf, len);

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
gl_err_t *glproto_msg_sendto(sock_handle_t *sock, glproto_msg_t *msg) {
	// TODO: Make this.
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
		default:
			gl_error_push(ERR_TYPE_GL, GL_ERR_PROTOCOL,
						  EMSG("Unknown message type to get sizeof"));
			exit(GL_ERR_PROTOCOL);
			return 0;
	}
}

/**
 * Prints out the contents of a message in a human-readable way. Extremely
 * useful for debugging.
 *
 * @param msg Message to be printed out.
 */
void glproto_msg_print(glproto_msg_t *msg) {
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
	printf("GLUPI: 0x%x/0x%x/0x%x/0x%x0x%x/0x%x/0x%x/0x%x\n",
		   msg->glupi[0], msg->glupi[1], msg->glupi[2], msg->glupi[3],
		   msg->glupi[4], msg->glupi[5], msg->glupi[6], msg->glupi[7]);
}
