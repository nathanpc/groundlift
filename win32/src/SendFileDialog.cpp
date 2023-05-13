/**
 * SendFileDialog.cpp
 * A dialog to send files to a specific client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "SendFileDialog.h"

#include "CommonIncludes.h"

/**
 * Initializes the dialog window object.
 *
 * @param hInst      Application's instance that this dialog belongs to.
 * @param hwndParent Parent window handle.
 */
SendFileDialog::SendFileDialog(HINSTANCE& hInst, HWND& hwndParent) :
	DialogWindow(hInst, hwndParent, IDD_SEND) {
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
INT_PTR CALLBACK SendFileDialog::DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
										 LPARAM lParam) {
	// Handle messages.
	switch (wMsg) {
		case WM_INITDIALOG:
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) {
				MsgBox(hDlg, MB_OK, L"Send File", L"Send the file!");
			} else if (LOWORD(wParam) == IDCANCEL) {
				MsgBox(hDlg, MB_OK, L"Send File", L"Cancel everything!");
			}
			break;
	}

	// Pass the message to the default message handler.
	return DefaultDlgProc(hDlg, wMsg, wParam, lParam);
}
