/**
 * WindowUtilities.c
 * Some utility functions to help us deal with Windows's windows and controls.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "WindowUtilities.h"

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

/**
 * Gets a window's text property into a newly allocated buffer. No need for
 * pointer wrangling.
 *
 * @warning This function allocates memory that must be free'd by you.
 * 
 * @param hWnd Handle of the window we want the text from.
 * 
 * @return Newly allocated buffer with the window's text.
 */
LPTSTR GetWindowTextAlloc(HWND hWnd) {
	int iLen;
	LPTSTR szText;

	/* Get the length of the text. */
	iLen = GetWindowTextLength(hWnd) + 1;

	/* Allocate the memory to receive the window's text. */
	szText = (LPTSTR)malloc(iLen * sizeof(TCHAR));
	if (szText == NULL)
		return NULL;

	/* Get the text from the window. */
	GetWindowText(hWnd, szText, iLen);

	return szText;
}

/**
 * Sets the window's text in a printf-like fashion.
 * 
 * @param hWnd     Handle of the window to have its text set.
 * @param szFormat Format of the string to be used in the window.
 * @param ...      Parameters to be inserted into the string. (printf style)
 * 
 * @return The return value of SetWindowText or FALSE if an error occurred.
 */
BOOL SetWindowFormatText(HWND hWnd, LPCTSTR szFormat, ...) {
	va_list args;
	size_t nLen;
	LPTSTR szBuffer;
	BOOL bRet;

	va_start(args, szFormat);

	/* Get the number of characters needed for the buffer. */
	nLen = _vsntprintf(NULL, 0, szFormat, args);
	nLen++;

	/* Allocate the buffer to hold the string. */
	szBuffer = (LPTSTR)malloc(nLen * sizeof(TCHAR));
	if (szBuffer == NULL)
		return FALSE;

	/* Populate the buffer. */
	_vsntprintf(szBuffer, nLen, szFormat, args);

	va_end(args);

	/* Set the window's text. */
	bRet = SetWindowText(hWnd, szBuffer);

	/* Free our temporary buffer. */
	free(szBuffer);
}
