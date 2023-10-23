/**
 * protocol.h
 * GroundLift's glproto implementation and helper functions.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_PROTOCOL_H
#define _GL_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Valid message types.
 */
typedef enum {
	GLPROTO_TYPE_DISCOVERY = 'D',
	GLPROTO_TYPE_URL = 'U',
	GLPROTO_TYPE_FILE = 'F'
} glproto_type_t;

#ifdef __cplusplus
}
#endif

#endif /* _GL_PROTOCOL_H */
