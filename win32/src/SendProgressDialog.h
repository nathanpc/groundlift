/**
 * SendProgressDialog.h
 * A dialog that monitors the progress of a send operation.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_SENDPROGRESSDIALOG_H
#define _WINGL_SENDPROGRESSDIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <DialogWindow.h>

#include "Models/Peer.h"
#include "Controllers/Client.h"

/**
 * A dialog that monitors the progress of a send operation.
 */
class SendProgressDialog : public DialogWindow {
protected:
	HWND hwndContextLabel;
	HWND hwndProgressBar;
	HWND hwndRateLabel;
	HWND hwndProgressLabel;
	HWND hwndOpenFileButton;
	HWND hwndOpenFolderButton;
	HWND hwndCancelButton;

	GroundLift::Client glClient;

	void SwitchCancelButtonToClose();

public:
	SendProgressDialog(HINSTANCE& hInst, HWND& hwndParent);
	virtual ~SendProgressDialog();

	void SendFile(LPCTSTR szIP, LPCTSTR szFilePath);
	void SendFile(GroundLift::Peer& peer, LPCTSTR szFilePath);

private:
	void SetupControls(HWND hDlg);
	
	static void OnConnectionResponse(const file_bundle_t *fb,
									 bool accepted, void *arg);
	static void OnProgress(const gl_client_progress_t *progress, void *arg);
	static void OnSuccess(const file_bundle_t *fb, void *arg);

	INT_PTR CALLBACK DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
							 LPARAM lParam);
};

#endif // _WINGL_SENDPROGRESSDIALOG_H
