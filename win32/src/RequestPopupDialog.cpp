/**
 * RequestPopupDialog.cpp
 * A popup dialog, similar to a modern notification, that allows the user to
 * accept or decline a transfer requested by a client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "RequestPopupDialog.h"

#include "CommonIncludes.h"

/**
 * Initializes the dialog window object.
 *
 * @param hInst      Application's instance that this dialog belongs to.
 * @param hwndParent Parent window handle.
 */
RequestPopupDialog::RequestPopupDialog(HINSTANCE& hInst, HWND& hwndParent) :
	DialogWindow(hInst, hwndParent, IDD_REQUEST_POPUP) {
	this->hwndInfoLabel = NULL;
	this->hwndAcceptButton = NULL;
	this->hwndDeclineButton = NULL;
}

/**
 * Frees up any resources allocated by this object.
 */
RequestPopupDialog::~RequestPopupDialog() {
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
INT_PTR CALLBACK RequestPopupDialog::DlgProc(HWND hDlg, UINT wMsg,
											 WPARAM wParam, LPARAM lParam) {
	// Handle messages.
	switch (wMsg) {
		case WM_INITDIALOG:
			// Get the handle of every useful control in the window.
			this->hwndInfoLabel = GetDlgItem(hDlg, IDC_STATIC_NOTIFICATION);
			this->hwndAcceptButton = GetDlgItem(hDlg, IDOK);
			this->hwndDeclineButton = GetDlgItem(hDlg, IDCANCEL);

			// Move ourselves into position.
			MoveIntoPosition();
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
				case IDOK:
					break;
				case IDCANCEL:
					break;
			}
			break;
	}

	// Pass the message to the default message handler.
	return DefaultDlgProc(hDlg, wMsg, wParam, lParam);
}

/**
 * Moves the dialog into its position at the bottom right-hand corner of the
 * desktop.
 */
void RequestPopupDialog::MoveIntoPosition() {
	RECT rcDesktop;
	RECT rcWindow;

	// Get the desktop (without the taskbar) rectangle.
	if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0)) {
		// At least make ourselves the topmost window.
		SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		return;
	}

	// Get our own metrics.
	GetWindowRect(this->hDlg, &rcWindow);

	// Calculate the new position.
	int posX = rcDesktop.right - rcWindow.right;
	int posY = rcDesktop.bottom - rcWindow.bottom;

	// Make ourselves the topmost window and reposition us.
	SetWindowPos(hDlg, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE);
}
