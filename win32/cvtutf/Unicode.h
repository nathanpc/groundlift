/**
 * Unicode.h
 * A wrapper for Unicode's CVTUTF string conversion library.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _SHIMS_CVTUTF_WRAPPER_H
#define _SHIMS_CVTUTF_WRAPPER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdbool.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

// Runtime assumptions check.
bool UnicodeAssumptionsCheck();

// Conversion functions.
bool UnicodeMultiByteToWideChar(const char *mbstr, wchar_t **wstr);
bool UnicodeWideCharToMultiByte(const wchar_t *wstr, char **mbstr);

#ifdef __cplusplus
}
#endif

#endif // _SHIMS_CVTUTF_WRAPPER_H
