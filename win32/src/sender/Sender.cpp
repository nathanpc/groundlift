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

// Global variables.
static TCHAR szWindowClass[20];
static TCHAR szAppTitle[20];

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
	HACCEL hAccel;
	HWND hwndMain;
	int rc;
	
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

	// Load the application class and title.
	LoadString(hInstance, IDS_APP_CLASS, szWindowClass, 20);
	LoadString(hInstance, IDS_APP_TITLE, szAppTitle, 20);

	// Initialize the application.
	rc = RegisterApplication(hInstance);
	if (rc == 0) {
		MsgBoxError(NULL, _T("Error Registering Class"),
			_T("An error occurred while trying to register the application's ")
			_T("window class."));
		rc = 1;
		goto terminate;
	}

	// Initialize this single instance.
	hwndMain = InitializeInstance(hInstance, lpCmdLine, nCmdShow);
	if (hwndMain == 0) {
		MsgBoxError(NULL, _T("Error Initializing Instance"),
			_T("An error occurred while trying to initialize the ")
			_T("application's instance."));
		rc = 0x10;
		goto terminate;
	}

	// Load accelerators.
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));

	// Application message loop.
	while (GetMessage(&msg, NULL, 0, 0)) {
		// Translate accelerators.
		if (!TranslateAccelerator(hwndMain, hAccel, &msg)) {
			// Translate message.
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Terminate instance.
	rc = TerminateInstance(hInstance, (int)msg.wParam);

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
 * Initializes the application and registers the application class.
 *
 * @param hInstance Application instance.
 *
 * @return TRUE if the class was registered.
 */
ATOM RegisterApplication(HINSTANCE hInstance) {
	// Setup the application's main window class.
	WNDCLASSEX wcex;
	wcex.cbSize         = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)MainWindowProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SENDER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= GetSysColorBrush(COLOR_WINDOW);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDM_MAIN);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	// Register the application's main window class.
	return RegisterClassEx(&wcex);
}

/**
 * Initializes the instance and creates the window.
 *
 * @param hInstance Program instance.
 * @param lpCmdLine String with command line text.
 * @param nShowCmd  Initial state of the program's main window.
 *
 * @return Window handler.
 */
HWND InitializeInstance(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow) {
	HWND hWnd;

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
	hWnd = CreateWindow(szWindowClass,			// Window class.
						szAppTitle,				// Window title.
						WS_OVERLAPPEDWINDOW,	// Style flags.
						CW_USEDEFAULT,			// X position.
						CW_USEDEFAULT,			// Y position.
						300,					// Initial width,
						100,					// Initial height.
						NULL,					// Parent window.
						NULL,					// Menu class. (Always NULL)
						hInstance,				// Application instance.
						NULL);					// Pointer to create parameters.

	// Check if the window creation worked.
	if (!IsWindow(hWnd)) {
		MsgBoxError(NULL, _T("Error Initializing Instance"),
			_T("Window creation failed."));
		return NULL;
	}

	// Show and update the window.
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

/**
 * Terminates the application instance.
 *
 * @param hInstance Application instance.
 * @param nDefRC    Return code.
 *
 * @return Previous return code.
 */
int TerminateInstance(HINSTANCE hInstance, int nDefRC) {
	return nDefRC;
}

/**
 * Main window procedure.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return 0 if everything worked.
 */
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT wMsg, WPARAM wParam,
								LPARAM lParam) {
	switch (wMsg) {
		case WM_CREATE:
			return WndMainCreate(hWnd, wMsg, wParam, lParam);
		case WM_COMMAND:
			return WndMainCommand(hWnd, wMsg, wParam, lParam);
		case WM_NOTIFY:
			return WndMainNotify(hWnd, wMsg, wParam, lParam);
		case WM_CLOSE:
			return WndMainClose(hWnd, wMsg, wParam, lParam);
		case WM_DESTROY:
			return WndMainDestroy(hWnd, wMsg, wParam, lParam);
	}

	return DefWindowProc(hWnd, wMsg, wParam, lParam);
}

/**
 * Process the WM_CREATE message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainCreate(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	// Ensure that the common controls DLL is loaded and initialized.
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_DATE_CLASSES |
	             ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	// Grab the dimensions of the client area.
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	return 0;
}

/**
 * Process the WM_COMMAND message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainCommand(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	UINT_PTR wmId = LOWORD(wParam);
	UINT_PTR wmEvent = HIWORD(wParam);

	switch (wmId) {
	case IDM_EXIT:
		PostMessage(hWnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
		return 0;
	default:
		MsgBoxInfo(hWnd, _T("Unknown Command ID"),
			_T("WM_COMMAND for MainWindow with unknown ID"));
	}

	return DefWindowProc(hWnd, wMsg, wParam, lParam);
}

/**
 * Process the WM_NOTIFY message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Identifier of the common control sending the message. Not
 *               always unique. hwndFrom or idFrom of the NMHDR should be used.
 * @param lParam Pointer to an NMHDR structure containing the notification code
 *               and additional information.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainNotify(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	LPNMHDR nmh = (LPNMHDR)lParam;
	switch (nmh->code) {
	default:
		return DefWindowProc(hWnd, wMsg, wParam, lParam);
	}
}

/**
 * Process the WM_CLOSE message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam This parameter is not used.
 * @param lParam This parameter is not used.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainClose(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	// Send main window destruction message.
	DestroyWindow(hWnd);

	// Call any destructors that might be needed.
	// TODO: Call destructors.

	return DefWindowProc(hWnd, wMsg, wParam, lParam);
}

/**
 * Process the WM_DESTROY message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam This parameter is not used.
 * @param lParam This parameter is not used.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainDestroy(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	// Post quit message and return.
	PostQuitMessage(0);
	return 0;
}
