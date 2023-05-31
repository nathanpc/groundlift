/**
 * ProgressDialog.h
 * A generic dialog base class that monitors the progress of an operation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "ProgressDialog.h"

#include <shlwapi.h>
#include <utils/utf16.h>

#include "CommonIncludes.h"

/**
 * Initializes the dialog window object.
 *
 * @param hInst      Application's instance that this dialog belongs to.
 * @param hwndParent Parent window handle.
 * @param wResID     Dialog resource ID.
 */
ProgressDialog::ProgressDialog(HINSTANCE& hInst, HWND& hwndParent,
							   WORD wResID) :
	DialogWindow(hInst, hwndParent, wResID) {
	this->hwndContextLabel = NULL;
	this->hwndProgressBar = NULL;
	this->hwndRateLabel = NULL;
	this->hwndProgressLabel = NULL;
	this->hwndOpenFileButton = NULL;
	this->hwndOpenFolderButton = NULL;
	this->hwndCancelButton = NULL;

	this->isButtonClose = false;
}

/**
 * Frees up any resources allocated by this object.
 */
ProgressDialog::~ProgressDialog() {
}

/**
 * Switches the cancel button into a close button.
 * 
 * @param bMakeDefault Make the close button the default in the dialog.
 */
void ProgressDialog::SwitchCancelButtonToClose(bool bMakeDefault) {
	// Make the default button of the dialog.
	if (bMakeDefault)
		SetDlgDefaultButton(this->hDlg, IDCANCEL);

	// Disable the other buttons.
	EnableWindow(this->hwndOpenFileButton, false);
	EnableWindow(this->hwndOpenFolderButton, false);

	// Change the text of the button.
	SetWindowText(this->hwndCancelButton, _T("Close"));
	this->isButtonClose = true;
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
INT_PTR CALLBACK ProgressDialog::DlgProc(HWND hDlg, UINT wMsg,
										 WPARAM wParam, LPARAM lParam) {
	// Handle messages.
	switch (wMsg) {
		case WM_INITDIALOG:
			// Setup the controls for the operation.
			SetupControls(hDlg);
			break;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
			if (cap_win_least_11()) {
				// Ensure we use the default background color of a window.
				return reinterpret_cast<INT_PTR>(
					GetSysColorBrush(COLOR_WINDOW));
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDCANCEL: {
					// Call the cancel button event handler if needed.
					if (!isButtonClose) {
						INT_PTR iRet = OnCancel(hDlg);
						if (iRet)
							return iRet;
					}
					break;
				}
			}
			break;
	}

	// Pass the message to the default message handler.
	return DefaultDlgProc(hDlg, wMsg, wParam, lParam);
}

/**
 * Sets up the controls for the current operation.
 * 
 * @param hDlg Dialog window handle.
 */
void ProgressDialog::SetupControls(HWND hDlg) {
	// Get the handle of every useful control in the window.
	this->hwndContextLabel = GetDlgItem(hDlg,
										IDC_STATIC_TRANSFER_CONTEXT);
	this->hwndProgressBar = GetDlgItem(hDlg, IDC_PROGRESS);
	this->hwndRateLabel = GetDlgItem(hDlg, IDC_STATIC_RATE);
	this->hwndProgressLabel = GetDlgItem(hDlg, IDC_STATIC_PROGRESS);
	this->hwndOpenFileButton = GetDlgItem(hDlg, IDC_OPEN_FILE);
	this->hwndOpenFolderButton = GetDlgItem(hDlg, IDC_OPEN_FOLDER);
	this->hwndCancelButton = GetDlgItem(hDlg, IDCANCEL);

	// Set some default texts for the labels.
	SetWindowText(this->hwndRateLabel, _T(""));
	SetWindowText(this->hwndProgressLabel, _T(""));
}

/**
 * Event handler that's called when the Cancel button gets clicked. Won't be
 * called after the Cancel button becomes a Close button.
 *
 * @param hDlg Dialog window handle.
 * 
 * @return Value to be returned by the dialog's message handling procedure.
 */
INT_PTR ProgressDialog::OnCancel(HWND hDlg) {
	return 0;
}
