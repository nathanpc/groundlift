/**
 * Sender.h
 * The native Windows GUI port of the GroundLift client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _SENDER_APP_H
#define _SENDER_APP_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"

// Initialization routines.
HWND InitializeInstance(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow);

// Dialog window procedure.
INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
							 LPARAM lParam);

// Dialog window message handlers.
INT_PTR DlgMainInit(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR DlgMainCommand(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR DlgMainClose(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR DlgMainDestroy(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

#endif // _SENDER_APP_H
