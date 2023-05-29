/**
 * WindowUtilities.c
 * Some utility functions to help us deal with Windows's windows and controls.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "WindowUtilities.h"

#include <stdlib.h>

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
