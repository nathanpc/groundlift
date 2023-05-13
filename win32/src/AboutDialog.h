/**
 * AboutDialog.h
 * Application's about dialog.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_ABOUTDIALOG_H
#define _WINGL_ABOUTDIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <DialogWindow.h>

/**
 * Application's about dialog.
 */
class AboutDialog : public DialogWindow {
public:
	AboutDialog(HINSTANCE& hInst, HWND& hwndParent);
};

#endif // _WINGL_ABOUTDIALOG_H
