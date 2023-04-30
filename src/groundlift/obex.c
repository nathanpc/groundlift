/**
 * obex.c
 * GroundLift OBEX implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "obex.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include "bitutils.h"
#include "conf.h"
#include "defaults.h"
#include "sockets.h"
#include "utf16utils.h"

/* Public variables. */
obex_packet_t *obex_invalid_packet;

/* Private variables. */
static obex_packet_t void_packet;

/* Private methods. */
bool obex_populate_name_length_headers(obex_packet_t *packet, const char *name, const uint32_t *len);
void *memcpy_n(void *dest, const void *src, size_t len);

/**
 * Initializes some of the internal states of the OBEX module.
 */
void obex_init(void) {
	/* Initialize the invalid packet. */
	obex_invalid_packet = &void_packet;
	void_packet.opcode = OBEX_OPCODE_ABORT;
	void_packet.size = 0;
	void_packet.params_count = 0;
	void_packet.params = NULL;
	void_packet.headers = NULL;
	void_packet.header_count = 0;
	void_packet.body_end = false;
	void_packet.body_length = 0;
	void_packet.body = NULL;
}

/**
 * Creates a brand new OBEX packet object.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param opcode    Opcode or return code for this brand new packet.
 * @param set_final Should we set the final bit?
 *
 * @return Newly allocated packet object or NULL if we couldn't allocate the
 *         object.
 *
 * @see obex_packet_free
 */
