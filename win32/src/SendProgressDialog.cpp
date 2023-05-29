/**
 * SendProgressDialog.cpp
 * A dialog that monitors the progress of a send operation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "SendProgressDialog.h"

#include <shlwapi.h>
#include <utils/utf16.h>
#include <groundlift/defaults.h>

#include "CommonIncludes.h"

/**
 * Initializes the dialog window object.
 *
 * @param hInst      Application's instance that this dialog belongs to.
 * @param hwndParent Parent window handle.
 */
SendProgressDialog::SendProgressDialog(HINSTANCE& hInst, HWND& hwndParent) :
	DialogWindow(hInst, hwndParent, IDD_TRANSFER) {
	this->hwndContextLabel = NULL;
	this->hwndProgressBar = NULL;
	this->hwndRateLabel = NULL;
	this->hwndProgressLabel = NULL;
	this->hwndOpenFileButton = NULL;
	this->hwndOpenFolderButton = NULL;
	this->hwndCancelButton = NULL;

	// Setup event handlers.
	this->glClient.SetConnectionResponseEvent(
		SendProgressDialog::OnConnectionResponse,
		reinterpret_cast<void *>(this));
	this->glClient.SetProgressEvent(SendProgressDialog::OnProgress,
									reinterpret_cast<void *>(this));
	this->glClient.SetSuccessEvent(SendProgressDialog::OnSuccess,
								   reinterpret_cast<void *>(this));
}

/**
 * Frees up any resources allocated by this object.
 */
SendProgressDialog::~SendProgressDialog() {
}

/**
 * Starts transferring a file to a peer.
 * 
 * @param szIP       IP address of the server.
 * @param szFilePath Path to the file to be sent.
 */
void SendProgressDialog::SendFile(LPCTSTR szIP, LPCTSTR szFilePath) {
	LPCTSTR szBasename;

	// Get the file basename.
	szBasename = PathFindFileName(szFilePath);

	// Set the context label text.
	SetWindowFormatText(this->hwndContextLabel,
						_T("Waiting for %s to accept %s"), szIP, szBasename);

	// TODO: Put progress bar in marquee mode.

	// TODO: Update progress label.

	// Start the transfer.
	this->glClient.SendFile(szIP, TCPSERVER_PORT, szFilePath);
}

/**
 * Starts transferring a file to a peer.
 *
 * @param peer       Peer to send the file to.
 * @param szFilePath Path to the file to be sent.
 */
void SendProgressDialog::SendFile(GroundLift::Peer& peer, LPCTSTR szFilePath) {
	LPTSTR szIP = peer.IPAddress();
	SendFile(szIP, szFilePath);
	free(szIP);
}

/**
 * Switches the cancel button into a close button.
 */
void SendProgressDialog::SwitchCancelButtonToClose() {
	// TODO: Make the default button of the dialog.

	// Change the text of the button.
	SetWindowText(this->hwndCancelButton, _T("Close"));
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
INT_PTR CALLBACK SendProgressDialog::DlgProc(HWND hDlg, UINT wMsg,
											 WPARAM wParam, LPARAM lParam) {
	// Handle messages.
	switch (wMsg) {
		case WM_INITDIALOG:
			// Get the handle of every useful control in the window.
			this->hwndContextLabel = GetDlgItem(hDlg,
												IDC_STATIC_TRANSFER_CONTEXT);
			this->hwndProgressBar = GetDlgItem(hDlg, IDC_PROGRESS);
			this->hwndRateLabel = GetDlgItem(hDlg, IDC_STATIC_RATE);
			this->hwndProgressLabel = GetDlgItem(hDlg, IDC_STATIC_PROGRESS);
			this->hwndOpenFileButton = GetDlgItem(hDlg, IDC_OPEN_FILE);
			this->hwndOpenFolderButton = GetDlgItem(hDlg, IDC_OPEN_FOLDER);
			this->hwndCancelButton = GetDlgItem(hDlg, IDCANCEL);

			// Setup the controls for this operation.
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
				case IDCANCEL:
					MsgBox(hDlg, MB_OK, L"Send File", L"CANCELED!");
					break;
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
void SendProgressDialog::SetupControls(HWND hDlg) {
	// Change the dialog's title to something more specific.
	SetWindowText(hDlg, _T("Sending file..."));

	// Hide the controls that we won't be needing.
	ShowWindow(this->hwndOpenFileButton, SW_HIDE);
	ShowWindow(this->hwndOpenFolderButton, SW_HIDE);

	// Set some default texts for the labels.
	SetWindowText(this->hwndContextLabel, _T("Preparing to send file..."));
	SetWindowText(this->hwndRateLabel, _T(""));
	SetWindowText(this->hwndProgressLabel, _T(""));
}

/**
 * Handles the connection response event.
 * 
 * @param fb       File bundle that was uploaded.
 * @param accepted Has the peer accepted the file transfer?
 * @param arg      Optional data set by the event handler setup.
 */
void SendProgressDialog::OnConnectionResponse(const file_bundle_t *fb,
											  bool accepted, void *arg) {
	// Get ourselves from the optional argument.
	SendProgressDialog *pThis = SendProgressDialog::GetOurObjectPointer(arg);

	// TODO: Setup the progress bar and progress label.

	// Update the controls depending on the peer's response.
	if (!accepted) {
		SetWindowText(pThis->hwndContextLabel,
					  _T("Peer declined our file transfer"));
		pThis->SwitchCancelButtonToClose();
		return;
	} else {
		SetWindowText(pThis->hwndContextLabel,
					  _T("Peer accepted our file transfer"));
	}
}

/**
 * Handles the transfer progress event.
 *
 * @param progress Structure containing all the information about the progress.
 * @param arg      Optional data set by the event handler setup.
 */
void SendProgressDialog::OnProgress(const gl_client_progress_t *progress,
									void *arg) {
	// Get ourselves from the optional argument.
	SendProgressDialog *pThis = SendProgressDialog::GetOurObjectPointer(arg);

	// Update the progress label.
	SetWindowFormatText(pThis->hwndProgressLabel,
						_T("%lu bytes of %I64u bytes"), progress->sent_bytes,
						progress->fb->size);

	// TODO: Update the progress bar with chunks.

	// TODO: Update the transfer rate label.
}

/**
 * Handles the transfer finished successfully event.
 * 
 * @param fb  File bundle that was uploaded.
 * @param arg Optional data set by the event handler setup.
 */
void SendProgressDialog::OnSuccess(const file_bundle_t *fb, void *arg) {
	// Get ourselves from the optional argument.
	SendProgressDialog *pThis = SendProgressDialog::GetOurObjectPointer(arg);

	// Update the context label.
	SetWindowText(pThis->hwndContextLabel,
				  _T("Successfully transferred the file"));

	// Change the cancel button into a close button.
	pThis->SwitchCancelButtonToClose();
}

/**
 * Get our own object pointer statically from a void pointer.
 * 
 * @param lpvThis Pointer to a valid instance of ourselves.
 * 
 * @return Pointer to an instance of ourselves.
 */
SendProgressDialog *SendProgressDialog::GetOurObjectPointer(void *lpvThis) {
	SendProgressDialog *pThis = NULL;

	// Cast the pointer to ourselves.
	pThis = reinterpret_cast<SendProgressDialog *>(lpvThis);
	if (pThis == NULL) {
		MsgBoxError(NULL, _T("Dialog Object Cast Failed"),
			_T("Failed to get instance of SendProgressDialog from pointer."));
		throw std::exception(
			"Failed to get instance of SendProgressDialog from pointer");
	}

	return pThis;
}
