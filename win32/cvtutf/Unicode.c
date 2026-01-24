/**
 * Unicode.c
 * A wrapper for Unicode's CVTUTF string conversion library.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "Unicode.h"

#include <stdlib.h>
#include <string.h>

#include "ConvertUTF.h"

/**
 * Margin used to allocate a buffer that can hold the conversion result, defined
 * as: Allocation Margin = Original Buffer + Margin.
 */
#define UTF16_ALLOC_MARGIN 1.5f
#define UTF8_ALLOC_MARGIN  3

/**
 * Checks if the wchar_t is the same size as UTF-16 (2 bytes) and if char is the
 * same as UTF-8 (1 byte).
 * 
 * @warning This check is REQUIRED to be done by your application once at
 *          startup. If it fails you have to abort since all other operations
 *          assume that wchar_t is 2 bytes long and char is 1 byte long.
 * 
 * @return TRUE if wchar_t and char are properly defined. FALSE otherwise.
 */
bool UnicodeAssumptionsCheck() {
	return (sizeof(wchar_t) == sizeof(UTF16)) && (sizeof(char) == sizeof(UTF8));
}

/**
 * Converts a multi-byte string (UTF-8) to a wide-character string (UTF-16).
 * 
 * @warning This function allocates memory dynamically that must be free'd.
 * 
 * @param mbstr UTF-8 string to be converted.
 * @param wstr  Pointer to a newly allocated UTF-16 string.
 * 
 * @return TRUE if the conversion was successful, FALSE otherwise.
 */
bool UnicodeMultiByteToWideChar(const char *mbstr, wchar_t **wstr) {
	const char* szInputStart = mbstr;
	const char* szInputEnd = NULL;
	wchar_t* szOutputStart = NULL;
	wchar_t* szOutputEnd = NULL;

	// Measure the input buffer and get the location of the NUL terminator.
	size_t len = strlen(szInputStart);
	szInputEnd = szInputStart + len;
	len = (size_t)(len * UTF16_ALLOC_MARGIN);

	// Allocate the new buffer and get its end target (NUL terminator).
	szOutputStart = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
	if (szOutputStart == NULL)
		return false;
	szOutputEnd = szOutputStart + len;

	// Perform the conversion.
	const UTF8* szInput = (const UTF8*)szInputStart;
	UTF16* szOutput = (UTF16*)szOutputStart;
	ConversionResult res = ConvertUTF8toUTF16(&szInput, (const UTF8*)szInputEnd,
		&szOutput, (UTF16*)szOutputEnd, lenientConversion);
	if (res != conversionOK) {
		free(szOutputStart);
		*wstr = NULL;
		return false;
	}
	*szOutput = (UTF16)L'\0';

	// Trim output buffer size if needed.
	size_t lenOutput = szOutput - (UTF16*)szOutputStart + 1;
	if (lenOutput < len) {
		szOutputStart = (wchar_t*)realloc(szOutputStart, lenOutput *
			sizeof(wchar_t));
	}

	// Set the output pointer and return.
	*wstr = szOutputStart;
	return true;
}

/**
 * Converts a wide-character string (UTF-16) to a multi-byte string (UTF-8).
 *
 * @warning This function allocates memory dynamically that must be free'd.
 *
 * @param wstr  UTF-16 string to be converted.
 * @param mbstr Pointer to a newly allocated UTF-8 string.
 *
 * @return TRUE if the conversion was successful, FALSE otherwise.
 */
bool UnicodeWideCharToMultiByte(const wchar_t *wstr, char **mbstr) {
	const wchar_t* szInputStart = wstr;
	const wchar_t* szInputEnd = NULL;
	char* szOutputStart = NULL;
	char* szOutputEnd = NULL;

	// Measure the input buffer and get the location of the NUL terminator.
	size_t len = wcslen(szInputStart);
	szInputEnd = szInputStart + len;
	len = (size_t)(len * UTF8_ALLOC_MARGIN);

	// Allocate the new buffer and get its end target (NUL terminator).
	szOutputStart = (char*)malloc((len + 1) * sizeof(char));
	if (szOutputStart == NULL)
		return false;
	szOutputEnd = szOutputStart + len;

	// Perform the conversion.
	const UTF16* szInput = (const UTF16*)szInputStart;
	UTF8* szOutput = (UTF8*)szOutputStart;
	ConversionResult res = ConvertUTF16toUTF8(&szInput,
		(const UTF16*)szInputEnd, &szOutput, (UTF8*)szOutputEnd,
		lenientConversion);
	if (res != conversionOK) {
		free(szOutputStart);
		*mbstr = NULL;
		return false;
	}
	*szOutput = (UTF8)'\0';

	// Trim output buffer size if needed.
	size_t lenOutput = szOutput - (UTF8*)szOutputStart + 1;
	if (lenOutput < len)
		szOutputStart = (char*)realloc(szOutputStart, lenOutput * sizeof(char));

	// Set the output pointer and return.
	*mbstr = szOutputStart;
	return true;
}
