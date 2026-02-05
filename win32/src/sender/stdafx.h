// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef _SENDER_STDAFX_H
#define _SENDER_STDAFX_H

#if _MSC_VER > 1000
#pragma once
#endif// _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers.
// Windows Header Files
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++ RunTime Header Files
#include <string>

// Resource definitions.
#include "Resource.h"

// Ensure we don't have to guard against LongPtr on 32-bit systems.
#ifndef SetWindowLongPtr
	#define SetWindowLongPtr SetWindowLong
#endif // !SetWindowLongPtr
#ifndef GetWindowLongPtr
	#define GetWindowLongPtr GetWindowLong
#endif // !GetWindowLongPtr
#ifndef GWLP_USERDATA
	#define GWLP_USERDATA GWL_USERDATA
#endif // !GWLP_USERDATA

// Shims and quality of life.
#include "../../cvtutf/Unicode.h"
#include "../utilities/MsgBoxes.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before
// the previous line.

#endif// _SENDER_STDAFX_H
