/**
 * DialogWindow.h
 * Base class for a dialog window.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINCOMMON_DIALOGWINDOW_H
#define _WINCOMMON_DIALOGWINDOW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

/**
 * Base class for a dialog window.
 */
class DialogWindow {
protected:
	HINSTANCE& hInst;
	HWND& hwndParent;
	HWND hDlg;
	WORD wResID;
	bool bIsModal;

	void RegisterHandle(HWND hDlg);

	INT_PTR CALLBACK DefaultDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
									LPARAM lParam);
	virtual INT_PTR CALLBACK DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
									 LPARAM lParam);

public:
	DialogWindow(HINSTANCE& hInst, HWND& hwndParent, WORD wResID);
	virtual ~DialogWindow();

	virtual bool Show();
	virtual INT_PTR ShowModal();
	virtual void Close(INT_PTR nResult);

	static INT_PTR CALLBACK DlgProcWrapper(HWND hDlg, UINT wMsg, WPARAM wParam,
										   LPARAM lParam);
};

#endif // _WINCOMMON_DIALOGWINDOW_H
