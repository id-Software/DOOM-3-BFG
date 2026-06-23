// Stub Windowsx header for MinGW
#ifndef _WINDOWSX_H_
#define _WINDOWSX_H_
#include <windows.h>

// Macro definitions for compatibility
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

// GWL_* -> GWLP_* for x64 compatibility
#ifndef GWL_WNDPROC
#define GWL_WNDPROC GWLP_WNDPROC
#endif
#ifndef GWL_HINSTANCE
#define GWL_HINSTANCE GWLP_HINSTANCE
#endif
#ifndef GWL_HWNDPARENT
#define GWL_HWNDPARENT GWLP_HWNDPARENT
#endif
#ifndef GWL_USERDATA
#define GWL_USERDATA GWLP_USERDATA
#endif

#endif
