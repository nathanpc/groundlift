/**
 * ReceiveProgressDialog.cpp
 * A dialog that monitors the progress of a receive operation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "ReceiveProgressDialog.h"

#include <stdexcept>
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
ReceiveProgressDialog::ReceiveProgressDialog(HINSTANCE& hInst, HWND& hwndParent) :
	ProgressDialog(hInst, hwndParent, IDD_TRANSFER) {
	this->glServer = NULL;
}

/**
 * Frees up any resources allocated by this object.
 */
ReceiveProgressDialog::~ReceiveProgressDialog() {
}

/**
 * Populates the controls on the window with information about the impending
 * transfer.
 * 
 * @param peer Information about the peer that's sending us a file.
 * @param fb   File bundle containing information about our download.
 */
void ReceiveProgressDialog::InitiateTransfer(GroundLift::Peer &peer,
											 const file_bundle_t *fb) {
	float fSize;
	char cPrefix;
	LPTSTR szFilename;
	LPTSTR szIP;

	// Get a human-readable file size and convert some strings to UTF-16.
	fSize = file_size_readable(fb->size, &cPrefix);
	szFilename = utf16_mbstowcs(fb->base);
	szIP = peer.IPAddress();

	// Set the window title and context label.
	SetWindowFormatText(this->hDlg, _T("Receiving %s"), szFilename);
	SetWindowFormatText(this->hDlg, _T("%s is sending you %s"),
						peer.Hostname(), szFilename);

	// Free our temporary buffers.
	free(szFilename);
	free(szIP);

	// Set transfer target and start tracking the transfer rate.
	SetProgressTarget(fb);
	StartTransferRateTracking();
}

/**
 * Sets up all of the event handlers that we and the transfer are responsible
 * for.
 *
 * @param glServer GroundLift server object to have its event handlers set.
 */
void ReceiveProgressDialog::SetupEventHandlers(GroundLift::Server* glServer) {
	// Get a copy of the server in case we need to cancel a transfer.
	this->glServer = glServer;

	// Setup the event handlers.
	glServer->SetDownloadProgressEvent(OnProgress,
									   reinterpret_cast<void *>(this));
	glServer->SetDownloadSuccessEvent(OnSuccess,
									  reinterpret_cast<void *>(this));
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
INT_PTR CALLBACK ReceiveProgressDialog::DlgProc(HWND hDlg, UINT wMsg,
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
void ReceiveProgressDialog::SetupControls(HWND hDlg) {
	// Set some defaults first.
	ProgressDialog::SetupControls(hDlg);

	// Change the dialog's title to something more specific.
	SetWindowText(hDlg, _T("Receiving file..."));

	// Disable the open buttons.
	EnableOpenButtons(false);

	// Set some default texts for the labels.
	SetWindowText(this->hwndContextLabel, _T("Preparing to receive file..."));
	SetWindowText(this->hwndRateLabel, _T(""));
	SetWindowText(this->hwndProgressLabel, _T(""));
}

/**
 * Handles the transfer progress event.
 *
 * @param progress Structure containing all the information about the progress.
 * @param arg      Optional data set by the event handler setup.
 */
void ReceiveProgressDialog::OnProgress(
		const gl_server_conn_progress_t *progress, void *arg) {
	// Get ourselves from the optional argument.
	ReceiveProgressDialog *pThis = GetOurObjectPointer(arg);

	// Update the progress-related controls.
	pThis->SetProgressValue(progress->recv_bytes, false);
}

/**
 * Handles the transfer finished successfully event.
 * 
 * @param fb  File bundle that was uploaded.
 * @param arg Optional data set by the event handler setup.
 */
void ReceiveProgressDialog::OnSuccess(const file_bundle_t *fb, void *arg) {
	// Get ourselves from the optional argument.
	ReceiveProgressDialog *pThis = GetOurObjectPointer(arg);

	// Set the window title and context label.
	LPTSTR szBuffer = utf16_mbstowcs(fb->base);
	SetWindowFormatText(pThis->hDlg, _T("Received %s"), szBuffer);
	SetWindowText(pThis->hwndContextLabel, szBuffer);
	free(szBuffer);
	szBuffer = NULL;

	// Update the progress labels and stop the transfer rate update.
	pThis->SetProgressValue(fb->size, true);
	pThis->StopTransferRateTracking();

	// Change the cancel button into a close button.
	pThis->SwitchCancelButtonToClose(true, true);
}

/**
 * Event handler that's called when the Cancel button gets clicked. Won't be
 * called after the Cancel button becomes a Close button.
 *
 * @param hDlg Dialog window handle.
 *
 * @return Value to be returned by the dialog's message handling procedure.
 */
INT_PTR ReceiveProgressDialog::OnCancel(HWND hDlg) {
	// Cancel the transfer.
	// TODO: this->glServer->Cancel();

	// Update some controls on the window.
	SetWindowText(this->hwndContextLabel, _T("File transfer canceled"));
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
ReceiveProgressDialog *ReceiveProgressDialog::GetOurObjectPointer(
		void *lpvThis) {
	ReceiveProgressDialog *pThis = NULL;

	// Cast the pointer to ourselves.
	pThis = reinterpret_cast<ReceiveProgressDialog *>(lpvThis);
	if (pThis == NULL) {
		MsgBoxError(NULL, _T("Dialog Object Cast Failed"),
			_T("Failed to get instance of ReceiveProgressDialog from pointer."));
		throw std::exception(
			"Failed to get instance of ReceiveProgressDialog from pointer");
	}

	return pThis;
}
