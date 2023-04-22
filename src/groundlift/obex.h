/**
 * obex.h
 * GroundLift OBEX implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_OBEX_H
#define _GL_OBEX_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>

#include "error.h"
#include "tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Converts a header encoding flag into a mask.
 *
 * @param flag Header encoding bits to be shifted into a mask.
 */
#define OBEX_HEADER_ENCODING_MASK(flag) ((flag) << 6)

/**
 * Sets the Final Bit Flag of an opcode to indicate the end of an operation.
 *
 * @param opcode Opcode to have its final bit set.
 */
#define OBEX_SET_FINAL_BIT(opcode) ((opcode) | (1 << 7))

/**
 * Checks wether the opcode has the Final Bit Flag set.
 *
 * @param opcode Opcode to have its final bit inspected.
 */
#define OBEX_IS_FINAL_OPCODE(opcode) ((opcode) & (1 << 7))

/**
 * Ignores the Final Bit Flag of an opcode.
 *
 * @param opcode Opcode to have its final bit removed.
 */
#define OBEX_IGNORE_FINAL_BIT(opcode) ((opcode) & 0x7F)

/**
 * OBEX header encoding. (Upper 2 bits of the header identifier shifted)
 */
typedef enum {
	OBEX_HEADER_ENCODING_UTF16  = 0,
	OBEX_HEADER_ENCODING_STRING = 1,
	OBEX_HEADER_ENCODING_BYTE   = 2,
	OBEX_HEADER_ENCODING_WORD32 = 3
} obex_header_encoding_t;

/**
 * Standard OBEX header identifiers including their standard encoding bits.
 */
typedef enum {
	OBEX_HEADER_COUNT = 0xC0,
	OBEX_HEADER_NAME = 0x01,
	OBEX_HEADER_TYPE = 0x42,
	OBEX_HEADER_LENGTH = 0xC3,
	OBEX_HEADER_TIME = 0x44,
	OBEX_HEADER_TIME_OBSOLETE = 0xC4,
	OBEX_HEADER_DESCRIPTION = 0x05,
	OBEX_HEADER_TARGET = 0x46,
	OBEX_HEADER_HTTP = 0x47,
	OBEX_HEADER_BODY = 0x48,
	OBEX_HEADER_END_BODY = 0x49,
	OBEX_HEADER_WHO = 0x4A,
	OBEX_HEADER_CONN_ID = 0xCB,
	OBEX_HEADER_APP_PARAMS = 0x4C,
	OBEX_HEADER_AUTH_CHALLENGE = 0x4D,
	OBEX_HEADER_AUTH_RESPONSE = 0x4E,
	OBEX_HEADER_CREATOR_ID = 0xCF,
	OBEX_HEADER_WAN_UUID = 0x50,
	OBEX_HEADER_OBJ_CLASS = 0x51,
	OBEX_HEADER_SESSION_PARAMS = 0x52,
	OBEX_HEADER_SESSION_SEQ_NUM = 0x93,
	OBEX_HEADER_ACTION_ID = 0x94,
	OBEX_HEADER_DEST_NAME = 0x15,
	OBEX_HEADER_PERMISSIONS = 0xD6,
	OBEX_HEADER_SRM = 0x97,
	OBEX_HEADER_SRM_PARAMS = 0x98
} obex_header_id_t;

/**
 * Standard OBEX opcodes with the final bit already set when applicable.
 */
typedef enum {
	/* Opcodes */
	OBEX_OPCODE_CONNECT = 0x80,
	OBEX_OPCODE_DISCONNECT = 0x81,
	OBEX_OPCODE_PUT = 0x02,
	OBEX_OPCODE_GET = 0x03,
	OBEX_OPCODE_RESERVED = 0x04,
	OBEX_OPCODE_SETPATH = 0x85,
	OBEX_OPCODE_ACTION = 0x06,
	OBEX_OPCODE_SESSION = 0x87,
	OBEX_OPCODE_ABORT = 0xFF,

	/* Response Codes */
	OBEX_RESPONSE_CONTINUE = 0x10,
	OBEX_RESPONSE_SUCCESS = 0x20,
	OBEX_RESPONSE_BAD_REQUEST = 0x40,
	OBEX_RESPONSE_UNAUTHORIZED = 0x41,
	OBEX_RESPONSE_FORBIDDEN = 0x43,
	OBEX_RESPONSE_METHOD_NOT_ALLOWED = 0x45,
	OBEX_RESPONSE_CONFLICT = 0x49,
	OBEX_RESPONSE_INTERNAL_ERROR = 0x50,
	OBEX_RESPONSE_NOT_IMPLEMENTED = 0x51,
	OBEX_RESPONSE_SERVICE_UNAVAILABLE = 0x53
} obex_opcodes_t;

