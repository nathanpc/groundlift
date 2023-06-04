/**
 * ProgressDialog.h
 * A generic dialog base class that monitors the progress of an operation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "ProgressDialog.h"

#include <shellapi.h>
#include <shlwapi.h>
#include <utils/utf16.h>

#include "CommonIncludes.h"

/* Ensure compilation under VC++ 6 with marquee style support. */
#ifndef PBS_MARQUEE
	#define PBS_MARQUEE 0x08
#endif /* !PBS_MARQUEE */
#ifndef PBM_SETMARQUEE
	#define PBM_SETMARQUEE (WM_USER + 10)
#endif /* !PBM_SETMARQUEE */

/**
 * Initializes the dialog window object.
 *
 * @param hInst      Application's instance that this dialog belongs to.
 * @param hwndParent Parent window handle.
 * @param wResID     Dialog resource ID.
 */
ProgressDialog::ProgressDialog(HINSTANCE& hInst, HWND& hwndParent,
							   WORD wResID) :
	DialogWindow(hInst, hwndParent, wResID) {
	this->hwndContextLabel = NULL;
	this->hwndProgressBar = NULL;
	this->hwndRateLabel = NULL;
	this->hwndProgressLabel = NULL;
	this->hwndOpenFileButton = NULL;
	this->hwndOpenFolderButton = NULL;
	this->hwndCancelButton = NULL;

	this->isButtonClose = false;
	this->uRateInterval = 500;

	this->szFilePath = NULL;
	this->fsTarget = 0;
	this->fsCurrent = 0;
	this->bDivideProgress = false;
	this->szTargetSize = NULL;
}

/**
 * Frees up any resources allocated by this object.
 */
ProgressDialog::~ProgressDialog() {
	if (this->szFilePath)
		free(this->szFilePath);
	if (this->szTargetSize)
		free(this->szTargetSize);
}

/**
 * Enables or disables the Open File and Open Folder buttons.
 *
 * @param bEnable Should the buttons be enabled?
 */
void ProgressDialog::EnableOpenButtons(bool bEnable) {
	EnableWindow(this->hwndOpenFileButton, bEnable);
	EnableWindow(this->hwndOpenFolderButton, bEnable);
}

/**
 * Switches the cancel button into a close button.
 * 
 * @param bMakeDefault Make the close button the default in the dialog.
 */
void ProgressDialog::SwitchCancelButtonToClose(bool bMakeDefault,
											   bool bEnableOpen) {
	// Check if Cancel should be the default button of the dialog.
	if (bMakeDefault)
		SetDlgDefaultButton(this->hDlg, IDCANCEL);

	// Enable or disable the open buttons.
	EnableOpenButtons(bEnableOpen);

	// Change the text of the button.
	SetWindowText(this->hwndCancelButton, _T("Close"));
	this->isButtonClose = true;
}

/**
 * Sets the path to where the file being transferred is located at.
 *
 * @param szFilePath Path to the file being transferred.
 */
void ProgressDialog::SetFilePath(LPCTSTR szFilePath) {
	// Check if there's another path already set and free it.
	if (this->szFilePath)
		free(this->szFilePath);

	// Duplicate the path and set it.
	this->szFilePath = _tcsdup(szFilePath);
}

/**
 * Sets the path to where the file being transferred is located at.
 * 
 * @param fb File bundle with the information about the file.
 */
void ProgressDialog::SetFilePath(const file_bundle_t* fb) {
	// Check if there's another path already set and free it.
	if (this->szFilePath)
		free(this->szFilePath);

	// Get the path from the file bundle and convert it to UTF-16.
	this->szFilePath = utf16_mbstowcs(fb->name);
}

/**
 * Turns the marquee mode of the progress bar ON or OFF.
 * 
 * @param bEnable Enable the marquee mode?
 */
void ProgressDialog::SetProgressBarMarquee(bool bEnabled) {
	if (bEnabled) {
		// Enable the marquee style and let it roll.
		SetWindowLongPtr(this->hwndProgressBar, GWL_STYLE,
			GetWindowLongPtr(this->hwndProgressBar, GWL_STYLE) | PBS_MARQUEE);
		SendMessage(this->hwndProgressBar, PBM_SETMARQUEE, TRUE, 0);
	} else {
		// Remove the marquee style from the window style flags.
		LONG_PTR lWnd = GetWindowLongPtr(this->hwndProgressBar, GWL_STYLE);
		lWnd &= ~(PBS_MARQUEE);

		// Stop the marquee animation and remove the style.
		SendMessage(this->hwndProgressBar, PBM_SETMARQUEE, FALSE, 0);
		SetWindowLongPtr(this->hwndProgressBar, GWL_STYLE, lWnd);
		SendMessage(this->hwndProgressBar, PBM_SETPOS, 0, 0);
	}
}

