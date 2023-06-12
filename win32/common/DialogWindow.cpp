/**
 * DialogWindow.cpp
 * Base class for a dialog window.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "DialogWindow.h"

#include <tchar.h>

#include "MsgBoxes.h"

/**
 * Initializes the dialog window object.
 *
 * @param hInst      Application's instance that this dialog belongs to.
 * @param hwndParent Parent window handle.
 * @param wResID     Dialog resource ID.
 */
DialogWindow::DialogWindow(HINSTANCE& hInst, HWND& hwndParent, WORD wResID) :
	hInst(hInst),
	hwndParent(hwndParent) {
	this->wResID = wResID;
	this->hDlg = NULL;
	this->bIsModal = false;
	this->bIsDisposable = false;
}

/**
 * Closes the dialog if needed and handles the object destruction.
 */
DialogWindow::~DialogWindow() {
	Close(-1, false);
}

/**
 * Displays a modeless dialog window.
 *
 * @return TRUE if the operation was successful.
 */
bool DialogWindow::Show() {
	HWND hWnd;

	// Create the dialog and pass our object to its initialization.
	hWnd = CreateDialogParam(this->hInst, MAKEINTRESOURCE(this->wResID),
		this->hwndParent, DlgProcWrapper, reinterpret_cast<LPARAM>(this));
	if (hWnd == NULL) {
		MsgBoxLastError(NULL);
		return false;
	}

	// Actually show the dialog window.
	this->bIsModal = false;
	ShowWindow(hWnd, SW_SHOWNORMAL); 
    UpdateWindow(hWnd);

	return true;
}

/**
 * Displays a modal dialog window.
 *
 * @return Return value of the dialog window procedure.
 */
INT_PTR DialogWindow::ShowModal() {
	this->bIsModal = true;
	return DialogBoxParam(this->hInst, MAKEINTRESOURCE(this->wResID),
		this->hwndParent, DialogWindow::DlgProcWrapper,
		reinterpret_cast<LPARAM>(this));
}

/**
 * Closes the dialog window.
 *
 * @param nResult Return value of the dialog.
 */
void DialogWindow::Close(INT_PTR nResult) {
	Close(nResult, true);
}

/**
 * Closes the dialog window.
 *
 * @param nResult      Return value of the dialog.
 * @param bSelfDispose Are we allowed to self dispose if asked to?
 */
void DialogWindow::Close(INT_PTR nResult, bool bSelfDispose) {
	// Ensure we have a dialog currently opened.
	if (this->hDlg == NULL)
		return;

	// Close the dialog.
	if (this->bIsModal) {
		EndDialog(this->hDlg, nResult);
	} else {
		DestroyWindow(this->hDlg);
	}

	// Invalidate our dialog window handle.
	this->hDlg = NULL;

	// Self destruct automagically.
	if (bSelfDispose && this->bIsDisposable)
		delete this;
}

/**
 * Enables this object to self destruct automatically after the window has been
 * closed.
 */
void DialogWindow::EnableSelfDisposal() {
	this->bIsDisposable = true;
}

/**
 * Registers the dialog's window handle internally to the class.
 *
 * @warning This function MUST always be called when a WM_INITDIALOG message is
 *          received.
 *
 * @param hDlg Dialog window handle.
 */
void DialogWindow::RegisterHandle(HWND hDlg) {
	this->hDlg = hDlg;
}

/**
 * Dialog window procedure.
 *
 * @param hDlg   Dialog window handle.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return TRUE if the message was handled by the function, FALSE otherwise.
 *
 * @see DefaultDlgProc
 */
INT_PTR CALLBACK DialogWindow::DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
									   LPARAM lParam) {
	return DefaultDlgProc(hDlg, wMsg, wParam, lParam);
}

/**
 * Default dialog window procedure that should be called whenever a message
 * isn't handled by a child's class dialog window procedure.
 *
 * @param hDlg   Dialog window handle.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return TRUE if the message was handled by the function, FALSE otherwise.
 *
 * @see DlgProc
 */
INT_PTR CALLBACK DialogWindow::DefaultDlgProc(HWND hDlg, UINT wMsg,
											  WPARAM wParam, LPARAM lParam) {
	// Handle messages.
	switch (wMsg) {
		case WM_INITDIALOG:
			break;
		case WM_COMMAND:
			if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL)) {
				Close(LOWORD(wParam));
				return TRUE;
			}
			break;
	}

    return FALSE;
}

/**
 * Dialog window procedure wrapper for C++.
 *
 * @param hDlg   Dialog window handle.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter. (Pointer to object during creation)
 *
 * @return TRUE if the message was handled by the function, FALSE otherwise.
 *
 * @see RegisterHandle
 * @see DefaultDlgProc
 */
INT_PTR CALLBACK DialogWindow::DlgProcWrapper(HWND hDlg, UINT wMsg,
											  WPARAM wParam, LPARAM lParam) {
	DialogWindow *pThis = NULL;

	// Handle the translation to a C++ call.
	if (WM_INITDIALOG == wMsg) {
		pThis = reinterpret_cast<DialogWindow *>(lParam);
		SetWindowLongPtr(hDlg, GWLP_USERDATA,
						 reinterpret_cast<LONG_PTR>(pThis));
		pThis->RegisterHandle(hDlg);
	} else {
		pThis = reinterpret_cast<DialogWindow *>(
			GetWindowLongPtr(hDlg, GWLP_USERDATA));
	}

	// Call our object's dialog procedure.
	if (pThis)
		return pThis->DlgProc(hDlg, wMsg, wParam, lParam);

	return FALSE;
}
