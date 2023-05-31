/**
 * ProgressDialog.h
 * A generic dialog base class that monitors the progress of an operation.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_PROGRESSDIALOG_H
#define _WINGL_PROGRESSDIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <DialogWindow.h>

/**
 * A generic dialog base class that monitors the progress of an operation.
 */
class ProgressDialog : public DialogWindow {
protected:
	HWND hwndContextLabel;
	HWND hwndProgressBar;
	HWND hwndRateLabel;
	HWND hwndProgressLabel;
	HWND hwndOpenFileButton;
	HWND hwndOpenFolderButton;
	HWND hwndCancelButton;

	bool isButtonClose;

	virtual void SetupControls(HWND hDlg);
	void SwitchCancelButtonToClose(bool bMakeDefault);

	virtual INT_PTR OnCancel(HWND hDlg);

	INT_PTR CALLBACK DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
							 LPARAM lParam);

public:
	ProgressDialog(HINSTANCE& hInst, HWND& hwndParent, WORD wResID);
	virtual ~ProgressDialog();
};

#endif // _WINGL_PROGRESSDIALOG_H