/**
 * Sets the progress-related controls to 100% target value.
 * 
 * @param fb File bundle with the information about the current file.
 */
void ProgressDialog::SetProgressTarget(const file_bundle_t* fb) {
	// Set the target and progress bar division factor if necessary.
	this->fsTarget = fb->size;
	this->bDivideProgress = fb->size > UINT_MAX;

	// Set the target size string.
	if (this->szTargetSize)
		free(this->szTargetSize);
	this->szTargetSize = _tcsdup(GetRoundedFileSize(fb->size));

	// Ensure the progress bar has its maximum value set.
	if (this->bDivideProgress) {
		SetProgressBarMax(static_cast<DWORD>(fb->size / 2));
	} else {
		SetProgressBarMax(static_cast<DWORD>(fb->size));
	}

	// Update the transfer rate label and the progress controls.
	SetWindowText(this->hwndRateLabel, _T(""));
	SetProgressValue(0, true);
}

/**
 * Sets the progress-related controls current value in relationship to their
 * target.
 * 
 * @param fsValue Current value of the progress.
 * @param bForce  Forcefully update the progress label value?
 */
void ProgressDialog::SetProgressValue(fsize_t fsValue, bool bForce) {
	fsCurrent = fsValue;

	SetProgressBarValue(fsValue);
	SetProgressLabelValue(fsValue, bForce);
}

/**
 * Start tracking the transfer rate.
 */
void ProgressDialog::StartTransferRateTracking() {
	SetTimer(this->hDlg, IDT_TRANSFER_RATE, this->uRateInterval,
			 static_cast<TIMERPROC>(NULL));
}

/**
 * Stop tracking the transfer rate.
 */
