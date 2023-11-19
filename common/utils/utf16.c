/**
 * utf16.c
 * UTF-16 string utilities for better interoperability with the OBEX spec and
 * between Windows and UTF-8 platforms.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "utf16.h"

#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <tchar.h>

#include "capabilities.h"
#endif /* _WIN32 */

#ifdef _WIN32
	/* Define the UTF-8 codepage on older systems. */
	#ifndef CP_UTF8
		#define CP_UTF8 65001
	#endif /* !CP_UTF8 */
#endif /* _WIN32 */

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
#else
	(void)str;
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

/**
 * Converts a 32-bit wchar_t into a uint16_t on platforms that need the
 * conversion.
 *
 * @param wc Wide character to be converted.
 *
 * @return Guaranteed UTF-16 character.
 */
uint16_t utf16_conv_ltos(wchar_t wc) {
#if _SIZEOF_WCHAR == 2
	return (uint16_t)wc;
#else
	return (uint16_t)(wc & 0xFFFF);
#endif /* _SIZEOF_WCHAR == 2 */
}

/**
 * Converts a UTF-8 multibyte string into an UTF-16 wide-character string.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param str UTF-8 string to be converted.
 *
 * @return UTF-16 wide-character converted string or NULL if an error occurred.
 */
wchar_t *utf16_mbstowcs(const char *str) {
	wchar_t *wstr;

#ifdef _WIN32
	int nLen;

	/* Get required buffer size and allocate some memory for it. */
	wstr = NULL;
	nLen = MultiByteToWideChar(cap_utf8() ? CP_UTF8 : CP_OEMCP, 0, str, -1,
							   NULL, 0);
	if (nLen == 0)
		goto failure;
	wstr = (wchar_t *)malloc(nLen * sizeof(wchar_t));
	if (wstr == NULL)
		return NULL;

	/* Perform the conversion. */
	nLen = MultiByteToWideChar(cap_utf8() ? CP_UTF8 : CP_OEMCP, 0, str, -1,
							   wstr, nLen);
	if (nLen == 0) {
failure:
		MessageBox(NULL, _T("Failed to convert UTF-8 string to UTF-16."),
			_T("String Conversion Failure"), MB_ICONERROR | MB_OK);
		if (wstr)
			free(wstr);

		return NULL;
	}
#else
	size_t len;

	/* Allocate some memory for our converted string. */
	len = mbstowcs(NULL, str, 0) + 1;
	wstr = (wchar_t *)malloc(len * sizeof(wchar_t));
	if (wstr == NULL)
		return NULL;

	/* Perform the string conversion. */
	len = mbstowcs(wstr, str, len);
	if (len == (size_t)-1) {
		free(wstr);
		return NULL;
	}
#endif /* _WIN32 */

	return wstr;
}

/**
 * Converts a UTF-16 wide-character string into a UTF-8 multibyte string.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param wstr UTF-16 string to be converted.
 *
 * @return UTF-8 multibyte converted string or NULL if an error occurred.
 */
char *utf16_wcstombs(const wchar_t *wstr) {
	char *str;

#ifdef _WIN32
	int nLen;

	/* Get required buffer size and allocate some memory for it. */
	nLen = WideCharToMultiByte(cap_utf8() ? CP_UTF8 : CP_OEMCP, 0, wstr, -1,
							   NULL, 0, NULL, NULL);
	if (nLen == 0)
		goto failure;
	str = (char *)malloc(nLen * sizeof(char));
	if (str == NULL)
		return NULL;

	/* Perform the conversion. */
	nLen = WideCharToMultiByte(cap_utf8() ? CP_UTF8 : CP_OEMCP, 0, wstr, -1,
							   str, nLen, NULL, NULL);
	if (nLen == 0) {
failure:
		MessageBox(NULL, _T("Failed to convert UTF-16 string to UTF-8."),
			_T("String Conversion Failure"), MB_ICONERROR | MB_OK);

		return NULL;
	}
#else
	size_t len;

	/* Allocate some memory for our converted string. */
	len = wcstombs(NULL, wstr, 0) + 1;
	str = (char *)malloc(len * sizeof(char));
	if (str == NULL)
		return NULL;

	/* Perform the string conversion. */
	len = wcstombs(str, wstr, len);
	if (len == (size_t)-1) {
		free(str);
		return NULL;
	}
#endif /* _WIN32 */

	return str;
}
