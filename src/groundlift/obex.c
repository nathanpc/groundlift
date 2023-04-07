/**
 * obex.c
 * GroundLift OBEX implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "obex.h"
#include <stdlib.h>
#include <string.h>

/**
 * Creates a brand new OBEX packet object.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param opcode Opcode or return code for this brand new packet.
 * @param final  Should we set the final bit?
 *
 * @return Newly allocated packet object or NULL if we couldn't allocate the
 *         object.
 *
 * @see obex_packet_free
 */
obex_packet_t *obex_packet_new(obex_opcodes_t opcode, bool final) {
	obex_packet_t *packet;

	/* Allocate some memory for our packet object. */
	packet = (obex_packet_t *)malloc(sizeof(obex_packet_t));
	if (packet == NULL)
		return NULL;

	/* Ensure we have a known initial state for everything. */
	packet->opcode = (uint8_t)opcode;
	packet->size = 0;
	packet->headers = NULL;
	packet->header_count = 0;
	packet->body_length = 0;
	packet->body = NULL;

	/* Set the final bit if needed. */
	if (final)
		packet->opcode = OBEX_SET_FINAL_BIT(packet->opcode);

	return packet;
}

/**
 * Frees the packet and any resources allocated by it.
 *
 * @param packet Packet to be free'd.
 *
 * @see obex_packet_new
 */
void obex_packet_free(obex_packet_t *packet) {
	uint16_t i;

	/* Free our headers. */
	for (i = 0; i < packet->header_count; i++) {
		obex_header_free(packet->headers[i]);
		packet->headers[i] = NULL;
	}
	free(packet->headers);
	packet->headers = NULL;

	/* Free the body. */
	if (packet->body) {
		free(packet->body);
		packet->body = NULL;
	}

	/* Free ourselves. */
	free(packet);
	packet = NULL;
}

/**
 * Appends a header to the packet.
 *
 * @param packet Packet to have the header appended to.
 * @param header Header to be appended to the packet. (Won't be copied)
 *
 * @return TRUE if the operation was successful.
 */
bool obex_packet_header_add(obex_packet_t *packet, obex_header_t *header) {
	/* Reallocate the memory for the headers array. */
	packet->headers = (obex_header_t **)realloc(packet->headers,
		(packet->header_count + 1) * sizeof(obex_header_t*));
	if (packet->headers == NULL)
		return false;

	/* Increment the header count and set the new packet. */
	packet->headers[packet->header_count] = header;
	packet->header_count++;

	return true;
}

/**
 * Sets the body of an OBEX packet.
 *
 * @param packet Packet to have the body appended to.
 * @param size   Size of the contents of the body in bytes.
 * @param body   Body contents. (Won't be copied by this function, so it must
 *               remain valid for the lifetime of the packet and will be free'd
 *               when the packet is free'd.)
 *
 * @see obex_packet_body_copy
 */
void obex_packet_body_set(obex_packet_t *packet, uint16_t size, void *body) {
	packet->body_length = size;
	packet->body = body;
}

/**
 * Copies the contents of the source into the body of a packet.
 *
 * @param packet Packet to have the body appended to.
 * @param size   Size of the contents of the body in bytes.
 * @param src    Body contents. (Won't be copied by this function, so it must
 *               remain valid for the lifetime of the packet and will be free'd
 *               when the packet is free'd.)
 *
 * @return TRUE when the operation was successful.
 *
 * @see obex_packet_body_set
 */
bool obex_packet_body_copy(obex_packet_t *packet, uint16_t size, const void *src) {
	void *body;

	/* Allocate memory for the body contents. */
	body = malloc(size);
	if (body == NULL)
		return false;

	/* Copy the body to the newly allocated memory. */
	memcpy(body, src, size);

	/* Set the packet body. */
	obex_packet_body_set(packet, size, body);

	return true;
}

/**
 * Creates a brand new OBEX header object.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param id Header ID.
 *
 * @return Newly allocated header object or NULL if we couldn't allocate the
 *         object.
 *
 * @see obex_header_free
 */
obex_header_t *obex_header_new(obex_header_id_t id) {
	obex_header_t *header;

	/* Allocate some memory for our header object. */
	header = (obex_header_t *)malloc(sizeof(obex_header_t));
	if (header == NULL)
		return NULL;

	/* Ensure we have a known initial state for everything. */
	memset(header, 0, sizeof(obex_header_t));
	header->identifier.id = (uint8_t)id;

	return header;
}

/**
 * Frees the header and any resources allocated by it.
 *
 * @param header Header to be free'd.
 *
 * @see obex_header_new
 */
void obex_header_free(obex_header_t *header) {
	/* Free strings that might have been allocated. */
	switch (header->identifier.fields.encoding) {
		case OBEX_HEADER_ENCODING_UTF16:
			free(header->value.wstring.text);
			header->value.wstring.text = NULL;
			break;
		case OBEX_HEADER_ENCODING_STRING:
			free(header->value.string.text);
			header->value.string.text = NULL;
			break;
		default:
			break;
	}

	/* Free ourselves. */
	free(header);
	header = NULL;
}

/**
 * Copies a string into the header and sets the appropriate length. Will only
 * operate if the header ID has the correct encoding.
 *
 * @param header Header object.
 * @param str    String to be copied into the header.
 *
 * @return TRUE if the operation was successful and the header ID encoding is
 *         correct.
 */
bool obex_header_string_copy(obex_header_t *header, const char *str) {
	/* Do we have the right encoding to operate on? */
	if (header->identifier.fields.encoding != OBEX_HEADER_ENCODING_STRING)
		return false;

	/* Get the string length and allocate some memory for it. */
	header->value.string.length = (uint16_t)strlen(str);
	header->value.string.text = (char *)realloc(header->value.string.text,
		(header->value.string.length + 1) * sizeof(char));
	if (header->value.string.text == NULL)
		return false;

	/* Actually copy the string over. */
	strcpy(header->value.string.text, str);

	return true;
}

