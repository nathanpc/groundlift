/**
 * utf16utils.c
 * UTF-16 string utilities for better interoperability with the OBEX spec and
 * between Windows and UTF-8 platforms.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "utf16utils.h"

/**
 * Fixes a UTF-16 that was caught up inside a 32-bit wchar_t and rearranges the
 * bytes in order to turn it into a valid UTF-32 native wide-character string.
 *
 * @note Under systems where wchar_t is 16-bit this function will be a NOP.
 *
 * @param str UTF-16 string to be converted into a proper UTF-32 string.
 */
void utf16_wchar32_fix(wchar_t *str) {
#if _SIZEOF_WCHAR == 4
	size_t len;

	/* Get the length of the string and ensure the NULL termination. */
	len = utf16_wchar32_len(str);
	str[len] = L'\0';

	/* Go through rearranging the characters. */
	for (len -= 1; len != 0; len--) {
		if ((len % 2) == 0) {
			/* Upper 16-bits. */
			str[len] = (str[len / 2] & 0xFFFF0000) >> 16;
		} else {
			/* Lower 16-bits. */
			str[len] = str[(len - 1) / 2] & 0xFFFF;
		}
	}

	/* Rearrange the first character of the string. */
	str[0] = (str[0] & 0xFFFF0000) >> 16;
#endif /* _SIZEOF_WCHAR == 4 */
}

/**
 * Gets the length of an UTF-16 string that's placed wrongly inside of a 32-bit
 * wchar_t.
 *
 * @note Under systems where wchar_t is 16-bit this function will be equivalent
 *       to calling wcslen.
 *
 * @param str UTF-16 string to have its characters counted.
 *
 * @return Number of UTF-16 characters inside the wchar_t.
 */
size_t utf16_wchar32_len(const wchar_t *str) {
#if _SIZEOF_WCHAR == 4
	const wchar_t *cur;
	size_t len;

	/* Start fresh. */
	len = 0;
	cur = str;

	/* Go through the string checking the lower and upper 16-bits of it. */
	while (((*cur & 0xFFFF0000) >> 16) != '\0') {
		/* Upper 16-bits isn't a NULL termination. */
		len++;

		/* Check the lower 16-bits for a NULL termination. */
		if ((*cur & 0xFFFF) == '\0')
			break;

		/* Count the lower 16-bits character and advance the cursor. */
		len++;
		cur++;
	}

	return len;
#else
	return wcslen(str);
#endif /* _SIZEOF_WCHAR == 4 */
}