/**
 * OBEX header abstraction.
 */
typedef struct {
	union {
		uint8_t id;

		struct {
			unsigned int meaning : 6;
			unsigned int encoding : 2;
		} fields;
	} identifier;

	union {
		uint8_t byte;
		uint32_t word32;

		struct {
			uint16_t length; /* Number of characters in the string. */
			char *text;
		} string;

		struct {
			uint16_t length; /* Number of characters in the string. */
			wchar_t *text;
		} wstring;
	} value;
} obex_header_t;

/**
 * OBEX packet parameter abstraction.
 */
typedef struct {
	uint8_t size; /* Size in bytes of the parameter. */
	union {
		uint8_t byte;
		uint16_t uint16;
	} value;
} obex_packet_param_t;

/**
 * OBEX packet abstraction.
 */
typedef struct {
	uint8_t opcode;
	uint16_t size; /* Size in bytes of the entire packet in transport format. */

	uint8_t params_count;
	obex_packet_param_t *params;

	obex_header_t **headers;
	uint16_t header_count;

	bool body_end;
	uint16_t body_length;
	void *body;
} obex_packet_t;

/* Packet manipulation. */
obex_packet_t *obex_packet_new(obex_opcodes_t opcode, bool set_final);
void obex_packet_free(obex_packet_t *packet);
bool obex_packet_param_add(obex_packet_t *packet, uint16_t value, uint8_t size);
bool obex_packet_header_add(obex_packet_t *packet, obex_header_t *header);
bool obex_packet_header_pop(obex_packet_t *packet);
void obex_packet_body_set(obex_packet_t *packet, uint16_t size, void *body, bool eob);
bool obex_packet_body_copy(obex_packet_t *packet, uint16_t size, const void *src, bool eob);
uint16_t obex_packet_size_refresh(obex_packet_t *packet);
obex_header_t *obex_packet_header_find(const obex_packet_t *packet, obex_header_id_t hid);

/* Header manipulation. */
obex_header_t *obex_header_new(obex_header_id_t id);
void obex_header_free(obex_header_t *header);
bool obex_header_string_set(obex_header_t *header, char *str);
bool obex_header_string_copy(obex_header_t *header, const char *str);
bool obex_header_wstring_set(obex_header_t *header, wchar_t *wstr);
bool obex_header_wstring_copy(obex_header_t *header, const wchar_t *wstr);
uint16_t obex_header_size(const obex_header_t *header);

/* Packet encoding and decoding. */
bool obex_packet_encode(obex_packet_t *packet, void **buf);
void *obex_packet_encode_header_memcpy(const obex_header_t *header, void *buf);
obex_packet_t *obex_packet_decode(const void *buf, uint16_t len, bool has_params);

/* Networking */
gl_err_t *obex_net_packet_send(int sockfd, obex_packet_t *packet);
obex_packet_t *obex_net_packet_recv(int sockfd, bool has_params);

/* Common packet constructors. */
obex_packet_t *obex_packet_new_connect(const char *fname, const uint32_t *fsize);
obex_packet_t *obex_packet_new_disconnect(void);
obex_packet_t *obex_packet_new_success(bool final, bool conn);
obex_packet_t *obex_packet_new_continue(bool final);
obex_packet_t *obex_packet_new_unauthorized(bool final);
obex_packet_t *obex_packet_new_put(const char *fname, const uint32_t *fsize, bool final);

/* Debugging */
void obex_print_header(const obex_header_t *header, bool th);
void obex_print_header_encoding(const obex_header_t *header);
void obex_print_header_value(const obex_header_t *header);
void obex_print_packet(obex_packet_t *packet);

#ifdef __cplusplus
}
#endif

#endif /* _GL_OBEX_H */
