/**
 * SendFileDialog.h
 * A dialog to send files to a specific client.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_SENDFILEDIALOG_H
#define _WINGL_SENDFILEDIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <DialogWindow.h>

/**
 * A dialog to send files to a specific client.
 */
class SendFileDialog : public DialogWindow {
protected:
	INT_PTR CALLBACK DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
							 LPARAM lParam);

public:
	SendFileDialog(HINSTANCE& hInst, HWND& hwndParent);
};

#endif // _WINGL_SENDFILEDIALOG_H
