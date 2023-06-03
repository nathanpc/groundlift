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
	ProgressDialog(hInst, hwndParent, IDD_TRANSFER) {
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

	// Put progress bar in marquee mode while we wait for a response.
	SetProgressBarMarquee(true);

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
			// Setup the controls for this operation.
			SetupControls(hDlg);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					break;
			}
			break;
	}

	// Pass the message to the default message handler.
	return ProgressDialog::DlgProc(hDlg, wMsg, wParam, lParam);
}

/**
 * Sets up the controls for the current operation.
 * 
 * @param hDlg Dialog window handle.
 */
void SendProgressDialog::SetupControls(HWND hDlg) {
	// Set some defaults first.
	ProgressDialog::SetupControls(hDlg);

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
	LPTSTR szBuffer;

	// Get ourselves from the optional argument.
	SendProgressDialog *pThis = SendProgressDialog::GetOurObjectPointer(arg);

	// Set the window title.
	szBuffer = utf16_mbstowcs(fb->base);
	SetWindowFormatText(pThis->hDlg, _T("Sending %s"), szBuffer);
	free(szBuffer);
	szBuffer = NULL;

	// Setup the progress bar and progress label.
	pThis->SetProgressBarMarquee(false);
	pThis->SetProgressTarget(fb);

	// Update the controls depending on the peer's response.
	if (!accepted) {
		SetWindowText(pThis->hwndContextLabel,
					  _T("Peer declined our file transfer"));
		pThis->SwitchCancelButtonToClose(true, false);
	} else {
		SetWindowText(pThis->hwndContextLabel,
					  _T("Peer accepted our file transfer"));
		pThis->StartTransferRateTracking();
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

	// Update the progress-related controls.
	pThis->SetProgressValue(progress->sent_bytes, false);
}

/**
 * Handles the transfer finished successfully event.
 * 
 * @param fb  File bundle that was uploaded.
 * @param arg Optional data set by the event handler setup.
 */
void SendProgressDialog::OnSuccess(const file_bundle_t *fb, void *arg) {
	LPTSTR szBuffer;

	// Get ourselves from the optional argument.
	SendProgressDialog *pThis = SendProgressDialog::GetOurObjectPointer(arg);

	// Set the window title.
	szBuffer = utf16_mbstowcs(fb->base);
	SetWindowFormatText(pThis->hDlg, _T("Sent %s"), szBuffer);
	free(szBuffer);
	szBuffer = NULL;

	// Update the context and progress labels.
	SetWindowText(pThis->hwndContextLabel,
				  _T("Successfully transferred the file"));
	pThis->SetProgressValue(fb->size, true);

	// Stop the transfer rate update.
	pThis->StopTransferRateTracking();

	// Change the cancel button into a close button.
	pThis->SwitchCancelButtonToClose(true, false);
}

/**
 * Event handler that's called when the Cancel button gets clicked. Won't be
 * called after the Cancel button becomes a Close button.
 *
 * @param hDlg Dialog window handle.
 *
 * @return Value to be returned by the dialog's message handling procedure.
 */
INT_PTR SendProgressDialog::OnCancel(HWND hDlg) {
	// Cancel the transfer.
	this->glClient.Cancel();

	// Update some controls on the window.
	SetWindowText(this->hwndContextLabel, _T("Canceled the file transfer"));
	ProgressDialog::OnCancel(hDlg);

	// Prevent the dialog from closing.
	return TRUE;
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