void ProgressDialog::StopTransferRateTracking() {
	KillTimer(this->hDlg, IDT_TRANSFER_RATE);
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
INT_PTR CALLBACK ProgressDialog::DlgProc(HWND hDlg, UINT wMsg,
										 WPARAM wParam, LPARAM lParam) {
	// Handle messages.
	switch (wMsg) {
		case WM_INITDIALOG:
			// Setup the controls for the operation.
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
				case IDC_OPEN_FILE:
					// Open File button was clicked.
					return OnOpenFile(hDlg);
				case IDC_OPEN_FOLDER:
					// Open Folder button was clicked.
					return OnOpenFolder(hDlg);
				case IDCANCEL: {
					// Call the cancel button event handler if needed.
					if (!isButtonClose) {
						INT_PTR iRet = OnCancel(hDlg);
						if (iRet)
							return iRet;
					}
					break;
				}
			}
			break;
		case WM_TIMER:
			switch (wParam) {
				case IDT_TRANSFER_RATE:
					// Update the transfer rate label.
					UpdateRateLabel();
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
void ProgressDialog::SetupControls(HWND hDlg) {
	// Get the handle of every useful control in the window.
	this->hwndContextLabel = GetDlgItem(hDlg,
										IDC_STATIC_TRANSFER_CONTEXT);
	this->hwndProgressBar = GetDlgItem(hDlg, IDC_PROGRESS);
	this->hwndRateLabel = GetDlgItem(hDlg, IDC_STATIC_RATE);
	this->hwndProgressLabel = GetDlgItem(hDlg, IDC_STATIC_PROGRESS);
	this->hwndOpenFileButton = GetDlgItem(hDlg, IDC_OPEN_FILE);
	this->hwndOpenFolderButton = GetDlgItem(hDlg, IDC_OPEN_FOLDER);
	this->hwndCancelButton = GetDlgItem(hDlg, IDCANCEL);

	// Set some default texts for the labels.
	SetWindowText(this->hwndRateLabel, _T(""));
	SetWindowText(this->hwndProgressLabel, _T(""));
}

/**
 * Sets the progress bar upper value.
 *
 * @param dwMaxRange Value that represents 100% of the progress.
 */
void ProgressDialog::SetProgressBarMax(DWORD dwMaxRange) {
	SendMessage(this->hwndProgressBar, PBM_SETRANGE32, 0,
				static_cast<LPARAM>(dwMaxRange));
}

/**
 * Sets the current value of the progress bar.
 *
 * @param dwValue Current value of the progress bar.
 */
void ProgressDialog::SetProgressBarValue(DWORD dwValue) {
	SendMessage(this->hwndProgressBar, PBM_SETPOS,
				static_cast<WPARAM>(dwValue), 0);
}

/**
 * Sets the current file size value of the progress bar.
 *
 * @param fsValue Current file size transfer value.
 */
void ProgressDialog::SetProgressBarValue(fsize_t fsValue) {
	if (this->bDivideProgress) {
		SetProgressBarValue(static_cast<DWORD>(fsValue / 2));
	} else {
		SetProgressBarValue(static_cast<DWORD>(fsValue));
	}
}

/**
 * Sets the current file size value of progress label.
 *
 * @param fsValue Current file size transfer value.
 * @param bForce  Forcefully update the label?
 */
void ProgressDialog::SetProgressLabelValue(fsize_t fsValue, bool bForce) {
	static fsize_t fsLast;

	// Check if it's worth updating the progress label.
	if (!bForce && ((fsize_t)(UINT64_TO_FLOAT(fsLast) * 1.01f) >= fsValue))
		return;

	// Update the progress label.
	SetWindowFormatText(this->hwndProgressLabel, _T("%s of %s"),
						GetRoundedFileSize(fsValue), this->szTargetSize);
	fsLast = fsValue;
}

/**
 * Updates the transfer rate label.
 */
void ProgressDialog::UpdateRateLabel() {
	static fsize_t fsLast;
	fsize_t fsRate;

	// Ignore the initial burst.
	if (fsLast == 0) {
		fsLast = this->fsCurrent;
		return;
	}

	// Calculate the transfer rate and update the label.
	fsRate = (fsize_t)(UINT64_TO_FLOAT(this->fsCurrent - fsLast) /
					   (UINT64_TO_FLOAT(this->uRateInterval) / 1000));
	SetWindowFormatText(this->hwndRateLabel, _T("%s/s"),
						GetRoundedFileSize(fsRate));

	// Reset the reference value.
	fsLast = this->fsCurrent;
}

/**
 * Gets a human-readable string with a file size and its unit rounded to the
 * nearest hundred.
 * 
 * @warning This function allocates memory that must be free'd by you.
 * 
 * @param fsSize File size to be rounded.
 * 
 * @return Newly allocated string with the file size and its unit.
 */
LPTSTR ProgressDialog::GetRoundedFileSize(fsize_t fsSize) {
	static TCHAR szBuffer[15];
	float fReadable;
	char cPrefix;
	WCHAR wcPrefix;

	// Get the human-readable file size.
	fReadable = file_size_readable(fsSize, &cPrefix);
	wcPrefix = btowc(cPrefix);

	// Build up a nice string for our file size.
	if (wcPrefix != L'B') {
		_sntprintf(szBuffer, 15, _T("%.2f %cB"), fReadable, wcPrefix);
	} else {
		_sntprintf(szBuffer, 15, _T("%.0f"), fReadable);
	}

	return szBuffer;
}

/**
 * Event handler that's called when the Open File button gets clicked.
 *
 * @param hDlg Dialog window handle.
 *
 * @return Value to be returned by the dialog's message handling procedure.
 */
INT_PTR ProgressDialog::OnOpenFile(HWND hDlg) {
	// Check if we have a file to open.
	if (this->szFilePath == NULL) {
		MsgBoxWarning(hDlg, _T("Can't open file"),
			_T("Couldn't open the file since it wasn't set in the dialog."));
		return TRUE;
	}

	// Open the file using the default program and close the dialog.
	ShellExecute(NULL, _T("open"), this->szFilePath, NULL, NULL, SW_SHOW);
	Close(IDOK);

	return TRUE;
}

/**
 * Event handler that's called when the Open Folder button gets clicked.
 *
 * @param hDlg Dialog window handle.
 *
 * @return Value to be returned by the dialog's message handling procedure.
 */
INT_PTR ProgressDialog::OnOpenFolder(HWND hDlg) {
	LPTSTR szFolder;
	LPTSTR szBuffer;

	// Check if we have a folder to open.
	if (this->szFilePath == NULL) {
		MsgBoxWarning(hDlg, _T("Can't open folder"),
			_T("Couldn't open the folder since it wasn't set in the dialog."));
		return TRUE;
	}

	// Get the folder from the file path.
	szFolder = _tcsdup(this->szFilePath);
	szBuffer = const_cast<LPTSTR>(PathFindFileName(szFolder)) - 1;
	*szBuffer = _T('\0');

	// Open the folder and close the dialog.
	ShellExecute(NULL, _T("open"), szFolder, NULL, NULL, SW_SHOW);
	Close(IDOK);

	return TRUE;
}

/**
 * Event handler that's called when the Cancel button gets clicked. Won't be
 * called after the Cancel button becomes a Close button.
 *
 * @param hDlg Dialog window handle.
 * 
 * @return Value to be returned by the dialog's message handling procedure.
 */
INT_PTR ProgressDialog::OnCancel(HWND hDlg) {
	// Stop the rate tracking and turn the cancel button into a close one.
	StopTransferRateTracking();
	SwitchCancelButtonToClose(true, false);

	return FALSE;
}
