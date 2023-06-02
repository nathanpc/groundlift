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
	BOOL bRet;
#if (_MSC_VER > 1200)
	LPTSTR szBuffer;
#else
	TCHAR szBuffer[1024];

	/* Ensure we have a maximum length set. */
	nLen = 1023;
#endif /* _MSC_VER > 1200 */

	va_start(args, szFormat);

#if (_MSC_VER > 1200)
	/* Get the number of characters needed for the buffer. */
	nLen = _vsntprintf(NULL, 0, szFormat, args);

	/* Allocate the buffer to hold the string. */
	szBuffer = (LPTSTR)malloc((nLen + 1) * sizeof(TCHAR));
	if (szBuffer == NULL)
		return FALSE;
#endif /* _MSC_VER > 1200 */

	/* Populate the buffer and ensure its termination. */
	_vsntprintf(szBuffer, nLen, szFormat, args);
	szBuffer[nLen] = _T('\0');

	va_end(args);

	/* Set the window's text. */
	bRet = SetWindowText(hWnd, szBuffer);

#if (_MSC_VER > 1200)
	/* Free our temporary buffer. */
	free(szBuffer);
#endif /* _MSC_VER > 1200 */

	return bRet;
}

/**
 * Sets the default button in a dialog box and ensures any previous default
 * button looses its default button style.
 * 
 * @param hDlg Dialog handle.
 * @param nID  ID of the new default button in the dialog.
 */
void SetDlgDefaultButton(HWND hDlg, int nID) {
	LRESULT lDefButton;

	// Get the current default button.
	lDefButton = SendMessage(hDlg, DM_GETDEFID, 0, 0);

	// Ensure that the last default button looses its style.
	if (HIWORD(lDefButton) == DC_HASDEFID)
		SendMessage(hDlg, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));

	// Set the new default button.
	SendMessage(hDlg, DM_SETDEFID, nID, 0);
}
