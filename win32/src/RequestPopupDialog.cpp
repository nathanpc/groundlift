/**
 * RequestPopupDialog.cpp
 * A popup dialog, similar to a modern notification, that allows the user to
 * accept or decline a transfer requested by a client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "RequestPopupDialog.h"

#include <stdexcept>
#include <utils/utf16.h>

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
 * Sets up all of the event handlers that we and the transfer are responsible
 * for.
 * 
 * @param glServer GroundLift server object to have its event handlers set.
 */
void RequestPopupDialog::SetupEventHandlers(GroundLift::Server* glServer) {
	glServer->SetClientTransferRequestEvent(OnTransferRequested,
										   reinterpret_cast<void*>(this));
	// TODO: Use SetConnectionCloseEvent for closing canceled requests.
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
	int posX = rcDesktop.right - (rcWindow.right - rcWindow.left);
	int posY = rcDesktop.bottom - (rcWindow.bottom - rcWindow.top);

	// Make ourselves the topmost window and reposition us.
	SetWindowPos(hDlg, HWND_TOPMOST, posX, posY, 0, 0, SWP_NOSIZE);
}

/**
 * Handles the Client Transfer Request event.
 *
 * @param req Information about the client and its request.
 * @param arg Optional data set by the event handler setup.
 *
 * @return Return 0 to refuse the connection request. Anything else will be
 *         treated as accepting.
 */
int RequestPopupDialog::OnTransferRequested(const gl_server_conn_req_t* req,
											void* arg) {
	float fSize;
	char cPrefix;
	LPTSTR szHostname;
	LPTSTR szIP;
	LPTSTR szFilename;

	// Get ourselves from the optional argument.
	RequestPopupDialog* pThis = RequestPopupDialog::GetOurObjectPointer(arg);

	// Get a human-readable file size and convert some strings to UTF-16.
	fSize = file_size_readable(req->fb->size, &cPrefix);
	szHostname = utf16_mbstowcs(req->hostname);
	szFilename = utf16_mbstowcs(req->fb->base);
	szIP = NULL;

	// Get the peer IP address.
	char* szBuffer;
	if (socket_itos(&szBuffer, (const struct sockaddr*)&req->conn->addr)) {
		szIP = utf16_mbstowcs(szBuffer);
		free(szBuffer);
		szBuffer = NULL;
	}

	// Set a nice notification message.
	if (cPrefix != 'B') {
		SetWindowFormatText(pThis->hwndInfoLabel,
							_T("%s (%s) wants to send you %s (%.0f bytes)"),
							szHostname, szIP, szFilename, fSize, cPrefix);
	} else {
		SetWindowFormatText(pThis->hwndInfoLabel,
							_T("%s (%s) wants to send you %s (%.2f %cB)"),
							szHostname, szIP, szFilename, fSize, cPrefix);
	}

	// Free our temporary buffers.
	free(szHostname);
	free(szIP);
	free(szFilename);

	// Show the notification popup.
	return pThis->ShowModal() == IDOK;
}

/**
 * Get our own object pointer statically from a void pointer.
 *
 * @param lpvThis Pointer to a valid instance of ourselves.
 *
 * @return Pointer to an instance of ourselves.
 */
RequestPopupDialog* RequestPopupDialog::GetOurObjectPointer(void* lpvThis) {
	RequestPopupDialog* pThis = NULL;

	// Cast the pointer to ourselves.
	pThis = reinterpret_cast<RequestPopupDialog*>(lpvThis);
	if (pThis == NULL) {
		MsgBoxError(NULL, _T("Dialog Object Cast Failed"),
			_T("Failed to get instance of RequestPopupDialog from pointer."));
		throw std::exception(
			"Failed to get instance of RequestPopupDialog from pointer");
	}

	return pThis;
}
