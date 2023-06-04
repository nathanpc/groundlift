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

	this->glPeer = NULL;
	this->lpFB = NULL;

	// Setup our progress dialog.
	this->dlgProgress = new ReceiveProgressDialog(this->hInst, hwndParent);
}

/**
 * Frees up any resources allocated by this object.
 */
RequestPopupDialog::~RequestPopupDialog() {
	if (this->glPeer)
		delete this->glPeer;
	if (this->lpFB)
		file_bundle_free(this->lpFB);
	if (this->dlgProgress)
		delete this->dlgProgress;
}

/**
 * Shows the transfer progress dialog.
 */
void RequestPopupDialog::ShowTransferProgress() {
	this->dlgProgress->Show();
}

/**
 * Sets up all of the event handlers that we and the transfer are responsible
 * for.
 * 
 * @param glServer GroundLift server object to have its event handlers set.
 */
void RequestPopupDialog::SetupEventHandlers(GroundLift::Server* glServer) {
	// Setup our own event handlers.
	glServer->SetClientTransferRequestEvent(OnTransferRequested,
											reinterpret_cast<void*>(this));
	// TODO: Use SetConnectionCloseEvent for closing canceled requests.

	// Setup the progress dialog event handlers.
	this->dlgProgress->SetupEventHandlers(glServer);
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
			// Setup the controls and move ourselves into position.
			SetupControls();
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
	}

	// Pass the message to the default message handler.
	return DefaultDlgProc(hDlg, wMsg, wParam, lParam);
}

/**
 * Sets up the initial state of the controls in the dialog.
 */
void RequestPopupDialog::SetupControls() {
	// Get the handle of every useful control in the window.
	this->hwndInfoLabel = GetDlgItem(this->hDlg, IDC_STATIC_NOTIFICATION);
	this->hwndAcceptButton = GetDlgItem(this->hDlg, IDOK);
	this->hwndDeclineButton = GetDlgItem(this->hDlg, IDCANCEL);

	// Populate the notification with some context.
	PopulateNotification();
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
 * Populates the notification with some context.
 */
void RequestPopupDialog::PopulateNotification() {
	float fSize;
	char cPrefix;
	LPTSTR szFilename;
	LPTSTR szIP;

	// Get a human-readable file size and IP address.
	fSize = file_size_readable(this->lpFB->size, &cPrefix);
	szFilename = utf16_mbstowcs(this->lpFB->base);
	szIP = this->glPeer->IPAddress();

	// Set a nice notification message.
	if (cPrefix != 'B') {
		SetWindowFormatText(this->hwndInfoLabel,
			_T("%s (%s) wants to send you %s (%.0f bytes)"),
			glPeer->Hostname(), szIP, szFilename, fSize, cPrefix);
	} else {
		SetWindowFormatText(this->hwndInfoLabel,
			_T("%s (%s) wants to send you %s (%.2f %cB)"),
			glPeer->Hostname(), szIP, szFilename, fSize, cPrefix);
	}

	// Free our temporary buffers.
	free(szFilename);
	free(szIP);
}

/**
 * Sets the context of the notification that's about to be shown.
 * 
 * @param fb   Information about the file to be transferred.
 * @param peer Pointer to a peer object that will be stored by us.
 */
void RequestPopupDialog::SetNotificationContext(const file_bundle_t* fb,
												GroundLift::Peer* peer) {
	// Copy over the file bundle.
	if (this->lpFB)
		file_bundle_free(this->lpFB);
	this->lpFB = file_bundle_dup(fb);

	// Set the peer object.
	if (this->glPeer)
		delete this->glPeer;
	this->glPeer = peer;
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
	int nAccepted;

	// Get ourselves from the optional argument.
	RequestPopupDialog* pThis = GetOurObjectPointer(arg);

	// Create our peer object and set our context.
	GroundLift::Peer* peer = new GroundLift::Peer("?", req->hostname,
		(const struct sockaddr*)&req->conn->addr, req->conn->addr_size);
	pThis->SetNotificationContext(req->fb, peer);

	// Send the message to show the notification popup.
	nAccepted = 0;
	SendMessage(pThis->hwndParent, WM_SHOWPOPUP, 0,
				reinterpret_cast<LPARAM>(&nAccepted));
	if (nAccepted)
		pThis->dlgProgress->InitiateTransfer(*peer, req->fb);

	return nAccepted;
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
