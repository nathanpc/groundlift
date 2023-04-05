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
 * OBEX header encoding. (Upper 2 bits of the header identifier shifted)
 */
typedef enum {
	OBEX_HEADER_ENCODING_UTF16  = 0b00,
	OBEX_HEADER_ENCODING_STRING = 0b01,
	OBEX_HEADER_ENCODING_BYTE   = 0b10,
	OBEX_HEADER_ENCODING_WORD64 = 0b11
} obex_header_encoding_t;

/**
 * Standard OBEX header identifiers. (Includes their standard encoding bits)
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
 * OBEX header representation.
 */
typedef struct {
	union {
		uint8_t id;

		struct {
			unsigned int meaning : 6;
			unsigned int encoding : 2;
		};
	} identifier;

	union {
		void *data;
		uint8_t byte;
		uint64_t word64;

		struct {
			uint16_t fhlength;
			char *text;
		} string;

		struct {
			uint16_t fhlength;
			uint16_t *text;
		} wstring;
	} value;
} obex_header_t;

/* Debug */
void obex_print_header(const obex_header_t *header);
void obex_print_header_encoding(const obex_header_t *header);
void obex_print_header_value(const obex_header_t *header);

#ifdef __cplusplus
}
#endif

#endif /* _GL_OBEX_H */
