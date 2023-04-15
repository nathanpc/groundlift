/**
 * utf16utils.h
 * UTF-16 string utilities for better interoperability with the OBEX spec and
 * between Windows and UTF-8 platforms.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_UTF16UTILS_H
#define _GL_UTF16UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UTF-16 mangled inside a 32-bit wchar_t. */
void utf16_wchar32_fix(wchar_t *str);
size_t utf16_wchar32_len(const wchar_t *str);

/* Conversions */
uint16_t utf16_conv_ltos(wchar_t wc);
wchar_t *utf16_mbstowcs(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* _GL_UTF16UTILS_H */
