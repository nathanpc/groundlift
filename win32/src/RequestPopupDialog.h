/**
 * RequestPopupDialog.h
 * A popup dialog, similar to a modern notification, that allows the user to
 * accept or decline a transfer requested by a client.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_REQUESRPOPUPDIALOG_H
#define _WINGL_REQUESRPOPUPDIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <DialogWindow.h>

#include "Controllers/Server.h"
#include "Models/Peer.h"
#include "ReceiveProgressDialog.h"

/**
 * Message sent when the popup should get opened.
 * 
 * @param wParam Should be 0.
 * @param lParam Pointer to a variable that will hold the acceptance response.
 */
#define WM_SHOWPOPUP (WM_APP + 0x01)

/**
 * A popup dialog, similar to a modern notification, that allows the user to
 * accept or decline a transfer requested by a client.
 */
class RequestPopupDialog : public DialogWindow {
protected:
	HWND hwndInfoLabel;
	HWND hwndAcceptButton;
	HWND hwndDeclineButton;
	ReceiveProgressDialog* dlgProgress;

	INT_PTR CALLBACK DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
							 LPARAM lParam);

public:
	RequestPopupDialog(HINSTANCE& hInst, HWND& hwndParent);
	virtual ~RequestPopupDialog();

	void ShowTransferProgress();

	void SetupEventHandlers(GroundLift::Server* glServer);

private:
	void MoveIntoPosition();

	static int OnTransferRequested(const gl_server_conn_req_t* req, void* arg);

	static RequestPopupDialog* GetOurObjectPointer(void* lpvThis);
};

#endif // _WINGL_REQUESRPOPUPDIALOG_H
