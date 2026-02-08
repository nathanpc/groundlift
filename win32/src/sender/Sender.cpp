/**
 * Sender.cpp
 * The native Windows GUI port of the GroundLift client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "Sender.h"

#if defined(DEBUG) && !defined(UNDER_CE)
	#include <crtdbg.h>
#endif // defined(DEBUG) && !defined(UNDER_CE)

// Global state variables.
static HINSTANCE g_hInstance;
static HWND g_hWnd;

/**
 * Application's main entry point.
 *
 * @param hInstance     Program instance.
 * @param hPrevInstance Ignored: Leftover from Win16.
 * @param lpCmdLine     String with command line text.
 * @param nShowCmd      Initial state of the program's main window.
 *
 * @return wParam of the WM_QUIT message.
 */
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					  LPTSTR lpCmdLine, int nCmdShow) {
	MSG msg;
	int rc = 0;
	
	// Ensure we specify parameters not in use.
	UNREFERENCED_PARAMETER(hPrevInstance);

#if defined(_DEBUG) && !defined(UNDER_CE)
	// Initialize memory leak detection.
	_CrtMemState snapBegin;
	_CrtMemState snapEnd;
	_CrtMemState snapDiff;
	_CrtMemCheckpoint(&snapBegin);
#endif // defined(_DEBUG) && !defined(UNDER_CE)

	// Check if we can perform proper Unicode conversions.
	if (!UnicodeAssumptionsCheck()) {
		MsgBoxError(NULL, _T("Unicode Conformance Issue"),
			_T("Either the size of wchar_t is not 2 bytes long as required ")
			_T("for UTF-16, or char is not 1 byte long, as per UTF-8. All ")
			_T("Unicode conversions would fail."));
		return 1;
	}

	// Initialize the dialog window.
	g_hInstance = hInstance;
	g_hWnd = InitializeInstance(hInstance, lpCmdLine, nCmdShow);
	if (g_hWnd == NULL) {
		rc = 0x10;
		goto terminate;
	}

	// Application message loop.
	while (GetMessage(&msg, NULL, 0, 0)) {
		// Translate message.
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

terminate:
#if defined(_DEBUG) && !defined(UNDER_CE)
	// Detect memory leaks.
	_CrtMemCheckpoint(&snapEnd);
	if (_CrtMemDifference(&snapDiff, &snapBegin, &snapEnd)) {
		OutputDebugString(_T("MEMORY LEAKS DETECTED\r\n"));
		OutputDebugString(L"----------- _CrtMemDumpStatistics ---------\r\n");
		_CrtMemDumpStatistics(&snapDiff);
		OutputDebugString(L"----------- _CrtMemDumpAllObjectsSince ---------\r\n");
		_CrtMemDumpAllObjectsSince(&snapBegin);
		OutputDebugString(L"----------- _CrtDumpMemoryLeaks ---------\r\n");
		_CrtDumpMemoryLeaks();
	} else {
		OutputDebugString(_T("No memory leaks detected. Congratulations!\r\n"));
	}
#endif // defined(_DEBUG) && !defined(UNDER_CE)

	return rc;
}

/**
 * Initializes the instance and creates the dialog window.
 *
 * @param hInstance Program instance.
 * @param lpCmdLine String with command line text.
 * @param nShowCmd  Initial state of the program's main window.
 *
 * @return Dialog window handler.
 */
HWND InitializeInstance(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow) {
	HWND hDlg;

	// Initialize main window object.
	int numArgs = 0;
	LPTSTR *lpArgs = CommandLineToArgvW(GetCommandLine(), &numArgs);
	LPCTSTR szFilename = NULL;
	if (numArgs > 1)
		szFilename = lpArgs[1];
	LocalFree(lpArgs);
	lpArgs = NULL;
	szFilename = NULL;

	// Create the main window.
	hDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_SENDER), NULL,
						(DLGPROC)MainDlgProc);

	// Check if the window creation worked.
	if (hDlg == NULL) {
		MsgBoxError(NULL, _T("Error Initializing Instance"),
		            _T("An error occurred while trying to initialize the ")
		            _T("application's main dialog."));
		return NULL;
	}

	// Show the dialog window and return.
	ShowWindow(hDlg, SW_SHOW);
	return hDlg;
}

/**
 * Main diaog window procedure.
 *
 * @param hDlg   Dialog window handle.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return TRUE if everything worked.
 */
INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
							 LPARAM lParam) {
	switch (wMsg) {
		case WM_INITDIALOG:
			return DlgMainInit(hDlg, wMsg, wParam, lParam);
		case WM_COMMAND:
			return DlgMainCommand(hDlg, wMsg, wParam, lParam);
		case WM_CLOSE:
			return DlgMainClose(hDlg, wMsg, wParam, lParam);
		case WM_DESTROY:
			return DlgMainDestroy(hDlg, wMsg, wParam, lParam);
	}

	return (INT_PTR)false;
}

/**
 * Process the WM_INITDIALOG message for the window.
 *
 * @param hDlg   Dialog window handle.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return TRUE if everything worked.
 */
INT_PTR DlgMainInit(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	// Ensure that the common controls DLL is loaded and initialized.
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_DATE_CLASSES |
	             ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	// Set the dialog window icon.
	HICON hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_SENDER),
	                               IMAGE_ICON, GetSystemMetrics(SM_CXICON),
	                               GetSystemMetrics(SM_CYICON),
	                               LR_DEFAULTCOLOR);
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_SMALL),
	                         IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
	                         GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	return (INT_PTR)true;
}

/**
 * Process the WM_COMMAND message for the window.
 *
 * @param hDlg   Dialog window handle.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return TRUE if everything worked.
 */
INT_PTR DlgMainCommand(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	UINT_PTR wmId = LOWORD(wParam);
	UINT_PTR wmEvent = HIWORD(wParam);

	switch (wmId) {
		case IDOK:
		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			//EndDialog(hDlg, wmId);
			return (INT_PTR)true;
		default:
			MsgBoxInfo(hDlg, _T("Unknown Command ID"),
				_T("WM_COMMAND for main dialog with unknown ID"));
	}

	return (INT_PTR)false;
}

/**
 * Process the WM_CLOSE message for the window.
 *
 * @param hDlg   Dialog window handle.
 * @param wMsg   Message type.
 * @param wParam This parameter is not used.
 * @param lParam This parameter is not used.
 *
 * @return TRUE if everything worked.
 */
INT_PTR DlgMainClose(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	// Send window destruction message.
	DestroyWindow(hDlg);

	// Call any destructors that might be needed.
	// TODO: Call destructors.

	return (INT_PTR)true;
}

/**
 * Process the WM_DESTROY message for the window.
 *
 * @param hDlg   Dialog window handle.
 * @param wMsg   Message type.
 * @param wParam This parameter is not used.
 * @param lParam This parameter is not used.
 *
 * @return TRUE if everything worked.
 */
INT_PTR DlgMainDestroy(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	// Post quit message and return.
	PostQuitMessage(0);
	return (INT_PTR)true;
}
