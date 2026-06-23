// Stub DirectX Error header for MinGW
#ifndef _DXERR_H_
#define _DXERR_H_
#include <windows.h>
static inline const char* DXGetErrorString(HRESULT hr) { return "Error"; }
static inline const char* DXGetErrorDescription(HRESULT hr) { return "DirectX Error"; }
static inline HRESULT DXTrace(const char* file, int line, HRESULT hr, const char* msg, bool pop) { return hr; }
#define DXGetErrorStringA(x) "Error"
#define DXGetErrorDescriptionA(x) "DirectX Error"
#endif