/**
 * Copies a wide string into the header and sets the appropriate length. Will
 * only operate if the header ID has the correct encoding.
 *
 * @param header Header object.
 * @param wstr   Wide string to be copied into the header.
 *
 * @return TRUE if the operation was successful and the header ID encoding is
 *         correct.
 */
bool obex_header_wstring_copy(obex_header_t *header, const wchar_t *wstr) {
	/* Do we have the right encoding to operate on? */
	if (header->identifier.fields.encoding != OBEX_HEADER_ENCODING_UTF16)
		return false;

	/* Get the string length and allocate some memory for it. */
	header->value.wstring.length = (uint16_t)wcslen(wstr);
	header->value.wstring.text = (wchar_t *)realloc(header->value.wstring.text,
		(header->value.wstring.length + 1) * sizeof(wchar_t));
	if (header->value.wstring.text == NULL)
		return false;

	/* Actually copy the wide string over. */
	wcscpy(header->value.wstring.text, wstr);

	return true;
}

/**
 * Prints the contents of a packet in a human-readable, debug-friendly, way.
 *
 * @param packet OBEX packet to have its contents inspected.
 *
 * @see obex_print_header
 */
void obex_print_packet(const obex_packet_t *packet) {
	uint16_t i;

	/* Opcode or response code. */
	switch (packet->opcode) {
		case OBEX_OPCODE_CONNECT:
			printf("CONNECT ");
			break;
		case OBEX_OPCODE_DISCONNECT:
			printf("DISCONNECT ");
			break;
		case OBEX_OPCODE_PUT:
			printf("PUT ");
			break;
		case OBEX_OPCODE_GET:
			printf("GET ");
			break;
		case OBEX_OPCODE_SETPATH:
			printf("SETPATH ");
			break;
		case OBEX_OPCODE_ACTION:
			printf("ACTION ");
			break;
		case OBEX_OPCODE_SESSION:
			printf("SESSION ");
			break;
		case OBEX_OPCODE_ABORT:
			printf("ABORT ");
			break;
		case OBEX_RESPONSE_CONTINUE:
			printf("CONTINUE ");
			break;
		case OBEX_RESPONSE_SUCCESS:
			printf("SUCCESS ");
			break;
		default:
			printf("OTHER ");
			break;
	}

	/* Show opcode in hex and display the packet length and header count. */
	printf("[0x%02X] %u bytes and %u headers\n", packet->opcode, packet->size,
		   packet->header_count);

	/* Print out the headers. */
	for (i = 0; i < packet->header_count; i++) {
		obex_print_header(packet->headers[i], i == 0);
		printf("\n");
	}

	/* Print out the data body. */
	if (packet->body != NULL) {
		const unsigned char *p = (const unsigned char *)packet->body;

		/* Print out each byte of the body. */
		printf("\n");
		for (i = 0; i < packet->body_length; i++) {
			printf("%02X ", p[i]);
		}

		printf("\n");
	}

	printf("\n");
}

/**
 * Prints the contents of a header in a human-readable, debug-friendly, way.
 *
 * @param header Header to have its contents printed out.
 * @param th     Print a table labels header?
 *
 * @see obex_print_packet
 */
void obex_print_header(const obex_header_t *header, bool th) {
	/* Print table header. */
	if (th)
		printf("HI\tEncoding\tCode\tValue\n");

	/* Print header identifier and its various bits separated. */
	printf("0x%02X\t", header->identifier.id);
	obex_print_header_encoding(header);
	printf("\t0x%02X\t", header->identifier.fields.meaning);

	/* Print the header value. */
	obex_print_header_value(header);

	printf("\n");
}

/**
 * Prints the encoding type of a header in a human-readable way.
 *
 * @param header Header object to be inspected.
 */
void obex_print_header_encoding(const obex_header_t *header) {
	switch (header->identifier.fields.encoding) {
		case OBEX_HEADER_ENCODING_UTF16:
			printf("UTF-16 (0x%02X)", header->identifier.fields.encoding);
			break;
		case OBEX_HEADER_ENCODING_STRING:
			printf("String (0x%02X)", header->identifier.fields.encoding);
			break;
		case OBEX_HEADER_ENCODING_BYTE:
			printf("Byte (0x%02X)", header->identifier.fields.encoding);
			break;
		case OBEX_HEADER_ENCODING_WORD32:
			printf("4 Bytes (0x%02X)", header->identifier.fields.encoding);
			break;
		default:
			printf("Unknown (0x%02X)", header->identifier.fields.encoding);
			break;
	}
}

/**
 * Prints the value of a header in a human-readable way.
 *
 * @param header Header object to be inspected.
 */
void obex_print_header_value(const obex_header_t *header) {
	switch (header->identifier.fields.encoding) {
		case OBEX_HEADER_ENCODING_UTF16:
			printf("(%u) L\"%ls\"", (header->value.wstring.length * 2) + 3,
				   header->value.wstring.text);
			break;
		case OBEX_HEADER_ENCODING_STRING:
			printf("(%u) \"%s\"", header->value.string.length + 3,
				   header->value.string.text);
			break;
		case OBEX_HEADER_ENCODING_BYTE:
			printf("0x%02X", header->value.byte);
			break;
		case OBEX_HEADER_ENCODING_WORD32:
			printf("%u", header->value.word32);
			break;
		default:
			printf("Printing the contents of an unknown header encoding is "
				   "currently not supported.");
			break;
	}
}
