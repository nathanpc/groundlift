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

/* Check if the all important wchar_t size macro is defined. */
#ifndef _SIZEOF_WCHAR
	#ifdef _WIN32
		#define _SIZEOF_WCHAR 2
	#elif defined(__SIZEOF_WCHAR_T__)
		#define _SIZEOF_WCHAR __SIZEOF_WCHAR_T__
	#else
		#error _SIZEOF_WCHAR wasn't defined. Use the script under \
		scripts/wchar-size.sh to determine its size and define the macro in your \
		build system.
	#endif
#endif /* _SIZEOF_WCHAR */

/* UTF-16 mangled inside a 32-bit wchar_t. */
void utf16_wchar32_fix(wchar_t *str);
size_t utf16_wchar32_len(const wchar_t *str);

#ifdef __cplusplus
}
#endif

#endif /* _GL_UTF16UTILS_H */
