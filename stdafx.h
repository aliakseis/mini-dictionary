// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0500		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target Windows XP or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _CRT_SECURE_NO_DEPRECATE

#define _ATL_NO_EXCEPTIONS
#define _ATL_SINGLE_THREADED
#define _ATL_NO_FORCE_LIBS

#include "excpt.h"

#ifdef _ATL_MIN_CRT

#define __try
#define __except(foo) if (false)
#define __finally
#define _exception_code() (0)

#endif

#define _ATL_SINGLE_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE


// turns off ATL's hiding of some common and often safely ignored warning messages
#define _ATL_ALL_WARNINGS


#include "resource.h"

#include <memory.h>

#pragma intrinsic(memset, memcpy)

#include <atlbase.h>

#include <commctrl.h>

#include <MSHTML.H>


#include <OLEACC.H>


using namespace ATL;


#include <atlwin.h>
#include <atlcom.h>


#ifndef MIIM_STRING
#define MIIM_STRING      0x00000040
#endif

#ifndef TTS_BALLOON
#define TTS_BALLOON 0x40
#endif

#ifndef OBJID_NATIVEOM
#define OBJID_NATIVEOM      ((LONG)0xFFFFFFF0)
#endif

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
