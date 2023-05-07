/**
 * bits.h
 * Some utility and helper macros to shuffle around in bit land.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_UTILS_BITS_H
#define _GL_UTILS_BITS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Converts 2 bytes into a single 16-bit value.
 *
 * @param msb Most significant byte.
 * @param lsb Least significant byte.
 *
 * @return 16-bit value.
 */
#define BYTES_TO_USHORT(msb, lsb) \
	(uint16_t)(((msb) << 8) | (lsb))

/**
 * Converts 2 bytes into a single UTF-16 wide character value.
 *
 * @param msb Most significant byte.
 * @param lsb Least significant byte.
 *
 * @return UTF-16 wide character value.
 */
#define BYTES_TO_WCHAR(msb, lsb) \
	(wchar_t)(((msb) << 8) | (lsb))

/**
 * Converts 4 bytes into a single 32-bit value.
 *
 * @param msb1 Most significant byte.
 * @param msb2 More Significant byte.
 * @param lsb1 Less significant byte.
 * @param lsb2 Least significant byte.
 *
 * @return 32-bit value.
 */
#define BYTES_TO_U32(msb1, msb2, lsb1, lsb2) \
	(uint32_t)(((msb1) << 24) | ((msb2) << 16) | ((lsb1) << 8) | (lsb2))

/**
 * Converts a pointer to a sequence of unsigned bytes in network byte order into
 * a single 16-bit value.
 *
 * @param buf Pointer to a sequence of unsigned bytes. (Pointer will not be
 *            advanced by this function)
 *
 * @return 16-bit value.
 */
#define U8BUF_TO_USHORT(buf) \
	BYTES_TO_USHORT((*buf), *((buf) + 1))

/**
 * Converts a pointer to a sequence of unsigned bytes in network byte order into
 * a single UTF-16 wide character value.
 *
 * @param buf Pointer to a sequence of unsigned bytes. (Pointer will not be
 *            advanced by this function)
 *
 * @return UTF-16 wide character value.
 */
#define U8BUF_TO_WCHAR(buf) \
	BYTES_TO_WCHAR((*buf), *((buf) + 1))

/**
 * Converts a pointer to a sequence of unsigned bytes in network byte order into
 * a single 32-bit value.
 *
 * @param buf Pointer to a sequence of unsigned bytes. (Pointer will not be
 *            advanced by this function)
 *
 * @return 32-bit value.
 */
#define U8BUF_TO_U32(buf) \
	BYTES_TO_U32((*buf), *((buf) + 1), *((buf) + 2), *((buf) + 3))

#ifdef __cplusplus
}
#endif

#endif /* _GL_UTILS_BITS_H */
