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
#include <utils/filesystem.h>

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

	virtual void SetupControls(HWND hDlg);

	void EnableOpenButtons(bool bEnable);
	void SwitchCancelButtonToClose(bool bMakeDefault, bool bEnableOpen);

	void SetFilePath(LPCTSTR szFilePath);
	void SetFilePath(const file_bundle_t* fb);

	void SetProgressBarMarquee(bool bEnabled);
	void SetProgressTarget(const file_bundle_t* fb);
	void SetProgressValue(fsize_t fsValue, bool bForce);

	void StartTransferRateTracking();
	void StopTransferRateTracking();

	virtual INT_PTR OnOpenFile(HWND hDlg);
	virtual INT_PTR OnOpenFolder(HWND hDlg);
	virtual INT_PTR OnCancel(HWND hDlg);

	INT_PTR CALLBACK DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
							 LPARAM lParam);

public:
	ProgressDialog(HINSTANCE& hInst, HWND& hwndParent, WORD wResID);
	virtual ~ProgressDialog();

private:
	bool isButtonClose;
	UINT uRateInterval;

	LPTSTR szFilePath;
	fsize_t fsTarget;
	fsize_t fsCurrent;
	bool bDivideProgress;
	LPTSTR szTargetSize;

	void SetProgressBarMax(DWORD dwMaxRange);
	void SetProgressBarValue(DWORD dwValue);
	void SetProgressBarValue(fsize_t fsValue);

	void SetProgressLabelValue(fsize_t fsValue, bool bForce);
	void UpdateRateLabel();

	LPTSTR GetRoundedFileSize(fsize_t fsSize);
};

#endif // _WINGL_PROGRESSDIALOG_H