obex_packet_t *obex_packet_new(obex_opcodes_t opcode, bool set_final) {
	obex_packet_t *packet;

	/* Allocate some memory for our packet object. */
	packet = (obex_packet_t *)malloc(sizeof(obex_packet_t));
	if (packet == NULL)
		return NULL;

	/* Ensure we have a known initial state for everything. */
	packet->opcode = (uint8_t)opcode;
	packet->size = 0;
	packet->params_count = 0;
	packet->params = NULL;
	packet->headers = NULL;
	packet->header_count = 0;
	packet->body_end = false;
	packet->body_length = 0;
	packet->body = NULL;

	/* Set the final bit if needed. */
	if (set_final)
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

	/* Do we even have a packet to free? */
	if (packet == NULL)
		return;

	/* Free our parameters. */
	if (packet->params) {
		free(packet->params);
		packet->params = NULL;
	}

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
 * Appends a parameter to a packet.
 *
 * @param packet Packet to have the parameter appended to.
 * @param value  Value of the parameter.
 * @param size   Size in bytes of the parameter.
 *
 * @return TRUE if the operation was successful.
 */
bool obex_packet_param_add(obex_packet_t *packet, uint16_t value, uint8_t size) {
	/* Reallocate the memory for the parameters array. */
	packet->params = (obex_packet_param_t *)realloc(packet->params,
		(packet->params_count + 1) * sizeof(obex_packet_param_t));

	/* Ensure we were able to allocate the memory. */
	if (packet->params == NULL) {
		packet->params_count = 0;
		return false;
	}

	/* Append the new parameter and increment the parameter count. */
	packet->params[packet->params_count].size = size;
	switch (size) {
		case 1:
			packet->params[packet->params_count].value.byte = (value & 0xFF);
			break;
		case 2:
			packet->params[packet->params_count].value.uint16 = value;
			break;
		default:
			fprintf(stderr, "obex_packet_param_add: Invalid parameter size: "
					"%u\n", size);
			return false;
	}
	packet->params_count++;

	return true;
}

/**
 * Appends a header to a packet.
 *
 * @param packet Packet to have the header appended to.
 * @param header Header to be appended to the packet. (Will be directly assigned
 *               to the packet by this function, so it must remain valid for the
 *               lifetime of the packet and will be free'd when the packet is
 *               free'd.)
 *
 * @return TRUE if the operation was successful.
 */
bool obex_packet_header_add(obex_packet_t *packet, obex_header_t *header) {
	/* Reallocate the memory for the headers array. */
	packet->headers = (obex_header_t **)realloc(packet->headers,
		(packet->header_count + 1) * sizeof(obex_header_t*));

	/* Ensure we were able to allocate the memory. */
	if (packet->headers == NULL) {
		packet->header_count = 0;
		return false;
	}

	/* Append the new header and increment the header count. */
	packet->headers[packet->header_count] = header;
	packet->header_count++;

	return true;
}

/**
 * Appends a hostname header to a packet.
 *
 * @param packet Packet to have the hostname header appended to.
 *
 * @return TRUE if the operation was successful.
 */
bool obex_packet_header_add_hostname(obex_packet_t *packet) {
	obex_header_t *header;
	bool ret;

	/* Create the header object. */
	header = obex_header_new(OBEX_HEADER_EXT_HOSTNAME);
	if (header == NULL)
		return false;

	/* Copy the hostname from the configuration into the header. */
	if (!obex_header_string_copy(header, conf_get_hostname())) {
		obex_header_free(header);
		return false;
	}

	/* Add the header to the packet. */
	ret = obex_packet_header_add(packet, header);
	if (!ret)
		obex_header_free(header);

	return ret;
}

/**
 * Removes the last header from a packet.
 *
 * @param packet Packet to have the header popped from.
 *
 * @return TRUE if the operation was successful.
 */
bool obex_packet_header_pop(obex_packet_t *packet) {
	/* Do we even have anything to pop? */
	if (packet->header_count == 0)
		return false;

	/* Free the last header. */
	packet->header_count--;
	obex_header_free(packet->headers[packet->header_count]);
	packet->headers[packet->header_count] = NULL;

	/* Do we want to pop everything? */
	if (packet->header_count == 0) {
		free(packet->headers);
		packet->headers = NULL;

		return true;
	}

	/* Reallocate the memory for the headers array. */
	packet->headers = (obex_header_t **)realloc(packet->headers,
		packet->header_count * sizeof(obex_header_t *));

	/* Ensure we were able to allocate the memory. */
	if (packet->headers == NULL) {
		packet->header_count = 0;
		return false;
	}

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
 * @param eob    Should we set the End of Body flag?
 *
 * @see obex_packet_body_copy
 */
void obex_packet_body_set(obex_packet_t *packet, uint16_t size, void *body, bool eob) {
	packet->body_length = size;
	packet->body = body;
	packet->body_end = eob;
}

/**
 * Copies the contents of the source into the body of a packet.
 *
 * @param packet Packet to have the body appended to.
 * @param size   Size of the contents of the body in bytes.
 * @param src    Body contents.
 * @param eob    Should we set the End of Body flag?
 *
 * @return TRUE when the operation was successful.
 *
 * @see obex_packet_body_set
 */
bool obex_packet_body_copy(obex_packet_t *packet, uint16_t size, const void *src, bool eob) {
	void *body;

	/* Allocate memory for the body contents. */
	body = malloc(size);
	if (body == NULL)
		return false;

	/* Copy the body to the newly allocated memory. */
	memcpy(body, src, size);

	/* Set the packet body. */
	obex_packet_body_set(packet, size, body, eob);

	return true;
}

/**
 * Recalculates the network size of the packet taking into account everything
 * that's populated within it.
 *
 * @param packet Packet object to have its network size updated.
 *
 * @return New network size of the entire packet.
 */
uint16_t obex_packet_size_refresh(obex_packet_t *packet) {
	uint16_t i;

	/* A packet must contain at least the opcode and the length. */
	packet->size = 3;

	/* Any parameters to account for? */
	if (packet->params != NULL) {
		for (i = 0; i < packet->params_count; i++) {
			packet->size += packet->params[i].size;
		}
	}

	/* Do we have a body to sum up as well? */
	if ((packet->body != NULL) || (packet->body_end))
		packet->size += packet->body_length + 3;

	/* Better count those headers! */
	for (i = 0; i < packet->header_count; i++) {
		packet->size += obex_header_size(packet->headers[i]);
	}

	return packet->size;
}

/**
 * Searches of a specific header in a packet.
 *
 * @param packet Packet to have its headers searched through.
 * @param hid    ID of the header to search for.
 *
 * @return Header that was being searched for or NULL if one wasn't found.
 */
obex_header_t *obex_packet_header_find(const obex_packet_t *packet, obex_header_id_t hid) {
	uint16_t i;

	/* Go through the headers searching for a match. */
	for (i = 0; i < packet->header_count; i++) {
		if (packet->headers[i]->identifier.id == hid)
			return packet->headers[i];
	}

	return NULL;
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
			if (header->value.wstring.text)
				free(header->value.wstring.text);
			header->value.wstring.text = NULL;
			break;
		case OBEX_HEADER_ENCODING_STRING:
			if (header->value.string.text)
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
 * Sets a string into the header and sets the appropriate length. This string
 * pointer must survive the entire lifetime of the header. Will only operate if
 * the header ID has the correct encoding.
 *
 * @param header Header object.
 * @param str    String to be set into the header.
 *
 * @return TRUE if the header ID encoding is correct.
 */
bool obex_header_string_set(obex_header_t *header, char *str) {
	/* Do we have the right encoding to operate on? */
	if (header->identifier.fields.encoding != OBEX_HEADER_ENCODING_STRING)
		return false;

	/* Get the string length and sets the pointer. */
	header->value.string.length = (uint16_t)strlen(str);
	header->value.string.text = str;

	return true;
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
 * Sets a wide string into the header and sets the appropriate length. This wide
 * string pointer must survive the entire lifetime of the header. Will only
 * operate if the header ID has the correct encoding.
 *
 * @param header Header object.
 * @param wstr   Wide string to be set into the header.
 *
 * @return TRUE if the header ID encoding is correct.
 */
bool obex_header_wstring_set(obex_header_t *header, wchar_t *wstr) {
	/* Do we have the right encoding to operate on? */
	if (header->identifier.fields.encoding != OBEX_HEADER_ENCODING_UTF16)
		return false;

	/* Get the string length and sets the pointer. */
	header->value.wstring.length = (uint16_t)wcslen(wstr);
	header->value.wstring.text = wstr;

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
 * Gets the full size of a header as it will be when sent in a packet. This will
 * take into account the header ID, encoding, and the length field itself.
 *
 * @param header Header object.
 *
 * @return Size of the header as it should be used in the length field.
 */
uint16_t obex_header_size(const obex_header_t *header) {
	switch (header->identifier.fields.encoding) {
		case OBEX_HEADER_ENCODING_UTF16:
			return ((header->value.wstring.length + 1) * 2) + 3;
		case OBEX_HEADER_ENCODING_STRING:
			return (header->value.string.length + 1) + 3;
		case OBEX_HEADER_ENCODING_BYTE:
			return sizeof(uint8_t) + 1;
		case OBEX_HEADER_ENCODING_WORD32:
			return sizeof(int32_t) + 1;
		default:
			fprintf(stderr, "obex_header_size: Unknown header encoding: %u\n",
					header->identifier.fields.encoding);
			return 0;
	}
}

/**
 * Encodes an OBEX packet structure to be transferred over the network. Will
 * also set the appropriate size of the final packet to the structure to be used
 * when sending.
 *
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param packet Packet object to be encoded.
 * @param buf    Pointer to a buffer that will contain the encoded data. (Will
 *               be allocated by this function)
 *
 * @return TRUE if the operation was successful.
 */
bool obex_packet_encode(obex_packet_t *packet, void **buf) {
	uint16_t i;
	uint16_t n;
	void *p;

	/* Calculate the length of the whole packet */
	obex_packet_size_refresh(packet);

	/* Allocate some memory for our packet buffer. */
	*buf = malloc(packet->size);
	if (*buf == NULL) {
		buf = NULL;
		return false;
	}

	/* Start copying over the opcode and the length of the packet. */
	p = *buf;
	p = memcpy_n(p, &packet->opcode, 1);
	n = htons(packet->size);
	p = memcpy_n(p, &n, 2);

	/* Copy over any parameters that we might have. */
	for (i = 0; i < packet->params_count; i++) {
		obex_packet_param_t param = packet->params[i];

		switch (param.size) {
			case 1:
				p = memcpy_n(p, &param.value.byte, 1);
				break;
			case 2:
				n = htons(param.value.uint16);
				p = memcpy_n(p, &n, sizeof(uint16_t));
				break;
			default:
				fprintf(stderr, "obex_packet_encode: Invalid packet parameter "
						"size: %u\n", param.size);
				free(*buf);
				*buf = NULL;

				return false;
		}
	}

	/* Copy over the headers. */
	for (i = 0; i < packet->header_count; i++) {
		p = obex_packet_encode_header_memcpy(packet->headers[i], p);
	}

	/* Copy over the body over if needed. */
	if ((packet->body != NULL) || packet->body_end) {
		uint8_t hi;

		/* Header identifier. */
		hi = (packet->body_end) ? OBEX_HEADER_END_BODY : OBEX_HEADER_BODY;
		p = memcpy_n(p, &hi, 1);

		/* Header length */
		n = htons(packet->body_length + 3);
		p = memcpy_n(p, &n, sizeof(uint16_t));

		/* Body contents. */
		if ((packet->body_length > 0) && (packet->body != NULL))
			p = memcpy_n(p, packet->body, packet->body_length);
	}

	(void)p;
	return true;
}

/**
 * Encodes a header structure to be transferred over the network and appends it
 * to the packet buffer returning the next byte just like memcpy_n.
 *
 * @param header Header object.
 * @param buf    Buffer to have the header copied to.
 *
 * @return Next byte in the buffer after the copied data. (memcpy_n returned)
 *
 * @see obex_packet_encode
 * @see memcpy_n
 */
void *obex_packet_encode_header_memcpy(const obex_header_t *header, void *buf) {
	void *p;

	/* Start by copying the header identifier. */
	p = buf;
	p = memcpy_n(p, &header->identifier.id, 1);

	/* Copy the value over. */
	switch (header->identifier.fields.encoding) {
		case OBEX_HEADER_ENCODING_UTF16: {
			uint16_t len;
			const wchar_t *wstr;
			uint16_t wc;

			/* Copy length first. */
			len = htons(obex_header_size(header));
			p = memcpy_n(p, &len, sizeof(uint16_t));

			/* Copy the wide string over. */
			wstr = header->value.wstring.text;
			while (*wstr != L'\0') {
				wc = htons(utf16_conv_ltos(*wstr));
				p = memcpy_n(p, &wc, sizeof(uint16_t));

				wstr++;
			}
			wc = htons(L'\0');
			p = memcpy_n(p, &wc, sizeof(uint16_t));

			break;
		}
		case OBEX_HEADER_ENCODING_STRING: {
			uint16_t len;

			/* Copy length first. */
			len = htons(obex_header_size(header));
			p = memcpy_n(p, &len, sizeof(uint16_t));

			/* Copy the string over. */
			p = memcpy_n(p, header->value.string.text,
						 header->value.string.length + 1);

			break;
		}
		case OBEX_HEADER_ENCODING_BYTE:
			p = memcpy_n(p, &header->value.byte, sizeof(uint8_t));
			break;
		case OBEX_HEADER_ENCODING_WORD32: {
			uint32_t n = htonl(header->value.word32);
			p = memcpy_n(p, &n, sizeof(uint32_t));
			break;
		}
		default:
			fprintf(stderr, "obex_packet_encode_header_memcpy: Unknown header "
					"encoding: %u\n", header->identifier.fields.encoding);
			return 0;
	}

	return p;
}

/**
 * Decodes an OBEX network packet into an OBEX packet object.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param buf        OBEX network packet buffer.
 * @param len        Length of the buffer in bytes.
 * @param has_params Does this packet contain parameters?
 *
 * @return Fully populated OBEX packet, obex_invalid_packet if the packet was
 *         invalid or NULL if an error occurred.
 */
obex_packet_t *obex_packet_decode(const void *buf, uint16_t len, bool has_params) {
	obex_packet_t *packet;
	const uint8_t *cur;
	uint16_t rlen;

	/* Set some starting values. */
	rlen = 0;
	cur = (const uint8_t *)buf;

	/* Create our packet object. */
	packet = obex_packet_new(*cur, false);
	cur++;
	rlen++;

	/* Read the packet size. */
	packet->size = len;
	cur += 2;
	rlen += 2;

	/* Deal with the parameters if needed. */
	if (has_params) {
		switch (packet->opcode) {
			case OBEX_OPCODE_CONNECT:
			case OBEX_SET_FINAL_BIT(OBEX_RESPONSE_SUCCESS):
			case OBEX_SET_FINAL_BIT(OBEX_RESPONSE_UNAUTHORIZED):
				/* Protocol version. */
				obex_packet_param_add(packet, *cur, 1);
				cur++;
				rlen++;

				/* Connection flags. */
				obex_packet_param_add(packet, *cur, 1);
				cur++;
				rlen++;

				/* Packet maximum length. */
				obex_packet_param_add(packet, U8BUF_TO_USHORT(cur), 2);
				cur += 2;
				rlen += 2;
				break;
			default:
				/* Unhandled opcode. */
				fprintf(stderr, "obex_packet_decode: Invalid opcode 0x%02X "
						"received with has_params flag.\n", packet->opcode);
				obex_packet_free(packet);
				return obex_invalid_packet;
		}
	}

	/* Decode the headers. */
	while (rlen < len) {
		/* Start creating a new header. */
		obex_header_t *header = obex_header_new(*cur);
		cur++;
		rlen++;

		/* Preemptively append the header to the packet. */
		if (!obex_packet_header_add(packet, header)) {
			obex_header_free(header);
			obex_packet_free(packet);

			return NULL;
		}

		/* Copy the data or just the length depending on the encoding. */
		switch (header->identifier.fields.encoding) {
			case OBEX_HEADER_ENCODING_UTF16:
				header->value.wstring.length =
					(U8BUF_TO_USHORT(cur) - 3 - 2) / 2;
				cur += 2;
				rlen += 2;
				break;
			case OBEX_HEADER_ENCODING_STRING:
				header->value.string.length = U8BUF_TO_USHORT(cur) - 4;
				cur += 2;
				rlen += 2;
				break;
			case OBEX_HEADER_ENCODING_BYTE:
				header->value.byte = *cur;
				cur++;
				rlen++;
				continue;
			case OBEX_HEADER_ENCODING_WORD32:
				header->value.word32 = U8BUF_TO_U32(cur);
				cur += 4;
				rlen += 4;
				continue;
			default:
				fprintf(stderr, "obex_packet_decode: Invalid header identifier "
						"encoding.\n");
				obex_packet_free(packet);
				return obex_invalid_packet;
		}

		/* Copy over some of the special values. */
		if ((header->identifier.id == OBEX_HEADER_BODY) ||
			(header->identifier.id == OBEX_HEADER_END_BODY)) {
			/* Body header. */
			uint16_t blen;
			bool eob;

			/* Get the last bit of information needed from the header. */
			eob = header->identifier.id == OBEX_HEADER_END_BODY;

			/* Remove this header from the packet since it will be redundant. */
			if (!obex_packet_header_pop(packet)) {
				obex_packet_free(packet);
				return NULL;
			}

			/* Copy the body over. */
			blen = len - rlen;
			if (!obex_packet_body_copy(packet, blen, cur, eob)) {
				obex_packet_free(packet);
				return NULL;
			}

			cur += blen;
			rlen += blen;
		} else if (header->identifier.fields.encoding == OBEX_HEADER_ENCODING_STRING) {
			/* Header containing an regular string. */
			char *str;
			uint16_t str_len;
			uint16_t c;

			/* Allocate some memory for our string. */
			str_len = header->value.string.length + 1;
			header->value.string.text = (char *)malloc(str_len * sizeof(char));
			if (header->value.string.text == NULL) {
				obex_packet_free(packet);
				return NULL;
			}

			/* Copy over the contents to our string. */
			str = header->value.string.text;
			for (c = 0; c < str_len; c++) {
				str[c] = *cur;
				cur++;
				rlen++;
			}
		} else if (header->identifier.fields.encoding == OBEX_HEADER_ENCODING_UTF16) {
			/* Header containing an UTF-16 string. */
			wchar_t *wstr;
			uint16_t wstr_len;
			uint16_t c;

			/* Allocate some memory for our string. */
			wstr_len = header->value.wstring.length + 1;
			header->value.wstring.text =
				(wchar_t *)malloc(wstr_len * sizeof(wchar_t));
			if (header->value.wstring.text == NULL) {
				obex_packet_free(packet);
				return NULL;
			}

			/* Copy over the contents to our string. */
			wstr = header->value.wstring.text;
			for (c = 0; c < wstr_len; c++) {
				wstr[c] = U8BUF_TO_WCHAR(cur);
				cur += 2;
				rlen += 2;
			}
		}
	}

	return packet;
}

/**
 * Sends an OBEX TCP packet over the network.
 *
 * @param sockfd Socket to send the packet over.
 * @param packet Packet object to be sent.
 *
 * @return An error object if an error occurred or NULL if it was successful.
 */
gl_err_t *obex_net_packet_send(int sockfd, obex_packet_t *packet) {
	gl_err_t *err;
	tcp_err_t tcp_err;
	void *buf;

	/* Do we have a packet to send? */
	if (packet == NULL) {
		return gl_error_new(ERR_TYPE_GL, OBEX_ERR_NO_PACKET,
							EMSG("No packet supplied to be sent"));
	}

	/* Initialize variables. */
	buf = NULL;
	err = NULL;

	/* Get the packet network buffer. */
	if (!obex_packet_encode(packet, &buf)) {
		err = gl_error_new(ERR_TYPE_OBEX, OBEX_ERR_ENCODE,
						   EMSG("Failed to encode the OBEX connect packet"));
		goto cleanup;
	}

	/* Get the network buffer for the packet and send it out. */
	tcp_err = tcp_socket_send(sockfd, buf, packet->size, NULL);
	if (tcp_err != SOCK_OK) {
		err = gl_error_new(ERR_TYPE_TCP, (int8_t)tcp_err,
						   EMSG("Failed to send OBEX packet"));
		goto cleanup;
	}

cleanup:
	/* Free up any resources. */
	if (buf)
		free(buf);

	return err;
}

/**
 * Sends an OBEX UDP packet over the network.
 *
 * @param sock   UDP socket bundle to send the packet over.
 * @param packet Packet object to be sent.
 *
 * @return An error object if an error occurred or NULL if it was successful.
 */
gl_err_t *obex_net_packet_sendto(sock_bundle_t sock, obex_packet_t *packet) {
	gl_err_t *err;
	tcp_err_t tcp_err;
	void *buf;

	/* Do we have a packet to send? */
	if (packet == NULL) {
		return gl_error_new(ERR_TYPE_GL, OBEX_ERR_NO_PACKET,
							EMSG("No packet supplied to be sent"));
	}

	/* Initialize variables. */
	buf = NULL;
	err = NULL;

	/* Get the packet network buffer. */
	if (!obex_packet_encode(packet, &buf)) {
		err = gl_error_new(ERR_TYPE_OBEX, OBEX_ERR_ENCODE,
						   EMSG("Failed to encode the OBEX connect packet"));
		goto cleanup;
	}

	/* Get the network buffer for the packet and send it out. */
	tcp_err = udp_socket_send(sock.sockfd, buf, packet->size,
		(const struct sockaddr *)&sock.addr_in, sock.addr_in_size, NULL);
	if (tcp_err != SOCK_OK) {
		err = gl_error_new(ERR_TYPE_TCP, (int8_t)tcp_err,
						   EMSG("Failed to send OBEX packet"));
		goto cleanup;
	}

cleanup:
	/* Free up any resources. */
	if (buf)
		free(buf);

	return err;
}

/**
 * Handles the reception of an OBEX TCP packet via the network.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param sockfd     Socket to read our packet from.
 * @param has_params Does this packet contain parameters?
 *
 * @return Decoded packet object or NULL if the received packet was invalid.
 */
obex_packet_t *obex_net_packet_recv(int sockfd, bool has_params) {
	size_t len;
	size_t plen;
	tcp_err_t tcp_err;
	obex_packet_t *packet;
	uint16_t psize;
	uint8_t *tmp;
	uint8_t *buf;
	uint8_t peek_buf[3];

	/* Receive the packet's opcode and length. */
	tcp_err = tcp_socket_recv(sockfd, peek_buf, 3, &len, true);
	if ((tcp_err >= SOCK_OK) && (len != 3)) {
		fprintf(stderr, "obex_net_packet_recv: Failed to receive OBEX packet "
				"peek. (tcp_err %d len %lu)\n", tcp_err, len);
		return obex_invalid_packet;
	} else if ((tcp_err == SOCK_EVT_CONN_CLOSED) ||
			   (tcp_err == SOCK_EVT_CONN_SHUTDOWN)) {
		return NULL;
	}

	/* Allocate memory to receive the entire packet. */
	psize = BYTES_TO_USHORT(peek_buf[1], peek_buf[2]);
	if (psize > OBEX_MAX_PACKET_SIZE) {
		fprintf(stderr, "obex_net_packet_recv: Peek'd packet length %u is "
				"greater than the allowed %u.\n", psize, OBEX_MAX_PACKET_SIZE);
		return obex_invalid_packet;
	}
	buf = malloc(psize);

	/* Read the full packet that was sent. */
	tmp = buf;
	len = 0;
	while (len < psize) {
		tcp_err = tcp_socket_recv(sockfd, tmp, psize, &plen, false);
		if (tcp_err != SOCK_OK) {
			fprintf(stderr, "obex_net_packet_recv: Failed to receive full OBEX "
					"packet. (tcp_err %d psize %u plen %lu len %lu)\n",
					tcp_err, psize, plen, len);
			free(buf);

			return obex_invalid_packet;
		}

		tmp += plen;
		len += plen;
	}

	/* Decode the packet and free up any temporary resources. */
	packet = obex_packet_decode(buf, psize, has_params);
	free(buf);
	if (packet == NULL)
		return obex_invalid_packet;

	return packet;
}

/**
 * Handles the reception of an OBEX UDP packet via the network.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param sock       UDP socket bundle to read our packet from.
 * @param expected   Opcode that we are expected to receive (fails if different)
 *                   or NULL if we want to receive everything.
 * @param has_params Does this packet contain parameters?
 *
 * @return Decoded packet object, obex_invalid_packet if the received packet was
 *         invalid or NULL if an error occurred.
 */
obex_packet_t *obex_net_packet_recvfrom(sock_bundle_t *sock, const obex_opcodes_t *expected, bool has_params) {
	size_t len;
	size_t plen;
	tcp_err_t tcp_err;
	obex_packet_t *packet;
	uint16_t psize;
	uint8_t *tmp;
	uint8_t *buf;
	uint8_t peek_buf[3];

	/* Receive the packet's opcode and length. */
	len = 0;
	tcp_err = udp_socket_recv(sock->sockfd, peek_buf, 3,
		(struct sockaddr *)&sock->addr_in, &sock->addr_in_size, &len, true);
	if ((tcp_err == SOCK_OK) && (expected) && (peek_buf[0] != *expected)) {
		fprintf(stderr, "obex_net_packet_recvfrom: Opcode (0x%02X) not what "
				"was expected (0x%02X).\n", peek_buf[0], *expected);
		return obex_invalid_packet;
	} else if ((tcp_err >= SOCK_OK) && (len != 3)) {
		fprintf(stderr, "obex_net_packet_recvfrom: Failed to receive OBEX "
				"packet peek. (tcp_err %d len %lu)\n", tcp_err, len);
		return obex_invalid_packet;
	} else if ((tcp_err == SOCK_EVT_CONN_CLOSED) ||
			(tcp_err == SOCK_EVT_CONN_SHUTDOWN)) {
		return NULL;
	}

	/* Allocate memory to receive the entire packet. */
	psize = BYTES_TO_USHORT(peek_buf[1], peek_buf[2]);
	if (psize > OBEX_MAX_PACKET_SIZE) {
		fprintf(stderr, "obex_net_packet_recvfrom: Peek'd packet length %u is "
				"greater than the allowed %u.\n", psize, OBEX_MAX_PACKET_SIZE);
		return obex_invalid_packet;
	}
	buf = malloc(psize);

	/* Read the full packet that was sent. */
	tmp = buf;
	len = 0;
	plen = 0;
	while (len < psize) {
		tcp_err = udp_socket_recv(sock->sockfd, tmp, psize,
			(struct sockaddr *)&sock->addr_in, &sock->addr_in_size, &plen,
			false);
		if (tcp_err != SOCK_OK) {
			fprintf(stderr, "obex_net_packet_recvfrom: Failed to receive full "
					"OBEX packet. (tcp_err %d psize %u plen %lu len %lu)\n",
					tcp_err, psize, plen, len);
			free(buf);

			return obex_invalid_packet;
		}

		tmp += plen;
		len += plen;
	}

	/* Decode the packet and free up any temporary resources. */
	packet = obex_packet_decode(buf, psize, has_params);
	free(buf);
	if (packet == NULL)
		return obex_invalid_packet;

	return packet;
}

/**
 * Creates a brand new CONNECT packet.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param fname Name of the file to be transferred.
 * @param fsize Size of the file to be transferred in bytes.
 *
 * @return New populated CONNECT packet or NULL if we were unable to create it.
 */
obex_packet_t *obex_packet_new_connect(const char *fname, const uint32_t *fsize) {
	obex_packet_t *packet;

	/* Create the packet and populate it with default parameters and headers. */
	packet = obex_packet_new(OBEX_OPCODE_CONNECT, true);
	obex_packet_param_add(packet, OBEX_PROTO_VERSION, 1);
	obex_packet_param_add(packet, 0x00, 1);
	obex_packet_param_add(packet, OBEX_MAX_PACKET_SIZE, 2);

	/* Add file name and size headers to the packet. */
	if (!obex_populate_name_length_headers(packet, fname, fsize)) {
		obex_packet_free(packet);
		return NULL;
	}

	/* Add the hostname header to the packet. */
	if (!obex_packet_header_add_hostname(packet)) {
		fprintf(stderr, "obex_packet_new_connect: Failed to append hostname "
				"header to the packet.\n");
		obex_packet_free(packet);

		return NULL;
	}

	return packet;
}

/**
 * Creates a brand new DISCONNECT packet.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @return Brand new DISCONNECT packet or NULL if we were unable to create it.
 */
obex_packet_t *obex_packet_new_disconnect(void) {
	obex_packet_t *packet;

	/* Create the packet and populate it with default parameters and headers. */
	packet = obex_packet_new(OBEX_OPCODE_DISCONNECT, true);

	return packet;
}

/**
 * Creates a brand new SUCCESS packet.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param final Set the final bit?
 * @param conn  Is this a response to a connection request?
 *
 * @return Brand new SUCCESS packet or NULL if we were unable to create it.
 */
obex_packet_t *obex_packet_new_success(bool final, bool conn) {
	obex_packet_t *packet;

	/* Create the packet and populate it with default parameters and headers. */
	packet = obex_packet_new(OBEX_RESPONSE_SUCCESS, final);
	if (conn) {
		obex_packet_param_add(packet, OBEX_PROTO_VERSION, 1);
		obex_packet_param_add(packet, 0x00, 1);
		obex_packet_param_add(packet, OBEX_MAX_PACKET_SIZE, 2);
	}

	return packet;
}

/**
 * Creates a brand new CONTINUE packet.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param final Set the final bit?
 *
 * @return Brand new CONTINUE packet or NULL if we were unable to create it.
 */
obex_packet_t *obex_packet_new_continue(bool final) {
	obex_packet_t *packet;

	/* Create the packet and populate it with default parameters and headers. */
	packet = obex_packet_new(OBEX_RESPONSE_CONTINUE, final);

	return packet;
}

/**
 * Creates a brand new UNAUTHORIZED packet.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param final Set the final bit?
 *
 * @return Brand new UNAUTHORIZED packet or NULL if we were unable to create it.
 */
obex_packet_t *obex_packet_new_unauthorized(bool final) {
	obex_packet_t *packet;

	/* Create the packet and populate it with default parameters and headers. */
	packet = obex_packet_new(OBEX_RESPONSE_UNAUTHORIZED, final);
	obex_packet_param_add(packet, OBEX_PROTO_VERSION, 1);
	obex_packet_param_add(packet, 0x00, 1);
	obex_packet_param_add(packet, OBEX_MAX_PACKET_SIZE, 2);

	return packet;
}

/**
 * Creates a brand new PUT packet with the headers fully populated, only missing
 * the body.
 *
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param fname Name of the file to be sent or NULL if one isn't needed.
 * @param fsize Pointer to the total length of the file in bytes or NULL if one
 *              isn't needed.
 * @param final Set the final bit?
 *
 * @return New populated PUT packet or NULL if we were unable to create it.
 */
obex_packet_t *obex_packet_new_put(const char *fname, const uint32_t *fsize, bool final) {
	obex_packet_t *packet;

	/* Create the packet and populate it with default parameters. */
	packet = obex_packet_new(OBEX_OPCODE_PUT, final);
	if (packet == NULL)
		return NULL;

	/* Add file name and size headers to the packet. */
	if (!obex_populate_name_length_headers(packet, fname, fsize)) {
		obex_packet_free(packet);
		return NULL;
	}

	/* Add the hostname header to the packet. */
	if (!obex_packet_header_add_hostname(packet)) {
		fprintf(stderr, "obex_packet_new_put: Failed to append hostname header "
				"to the packet.\n");
		obex_packet_free(packet);

		return NULL;
	}

	return packet;
}

/**
 * Creates a brand new GET packet with the headers fully populated.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param fname Name of the object to get.
 * @param final Set the final bit?
 *
 * @return New populated GET packet or NULL if we were unable to create it.
 */
obex_packet_t *obex_packet_new_get(const char *name, bool final) {
	obex_packet_t *packet;
	obex_header_t *header;
	wchar_t *wname;

	/* Create the packet and populate it with default parameters. */
	packet = obex_packet_new(OBEX_OPCODE_GET, final);
	if (packet == NULL)
		return NULL;

	/* Convert the name to UTF-16. */
	wname = utf16_mbstowcs(name);
	if (wname == NULL) {
		fprintf(stderr, "obex_packet_new_get: Failed to convert name '%s' to "
				"UTF-16.\n", name);
		obex_packet_free(packet);

		return NULL;
	}

	/* Create a new packet. */
	header = obex_header_new(OBEX_HEADER_NAME);
	if (header == NULL) {
		fprintf(stderr, "obex_packet_new_get: Failed to create the name packet "
				"header.\n");
		free(wname);
		obex_packet_free(packet);

		return NULL;
	}

	/* Set the name value. */
	obex_header_wstring_set(header, wname);

	/* Add the header to the packet. */
	if (!obex_packet_header_add(packet, header)) {
		fprintf(stderr, "obex_packet_new_get: Failed to append name header to "
				"the packet.\n");
		obex_header_free(header);
		obex_packet_free(packet);

		return NULL;
	}

	/* Add the hostname header to the packet. */
	if (!obex_packet_header_add_hostname(packet)) {
		fprintf(stderr, "obex_packet_new_get: Failed to append hostname header "
				"to the packet.\n");
		obex_packet_free(packet);

		return NULL;
	}

	return packet;
}

/**
 * Prints the contents of a packet in a human-readable, debug-friendly, way.
 * @warning This function will also recalculate and update the packet network size.
 *
 * @param packet OBEX packet to have its contents inspected and its size
 *               updated.
 *
 * @see obex_print_header
 */
void obex_print_packet(obex_packet_t *packet) {
	uint16_t i;

	/* Check if we have an invalid packet. */
	if (packet == obex_invalid_packet) {
		printf("INVALID OBEX PACKET\n");
		return;
	}

	/* Opcode or response code. */
	switch (packet->opcode) {
		case OBEX_OPCODE_CONNECT:
			printf("CONNECT");
			break;
		case OBEX_OPCODE_DISCONNECT:
			printf("DISCONNECT");
			break;
		case OBEX_OPCODE_PUT:
		case OBEX_SET_FINAL_BIT(OBEX_OPCODE_PUT):
			printf("PUT");
			break;
		case OBEX_OPCODE_GET:
		case OBEX_SET_FINAL_BIT(OBEX_OPCODE_GET):
			printf("GET");
			break;
		case OBEX_OPCODE_SETPATH:
			printf("SETPATH");
			break;
		case OBEX_OPCODE_ACTION:
		case OBEX_SET_FINAL_BIT(OBEX_OPCODE_ACTION):
			printf("ACTION");
			break;
		case OBEX_OPCODE_SESSION:
			printf("SESSION");
			break;
		case OBEX_OPCODE_ABORT:
			printf("ABORT");
			break;
		case OBEX_RESPONSE_CONTINUE:
		case OBEX_SET_FINAL_BIT(OBEX_RESPONSE_CONTINUE):
			printf("CONTINUE");
			break;
		case OBEX_RESPONSE_SUCCESS:
		case OBEX_SET_FINAL_BIT(OBEX_RESPONSE_SUCCESS):
			printf("SUCCESS");
			break;
		case OBEX_RESPONSE_UNAUTHORIZED:
		case OBEX_SET_FINAL_BIT(OBEX_RESPONSE_UNAUTHORIZED):
			printf("UNAUTHORIZED");
			break;
		default:
			printf("OTHER");
			break;
	}

	/* Indicate if the final bit is set. */
	if (OBEX_IS_FINAL_OPCODE(packet->opcode)) {
		printf("* ");
	} else {
		printf(" ");
	}

	/* Show opcode in hex and display the packet length and header count. */
	obex_packet_size_refresh(packet);
	printf("[0x%02X] %u bytes and %u headers\n", packet->opcode, packet->size,
		   packet->header_count);

	/* Print out any parameters that we might have. */
	if (packet->params != NULL) {
		for (i = 0; i < packet->params_count; i++) {
			obex_packet_param_t param = packet->params[i];

			switch (param.size) {
				case 1:
					printf("[%02X] ", param.value.byte);
					break;
				case 2:
					printf("[%04X] ", param.value.uint16);
					break;
				default:
					printf("[Invalid parameter size %u] ", param.size);
			}
		}

		printf("\n");
	}

	/* Print out the headers. */
	for (i = 0; i < packet->header_count; i++) {
		obex_print_header(packet->headers[i], i == 0);
	}

	/* Print out the data body. */
	if (packet->body != NULL) {
		const unsigned char *p = (const unsigned char *)packet->body;

		/* Print out each byte of the body. */
		printf("\n%s:\n", (packet->body_end) ? "End of Body" : "Body");
		for (i = 0; i < packet->body_length; i++) {
			printf("%02X'%c' ", p[i], p[i]);
		}

		printf("\n");
	}
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
			printf("(%u) L\"%ls\"", obex_header_size(header),
				   header->value.wstring.text);
			break;
		case OBEX_HEADER_ENCODING_STRING:
			printf("(%u) \"%s\"", obex_header_size(header),
				   header->value.string.text);
			break;
		case OBEX_HEADER_ENCODING_BYTE:
			printf("0x%02X", header->value.byte);
			break;
		case OBEX_HEADER_ENCODING_WORD32:
			printf("%u", header->value.word32);
			break;
		default:
			printf("Unknown header encoding: %u",
				   header->identifier.fields.encoding);
			break;
	}
}

/**
 * Populates the Name and Length headers of a packet.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param packet OBEX packet object to be populated.
 * @param name   Value of the Name header or NULL if it's not supposed to be
 *               populated.
 * @param len    Value of the Length header or NULL if it's not supposed to be
 *               populated.
 *
 * @return TRUE if the operation was successful.
 */
bool obex_populate_name_length_headers(obex_packet_t *packet, const char *name, const uint32_t *len) {
	obex_header_t *header;

	/* Add the name if required. */
	if (name != NULL) {
		wchar_t *wname;

		/* Convert the name to UTF-16. */
		wname = utf16_mbstowcs(name);
		if (wname == NULL) {
			fprintf(stderr, "obex_populate_name_length_headers: Failed to "
					"convert name '%s' to UTF-16.\n", name);

			return false;
		}

		/* Create a new packet. */
		header = obex_header_new(OBEX_HEADER_NAME);
		if (header == NULL) {
			fprintf(stderr, "obex_populate_name_length_headers: Failed to "
					"create the name packet header.\n");
			free(wname);

			return false;
		}

		/* Set the file name value. */
		obex_header_wstring_set(header, wname);

		/* Add the header to the packet. */
		if (!obex_packet_header_add(packet, header)) {
			fprintf(stderr, "obex_populate_name_length_headers: Failed to "
					"append name header to the packet.\n");
			obex_header_free(header);

			return false;
		}
	}

	/* Add the length if required. */
	if (len != NULL) {
		/* Create a new packet. */
		header = obex_header_new(OBEX_HEADER_LENGTH);
		if (header == NULL) {
			fprintf(stderr, "obex_populate_name_length_headers: Failed to "
					"create the length packet header.\n");
			return false;
		}

		/* Set the length value. */
		header->value.word32 = *len;

		/* Add the header to the packet. */
		if (!obex_packet_header_add(packet, header)) {
			fprintf(stderr, "obex_populate_name_length_headers: Failed to "
					"append length header to the packet.\n");
			obex_header_free(header);

			return false;
		}
	}

	return true;
}

/**
 * Performs a memcpy operation but returns a pointer to the next byte after the
 * copied data instead of just a pointer to dest.
 *
 * @param dest Destination buffer.
 * @param src  Source buffer.
 * @param len  Number of bytes to copy.
 *
 * @return Pointer to the next byte after the copied data in dest. Warning: This
 *         pointer may be after the end of the allocated buffer.
 *
 * @see memcpy
 */
void *memcpy_n(void *dest, const void *src, size_t len) {
	memcpy(dest, src, len);
	return ((char *)dest) + len;
}
