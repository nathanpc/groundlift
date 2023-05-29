/**
 * CommonIncludes.h
 * Commonly-used includes that are global to the project.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_COMMONINCLUDES_H
#define _WINGL_COMMONINCLUDES_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Windows Header Files
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>

// Project Resource Files
#include "../vs6/Desktop/resource.h"

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

// Utilities
#include <utils/capabilities.h>
#include <MsgBoxes.h>
#include <WindowUtilities.h>

#endif // _WINGL_COMMONINCLUDES_H
