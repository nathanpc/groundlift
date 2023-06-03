/**
 * ReceiveProgressDialog.h
 * A dialog that monitors the progress of a receive operation.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_RECEIVEPROGRESSDIALOG_H
#define _WINGL_RECEIVEPROGRESSDIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

#include "ProgressDialog.h"
#include "Controllers/Server.h"
#include "Models/Peer.h"

/**
 * A dialog that monitors the progress of a receive operation.
 */
class ReceiveProgressDialog : public ProgressDialog {
protected:
	GroundLift::Server* glServer;

	INT_PTR OnCancel(HWND hDlg);

	void SetupControls(HWND hDlg);
	INT_PTR CALLBACK DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
							 LPARAM lParam);

public:
	ReceiveProgressDialog(HINSTANCE& hInst, HWND& hwndParent);
	virtual ~ReceiveProgressDialog();

	void InitiateTransfer(GroundLift::Peer& peer, const file_bundle_t *fb);

	void SetupEventHandlers(GroundLift::Server* glServer);

private:
	static void OnProgress(const gl_server_conn_progress_t *progress,
						   void *arg);
	static void OnSuccess(const file_bundle_t *fb, void *arg);

	static ReceiveProgressDialog *GetOurObjectPointer(void *lpvThis);
};

#endif // _WINGL_RECEIVEPROGRESSDIALOG_H
