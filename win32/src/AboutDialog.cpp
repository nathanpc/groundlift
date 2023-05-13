/**
 * AboutDialog.cpp
 * Application's about dialog.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "AboutDialog.h"

#include "CommonIncludes.h"

/**
 * Initializes the dialog window object.
 *
 * @param hInst      Application's instance that this dialog belongs to.
 * @param hwndParent Parent window handle.
 */
AboutDialog::AboutDialog(HINSTANCE& hInst, HWND& hwndParent) :
	DialogWindow(hInst, hwndParent, IDD_ABOUTBOX) {
}
