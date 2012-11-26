/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
/*
** QGL_WIN.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Doom you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/
#pragma hdrstop
#include "../../idlib/precompiled.h"

#include <float.h>
#include "win_local.h"
#include "../../renderer/tr_local.h"


int   ( WINAPI * qwglChoosePixelFormat )(HDC, CONST PIXELFORMATDESCRIPTOR *);
int   ( WINAPI * qwglDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
int   ( WINAPI * qwglGetPixelFormat)(HDC);
BOOL  ( WINAPI * qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
BOOL  ( WINAPI * qwglSwapBuffers)(HDC);

BOOL  ( WINAPI * qwglCopyContext)(HGLRC, HGLRC, UINT);
HGLRC ( WINAPI * qwglCreateContext)(HDC);
HGLRC ( WINAPI * qwglCreateLayerContext)(HDC, int);
BOOL  ( WINAPI * qwglDeleteContext)(HGLRC);
HGLRC ( WINAPI * qwglGetCurrentContext)(VOID);
HDC   ( WINAPI * qwglGetCurrentDC)(VOID);
PROC  ( WINAPI * qwglGetProcAddress)(LPCSTR);
BOOL  ( WINAPI * qwglMakeCurrent)(HDC, HGLRC);
BOOL  ( WINAPI * qwglShareLists)(HGLRC, HGLRC);
BOOL  ( WINAPI * qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

BOOL  ( WINAPI * qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT,
                                           FLOAT, int, LPGLYPHMETRICSFLOAT);

BOOL ( WINAPI * qwglDescribeLayerPlane)(HDC, int, int, UINT,
                                            LPLAYERPLANEDESCRIPTOR);
int  ( WINAPI * qwglSetLayerPaletteEntries)(HDC, int, int, int,
                                                CONST COLORREF *);
int  ( WINAPI * qwglGetLayerPaletteEntries)(HDC, int, int, int,
                                                COLORREF *);
BOOL ( WINAPI * qwglRealizeLayerPalette)(HDC, int, BOOL);
BOOL ( WINAPI * qwglSwapLayerBuffers)(HDC, UINT);

void ( APIENTRY * qglAccum )(GLenum op, GLfloat value);
void ( APIENTRY * qglAlphaFunc )(GLenum func, GLclampf ref);
GLboolean ( APIENTRY * qglAreTexturesResident )(GLsizei n, const GLuint *textures, GLboolean *residences);
void ( APIENTRY * qglArrayElement )(GLint i);
void ( APIENTRY * qglBegin )(GLenum mode);
void ( APIENTRY * qglBindTexture )(GLenum target, GLuint texture);
void ( APIENTRY * qglBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
void ( APIENTRY * qglCallList )(GLuint list);
void ( APIENTRY * qglCallLists )(GLsizei n, GLenum type, const GLvoid *lists);
void ( APIENTRY * qglClear )(GLbitfield mask);
void ( APIENTRY * qglClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ( APIENTRY * qglClearDepth )(GLclampd depth);
void ( APIENTRY * qglClearIndex )(GLfloat c);
void ( APIENTRY * qglClearStencil )(GLint s);
void ( APIENTRY * qglClipPlane )(GLenum plane, const GLdouble *equation);
void ( APIENTRY * qglColor3b )(GLbyte red, GLbyte green, GLbyte blue);
void ( APIENTRY * qglColor3bv )(const GLbyte *v);
void ( APIENTRY * qglColor3d )(GLdouble red, GLdouble green, GLdouble blue);
void ( APIENTRY * qglColor3dv )(const GLdouble *v);
void ( APIENTRY * qglColor3f )(GLfloat red, GLfloat green, GLfloat blue);
void ( APIENTRY * qglColor3fv )(const GLfloat *v);
void ( APIENTRY * qglColor3i )(GLint red, GLint green, GLint blue);
void ( APIENTRY * qglColor3iv )(const GLint *v);
void ( APIENTRY * qglColor3s )(GLshort red, GLshort green, GLshort blue);
void ( APIENTRY * qglColor3sv )(const GLshort *v);
void ( APIENTRY * qglColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
void ( APIENTRY * qglColor3ubv )(const GLubyte *v);
void ( APIENTRY * qglColor3ui )(GLuint red, GLuint green, GLuint blue);
void ( APIENTRY * qglColor3uiv )(const GLuint *v);
void ( APIENTRY * qglColor3us )(GLushort red, GLushort green, GLushort blue);
void ( APIENTRY * qglColor3usv )(const GLushort *v);
void ( APIENTRY * qglColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
void ( APIENTRY * qglColor4bv )(const GLbyte *v);
void ( APIENTRY * qglColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void ( APIENTRY * qglColor4dv )(const GLdouble *v);
void ( APIENTRY * qglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( APIENTRY * qglColor4fv )(const GLfloat *v);
void ( APIENTRY * qglColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
void ( APIENTRY * qglColor4iv )(const GLint *v);
void ( APIENTRY * qglColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
void ( APIENTRY * qglColor4sv )(const GLshort *v);
void ( APIENTRY * qglColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void ( APIENTRY * qglColor4ubv )(const GLubyte *v);
void ( APIENTRY * qglColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
void ( APIENTRY * qglColor4uiv )(const GLuint *v);
void ( APIENTRY * qglColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
void ( APIENTRY * qglColor4usv )(const GLushort *v);
void ( APIENTRY * qglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void ( APIENTRY * qglColorMaterial )(GLenum face, GLenum mode);
void ( APIENTRY * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void ( APIENTRY * qglCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
void ( APIENTRY * qglCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void ( APIENTRY * qglCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void ( APIENTRY * qglCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglCullFace )(GLenum mode);
void ( APIENTRY * qglDeleteLists )(GLuint list, GLsizei range);
void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint *textures);
void ( APIENTRY * qglDepthFunc )(GLenum func);
void ( APIENTRY * qglDepthMask )(GLboolean flag);
void ( APIENTRY * qglDepthRange )(GLclampd zNear, GLclampd zFar);
void ( APIENTRY * qglDisable )(GLenum cap);
void ( APIENTRY * qglDisableClientState )(GLenum array);
void ( APIENTRY * qglDrawArrays )(GLenum mode, GLint first, GLsizei count);
void ( APIENTRY * qglDrawBuffer )(GLenum mode);
void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void ( APIENTRY * qglDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglEdgeFlag )(GLboolean flag);
void ( APIENTRY * qglEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglEdgeFlagv )(const GLboolean *flag);
void ( APIENTRY * qglEnable )(GLenum cap);
void ( APIENTRY * qglEnableClientState )(GLenum array);
void ( APIENTRY * qglEnd )(void);
void ( APIENTRY * qglEndList )(void);
void ( APIENTRY * qglEvalCoord1d )(GLdouble u);
void ( APIENTRY * qglEvalCoord1dv )(const GLdouble *u);
void ( APIENTRY * qglEvalCoord1f )(GLfloat u);
void ( APIENTRY * qglEvalCoord1fv )(const GLfloat *u);
void ( APIENTRY * qglEvalCoord2d )(GLdouble u, GLdouble v);
void ( APIENTRY * qglEvalCoord2dv )(const GLdouble *u);
void ( APIENTRY * qglEvalCoord2f )(GLfloat u, GLfloat v);
void ( APIENTRY * qglEvalCoord2fv )(const GLfloat *u);
void ( APIENTRY * qglEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
void ( APIENTRY * qglEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void ( APIENTRY * qglEvalPoint1 )(GLint i);
void ( APIENTRY * qglEvalPoint2 )(GLint i, GLint j);
void ( APIENTRY * qglFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer);
void ( APIENTRY * qglFinish )(void);
void ( APIENTRY * qglFlush )(void);
void ( APIENTRY * qglFogf )(GLenum pname, GLfloat param);
void ( APIENTRY * qglFogfv )(GLenum pname, const GLfloat *params);
void ( APIENTRY * qglFogi )(GLenum pname, GLint param);
void ( APIENTRY * qglFogiv )(GLenum pname, const GLint *params);
void ( APIENTRY * qglFrontFace )(GLenum mode);
void ( APIENTRY * qglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLuint ( APIENTRY * qglGenLists )(GLsizei range);
void ( APIENTRY * qglGenTextures )(GLsizei n, GLuint *textures);
void ( APIENTRY * qglGetBooleanv )(GLenum pname, GLboolean *params);
void ( APIENTRY * qglGetClipPlane )(GLenum plane, GLdouble *equation);
void ( APIENTRY * qglGetDoublev )(GLenum pname, GLdouble *params);
GLenum ( APIENTRY * qglGetError )(void);
void ( APIENTRY * qglGetFloatv )(GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetIntegerv )(GLenum pname, GLint *params);
void ( APIENTRY * qglGetLightfv )(GLenum light, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetLightiv )(GLenum light, GLenum pname, GLint *params);
void ( APIENTRY * qglGetMapdv )(GLenum target, GLenum query, GLdouble *v);
void ( APIENTRY * qglGetMapfv )(GLenum target, GLenum query, GLfloat *v);
void ( APIENTRY * qglGetMapiv )(GLenum target, GLenum query, GLint *v);
void ( APIENTRY * qglGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetMaterialiv )(GLenum face, GLenum pname, GLint *params);
void ( APIENTRY * qglGetPixelMapfv )(GLenum map, GLfloat *values);
void ( APIENTRY * qglGetPixelMapuiv )(GLenum map, GLuint *values);
void ( APIENTRY * qglGetPixelMapusv )(GLenum map, GLushort *values);
void ( APIENTRY * qglGetPointerv )(GLenum pname, GLvoid* *params);
void ( APIENTRY * qglGetPolygonStipple )(GLubyte *mask);
const GLubyte * ( APIENTRY * qglGetString )(GLenum name);
void ( APIENTRY * qglGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetTexEnviv )(GLenum target, GLenum pname, GLint *params);
void ( APIENTRY * qglGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params);
void ( APIENTRY * qglGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetTexGeniv )(GLenum coord, GLenum pname, GLint *params);
void ( APIENTRY * qglGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY * qglGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params);
void ( APIENTRY * qglGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetTexParameteriv )(GLenum target, GLenum pname, GLint *params);
void ( APIENTRY * qglHint )(GLenum target, GLenum mode);
void ( APIENTRY * qglIndexMask )(GLuint mask);
void ( APIENTRY * qglIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglIndexd )(GLdouble c);
void ( APIENTRY * qglIndexdv )(const GLdouble *c);
void ( APIENTRY * qglIndexf )(GLfloat c);
void ( APIENTRY * qglIndexfv )(const GLfloat *c);
void ( APIENTRY * qglIndexi )(GLint c);
void ( APIENTRY * qglIndexiv )(const GLint *c);
void ( APIENTRY * qglIndexs )(GLshort c);
void ( APIENTRY * qglIndexsv )(const GLshort *c);
void ( APIENTRY * qglIndexub )(GLubyte c);
void ( APIENTRY * qglIndexubv )(const GLubyte *c);
void ( APIENTRY * qglInitNames )(void);
void ( APIENTRY * qglInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean ( APIENTRY * qglIsEnabled )(GLenum cap);
GLboolean ( APIENTRY * qglIsList )(GLuint list);
GLboolean ( APIENTRY * qglIsTexture )(GLuint texture);
void ( APIENTRY * qglLightModelf )(GLenum pname, GLfloat param);
void ( APIENTRY * qglLightModelfv )(GLenum pname, const GLfloat *params);
void ( APIENTRY * qglLightModeli )(GLenum pname, GLint param);
void ( APIENTRY * qglLightModeliv )(GLenum pname, const GLint *params);
void ( APIENTRY * qglLightf )(GLenum light, GLenum pname, GLfloat param);
void ( APIENTRY * qglLightfv )(GLenum light, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglLighti )(GLenum light, GLenum pname, GLint param);
void ( APIENTRY * qglLightiv )(GLenum light, GLenum pname, const GLint *params);
void ( APIENTRY * qglLineStipple )(GLint factor, GLushort pattern);
void ( APIENTRY * qglLineWidth )(GLfloat width);
void ( APIENTRY * qglListBase )(GLuint base);
void ( APIENTRY * qglLoadIdentity )(void);
void ( APIENTRY * qglLoadMatrixd )(const GLdouble *m);
void ( APIENTRY * qglLoadMatrixf )(const GLfloat *m);
void ( APIENTRY * qglLoadName )(GLuint name);
void ( APIENTRY * qglLogicOp )(GLenum opcode);
void ( APIENTRY * qglMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
void ( APIENTRY * qglMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
void ( APIENTRY * qglMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
void ( APIENTRY * qglMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
void ( APIENTRY * qglMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
void ( APIENTRY * qglMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
void ( APIENTRY * qglMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void ( APIENTRY * qglMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void ( APIENTRY * qglMaterialf )(GLenum face, GLenum pname, GLfloat param);
void ( APIENTRY * qglMaterialfv )(GLenum face, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglMateriali )(GLenum face, GLenum pname, GLint param);
void ( APIENTRY * qglMaterialiv )(GLenum face, GLenum pname, const GLint *params);
void ( APIENTRY * qglMatrixMode )(GLenum mode);
void ( APIENTRY * qglMultMatrixd )(const GLdouble *m);
void ( APIENTRY * qglMultMatrixf )(const GLfloat *m);
void ( APIENTRY * qglNewList )(GLuint list, GLenum mode);
void ( APIENTRY * qglNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
void ( APIENTRY * qglNormal3bv )(const GLbyte *v);
void ( APIENTRY * qglNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
void ( APIENTRY * qglNormal3dv )(const GLdouble *v);
void ( APIENTRY * qglNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
void ( APIENTRY * qglNormal3fv )(const GLfloat *v);
void ( APIENTRY * qglNormal3i )(GLint nx, GLint ny, GLint nz);
void ( APIENTRY * qglNormal3iv )(const GLint *v);
void ( APIENTRY * qglNormal3s )(GLshort nx, GLshort ny, GLshort nz);
void ( APIENTRY * qglNormal3sv )(const GLshort *v);
void ( APIENTRY * qglNormalPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglPassThrough )(GLfloat token);
void ( APIENTRY * qglPixelMapfv )(GLenum map, GLsizei mapsize, const GLfloat *values);
void ( APIENTRY * qglPixelMapuiv )(GLenum map, GLsizei mapsize, const GLuint *values);
void ( APIENTRY * qglPixelMapusv )(GLenum map, GLsizei mapsize, const GLushort *values);
void ( APIENTRY * qglPixelStoref )(GLenum pname, GLfloat param);
void ( APIENTRY * qglPixelStorei )(GLenum pname, GLint param);
void ( APIENTRY * qglPixelTransferf )(GLenum pname, GLfloat param);
void ( APIENTRY * qglPixelTransferi )(GLenum pname, GLint param);
void ( APIENTRY * qglPixelZoom )(GLfloat xfactor, GLfloat yfactor);
void ( APIENTRY * qglPointSize )(GLfloat size);
void ( APIENTRY * qglPolygonMode )(GLenum face, GLenum mode);
void ( APIENTRY * qglPolygonOffset )(GLfloat factor, GLfloat units);
void ( APIENTRY * qglPolygonStipple )(const GLubyte *mask);
void ( APIENTRY * qglPopAttrib )(void);
void ( APIENTRY * qglPopClientAttrib )(void);
void ( APIENTRY * qglPopMatrix )(void);
void ( APIENTRY * qglPopName )(void);
void ( APIENTRY * qglPrioritizeTextures )(GLsizei n, const GLuint *textures, const GLclampf *priorities);
void ( APIENTRY * qglPushAttrib )(GLbitfield mask);
void ( APIENTRY * qglPushClientAttrib )(GLbitfield mask);
void ( APIENTRY * qglPushMatrix )(void);
void ( APIENTRY * qglPushName )(GLuint name);
void ( APIENTRY * qglRasterPos2d )(GLdouble x, GLdouble y);
void ( APIENTRY * qglRasterPos2dv )(const GLdouble *v);
void ( APIENTRY * qglRasterPos2f )(GLfloat x, GLfloat y);
void ( APIENTRY * qglRasterPos2fv )(const GLfloat *v);
void ( APIENTRY * qglRasterPos2i )(GLint x, GLint y);
void ( APIENTRY * qglRasterPos2iv )(const GLint *v);
void ( APIENTRY * qglRasterPos2s )(GLshort x, GLshort y);
void ( APIENTRY * qglRasterPos2sv )(const GLshort *v);
void ( APIENTRY * qglRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY * qglRasterPos3dv )(const GLdouble *v);
void ( APIENTRY * qglRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglRasterPos3fv )(const GLfloat *v);
void ( APIENTRY * qglRasterPos3i )(GLint x, GLint y, GLint z);
void ( APIENTRY * qglRasterPos3iv )(const GLint *v);
void ( APIENTRY * qglRasterPos3s )(GLshort x, GLshort y, GLshort z);
void ( APIENTRY * qglRasterPos3sv )(const GLshort *v);
void ( APIENTRY * qglRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void ( APIENTRY * qglRasterPos4dv )(const GLdouble *v);
void ( APIENTRY * qglRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( APIENTRY * qglRasterPos4fv )(const GLfloat *v);
void ( APIENTRY * qglRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
void ( APIENTRY * qglRasterPos4iv )(const GLint *v);
void ( APIENTRY * qglRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
void ( APIENTRY * qglRasterPos4sv )(const GLshort *v);
void ( APIENTRY * qglReadBuffer )(GLenum mode);
void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY * qglRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void ( APIENTRY * qglRectdv )(const GLdouble *v1, const GLdouble *v2);
void ( APIENTRY * qglRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void ( APIENTRY * qglRectfv )(const GLfloat *v1, const GLfloat *v2);
void ( APIENTRY * qglRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
void ( APIENTRY * qglRectiv )(const GLint *v1, const GLint *v2);
void ( APIENTRY * qglRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void ( APIENTRY * qglRectsv )(const GLshort *v1, const GLshort *v2);
GLint ( APIENTRY * qglRenderMode )(GLenum mode);
void ( APIENTRY * qglRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY * qglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglScaled )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY * qglScalef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglSelectBuffer )(GLsizei size, GLuint *buffer);
void ( APIENTRY * qglShadeModel )(GLenum mode);
void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
void ( APIENTRY * qglStencilMask )(GLuint mask);
void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
void ( APIENTRY * qglTexCoord1d )(GLdouble s);
void ( APIENTRY * qglTexCoord1dv )(const GLdouble *v);
void ( APIENTRY * qglTexCoord1f )(GLfloat s);
void ( APIENTRY * qglTexCoord1fv )(const GLfloat *v);
void ( APIENTRY * qglTexCoord1i )(GLint s);
void ( APIENTRY * qglTexCoord1iv )(const GLint *v);
void ( APIENTRY * qglTexCoord1s )(GLshort s);
void ( APIENTRY * qglTexCoord1sv )(const GLshort *v);
void ( APIENTRY * qglTexCoord2d )(GLdouble s, GLdouble t);
void ( APIENTRY * qglTexCoord2dv )(const GLdouble *v);
void ( APIENTRY * qglTexCoord2f )(GLfloat s, GLfloat t);
void ( APIENTRY * qglTexCoord2fv )(const GLfloat *v);
void ( APIENTRY * qglTexCoord2i )(GLint s, GLint t);
void ( APIENTRY * qglTexCoord2iv )(const GLint *v);
void ( APIENTRY * qglTexCoord2s )(GLshort s, GLshort t);
void ( APIENTRY * qglTexCoord2sv )(const GLshort *v);
void ( APIENTRY * qglTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
void ( APIENTRY * qglTexCoord3dv )(const GLdouble *v);
void ( APIENTRY * qglTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
void ( APIENTRY * qglTexCoord3fv )(const GLfloat *v);
void ( APIENTRY * qglTexCoord3i )(GLint s, GLint t, GLint r);
void ( APIENTRY * qglTexCoord3iv )(const GLint *v);
void ( APIENTRY * qglTexCoord3s )(GLshort s, GLshort t, GLshort r);
void ( APIENTRY * qglTexCoord3sv )(const GLshort *v);
void ( APIENTRY * qglTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void ( APIENTRY * qglTexCoord4dv )(const GLdouble *v);
void ( APIENTRY * qglTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void ( APIENTRY * qglTexCoord4fv )(const GLfloat *v);
void ( APIENTRY * qglTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
void ( APIENTRY * qglTexCoord4iv )(const GLint *v);
void ( APIENTRY * qglTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
void ( APIENTRY * qglTexCoord4sv )(const GLshort *v);
void ( APIENTRY * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglTexEnvi )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglTexEnviv )(GLenum target, GLenum pname, const GLint *params);
void ( APIENTRY * qglTexGend )(GLenum coord, GLenum pname, GLdouble param);
void ( APIENTRY * qglTexGendv )(GLenum coord, GLenum pname, const GLdouble *params);
void ( APIENTRY * qglTexGenf )(GLenum coord, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglTexGeni )(GLenum coord, GLenum pname, GLint param);
void ( APIENTRY * qglTexGeniv )(GLenum coord, GLenum pname, const GLint *params);
void ( APIENTRY * qglTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglTexParameteriv )(GLenum target, GLenum pname, const GLint *params);
void ( APIENTRY * qglTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTranslated )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY * qglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglVertex2d )(GLdouble x, GLdouble y);
void ( APIENTRY * qglVertex2dv )(const GLdouble *v);
void ( APIENTRY * qglVertex2f )(GLfloat x, GLfloat y);
void ( APIENTRY * qglVertex2fv )(const GLfloat *v);
void ( APIENTRY * qglVertex2i )(GLint x, GLint y);
void ( APIENTRY * qglVertex2iv )(const GLint *v);
void ( APIENTRY * qglVertex2s )(GLshort x, GLshort y);
void ( APIENTRY * qglVertex2sv )(const GLshort *v);
void ( APIENTRY * qglVertex3d )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY * qglVertex3dv )(const GLdouble *v);
void ( APIENTRY * qglVertex3f )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglVertex3fv )(const GLfloat *v);
void ( APIENTRY * qglVertex3i )(GLint x, GLint y, GLint z);
void ( APIENTRY * qglVertex3iv )(const GLint *v);
void ( APIENTRY * qglVertex3s )(GLshort x, GLshort y, GLshort z);
void ( APIENTRY * qglVertex3sv )(const GLshort *v);
void ( APIENTRY * qglVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void ( APIENTRY * qglVertex4dv )(const GLdouble *v);
void ( APIENTRY * qglVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( APIENTRY * qglVertex4fv )(const GLfloat *v);
void ( APIENTRY * qglVertex4i )(GLint x, GLint y, GLint z, GLint w);
void ( APIENTRY * qglVertex4iv )(const GLint *v);
void ( APIENTRY * qglVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
void ( APIENTRY * qglVertex4sv )(const GLshort *v);
void ( APIENTRY * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);



static void ( APIENTRY * dllAccum )(GLenum op, GLfloat value);
static void ( APIENTRY * dllAlphaFunc )(GLenum func, GLclampf ref);
GLboolean ( APIENTRY * dllAreTexturesResident )(GLsizei n, const GLuint *textures, GLboolean *residences);
static void ( APIENTRY * dllArrayElement )(GLint i);
static void ( APIENTRY * dllBegin )(GLenum mode);
static void ( APIENTRY * dllBindTexture )(GLenum target, GLuint texture);
static void ( APIENTRY * dllBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
static void ( APIENTRY * dllBlendFunc )(GLenum sfactor, GLenum dfactor);
static void ( APIENTRY * dllCallList )(GLuint list);
static void ( APIENTRY * dllCallLists )(GLsizei n, GLenum type, const GLvoid *lists);
static void ( APIENTRY * dllClear )(GLbitfield mask);
static void ( APIENTRY * dllClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static void ( APIENTRY * dllClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static void ( APIENTRY * dllClearDepth )(GLclampd depth);
static void ( APIENTRY * dllClearIndex )(GLfloat c);
static void ( APIENTRY * dllClearStencil )(GLint s);
static void ( APIENTRY * dllClipPlane )(GLenum plane, const GLdouble *equation);
static void ( APIENTRY * dllColor3b )(GLbyte red, GLbyte green, GLbyte blue);
static void ( APIENTRY * dllColor3bv )(const GLbyte *v);
static void ( APIENTRY * dllColor3d )(GLdouble red, GLdouble green, GLdouble blue);
static void ( APIENTRY * dllColor3dv )(const GLdouble *v);
static void ( APIENTRY * dllColor3f )(GLfloat red, GLfloat green, GLfloat blue);
static void ( APIENTRY * dllColor3fv )(const GLfloat *v);
static void ( APIENTRY * dllColor3i )(GLint red, GLint green, GLint blue);
static void ( APIENTRY * dllColor3iv )(const GLint *v);
static void ( APIENTRY * dllColor3s )(GLshort red, GLshort green, GLshort blue);
static void ( APIENTRY * dllColor3sv )(const GLshort *v);
static void ( APIENTRY * dllColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
static void ( APIENTRY * dllColor3ubv )(const GLubyte *v);
static void ( APIENTRY * dllColor3ui )(GLuint red, GLuint green, GLuint blue);
static void ( APIENTRY * dllColor3uiv )(const GLuint *v);
static void ( APIENTRY * dllColor3us )(GLushort red, GLushort green, GLushort blue);
static void ( APIENTRY * dllColor3usv )(const GLushort *v);
static void ( APIENTRY * dllColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
static void ( APIENTRY * dllColor4bv )(const GLbyte *v);
static void ( APIENTRY * dllColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
static void ( APIENTRY * dllColor4dv )(const GLdouble *v);
static void ( APIENTRY * dllColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static void ( APIENTRY * dllColor4fv )(const GLfloat *v);
static void ( APIENTRY * dllColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
static void ( APIENTRY * dllColor4iv )(const GLint *v);
static void ( APIENTRY * dllColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
static void ( APIENTRY * dllColor4sv )(const GLshort *v);
static void ( APIENTRY * dllColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
static void ( APIENTRY * dllColor4ubv )(const GLubyte *v);
static void ( APIENTRY * dllColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
static void ( APIENTRY * dllColor4uiv )(const GLuint *v);
static void ( APIENTRY * dllColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
static void ( APIENTRY * dllColor4usv )(const GLushort *v);
static void ( APIENTRY * dllColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static void ( APIENTRY * dllColorMaterial )(GLenum face, GLenum mode);
static void ( APIENTRY * dllColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
static void ( APIENTRY * dllCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
static void ( APIENTRY * dllCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
static void ( APIENTRY * dllCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
static void ( APIENTRY * dllCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
static void ( APIENTRY * dllCullFace )(GLenum mode);
static void ( APIENTRY * dllDeleteLists )(GLuint list, GLsizei range);
static void ( APIENTRY * dllDeleteTextures )(GLsizei n, const GLuint *textures);
static void ( APIENTRY * dllDepthFunc )(GLenum func);
static void ( APIENTRY * dllDepthMask )(GLboolean flag);
static void ( APIENTRY * dllDepthRange )(GLclampd zNear, GLclampd zFar);
static void ( APIENTRY * dllDisable )(GLenum cap);
static void ( APIENTRY * dllDisableClientState )(GLenum array);
static void ( APIENTRY * dllDrawArrays )(GLenum mode, GLint first, GLsizei count);
static void ( APIENTRY * dllDrawBuffer )(GLenum mode);
static void ( APIENTRY * dllDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
static void ( APIENTRY * dllDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllEdgeFlag )(GLboolean flag);
static void ( APIENTRY * dllEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllEdgeFlagv )(const GLboolean *flag);
static void ( APIENTRY * dllEnable )(GLenum cap);
static void ( APIENTRY * dllEnableClientState )(GLenum array);
static void ( APIENTRY * dllEnd )(void);
static void ( APIENTRY * dllEndList )(void);
static void ( APIENTRY * dllEvalCoord1d )(GLdouble u);
static void ( APIENTRY * dllEvalCoord1dv )(const GLdouble *u);
static void ( APIENTRY * dllEvalCoord1f )(GLfloat u);
static void ( APIENTRY * dllEvalCoord1fv )(const GLfloat *u);
static void ( APIENTRY * dllEvalCoord2d )(GLdouble u, GLdouble v);
static void ( APIENTRY * dllEvalCoord2dv )(const GLdouble *u);
static void ( APIENTRY * dllEvalCoord2f )(GLfloat u, GLfloat v);
static void ( APIENTRY * dllEvalCoord2fv )(const GLfloat *u);
static void ( APIENTRY * dllEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
static void ( APIENTRY * dllEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
static void ( APIENTRY * dllEvalPoint1 )(GLint i);
static void ( APIENTRY * dllEvalPoint2 )(GLint i, GLint j);
static void ( APIENTRY * dllFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer);
static void ( APIENTRY * dllFinish )(void);
static void ( APIENTRY * dllFlush )(void);
static void ( APIENTRY * dllFogf )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllFogfv )(GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllFogi )(GLenum pname, GLint param);
static void ( APIENTRY * dllFogiv )(GLenum pname, const GLint *params);
static void ( APIENTRY * dllFrontFace )(GLenum mode);
static void ( APIENTRY * dllFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLuint ( APIENTRY * dllGenLists )(GLsizei range);
static void ( APIENTRY * dllGenTextures )(GLsizei n, GLuint *textures);
static void ( APIENTRY * dllGetBooleanv )(GLenum pname, GLboolean *params);
static void ( APIENTRY * dllGetClipPlane )(GLenum plane, GLdouble *equation);
static void ( APIENTRY * dllGetDoublev )(GLenum pname, GLdouble *params);
GLenum ( APIENTRY * dllGetError )(void);
static void ( APIENTRY * dllGetFloatv )(GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetIntegerv )(GLenum pname, GLint *params);
static void ( APIENTRY * dllGetLightfv )(GLenum light, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetLightiv )(GLenum light, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetMapdv )(GLenum target, GLenum query, GLdouble *v);
static void ( APIENTRY * dllGetMapfv )(GLenum target, GLenum query, GLfloat *v);
static void ( APIENTRY * dllGetMapiv )(GLenum target, GLenum query, GLint *v);
static void ( APIENTRY * dllGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetMaterialiv )(GLenum face, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetPixelMapfv )(GLenum map, GLfloat *values);
static void ( APIENTRY * dllGetPixelMapuiv )(GLenum map, GLuint *values);
static void ( APIENTRY * dllGetPixelMapusv )(GLenum map, GLushort *values);
static void ( APIENTRY * dllGetPointerv )(GLenum pname, GLvoid* *params);
static void ( APIENTRY * dllGetPolygonStipple )(GLubyte *mask);
const GLubyte * ( APIENTRY * dllGetString )(GLenum name);
static void ( APIENTRY * dllGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexEnviv )(GLenum target, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params);
static void ( APIENTRY * dllGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexGeniv )(GLenum coord, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
static void ( APIENTRY * dllGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexParameteriv )(GLenum target, GLenum pname, GLint *params);
static void ( APIENTRY * dllHint )(GLenum target, GLenum mode);
static void ( APIENTRY * dllIndexMask )(GLuint mask);
static void ( APIENTRY * dllIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllIndexd )(GLdouble c);
static void ( APIENTRY * dllIndexdv )(const GLdouble *c);
static void ( APIENTRY * dllIndexf )(GLfloat c);
static void ( APIENTRY * dllIndexfv )(const GLfloat *c);
static void ( APIENTRY * dllIndexi )(GLint c);
static void ( APIENTRY * dllIndexiv )(const GLint *c);
static void ( APIENTRY * dllIndexs )(GLshort c);
static void ( APIENTRY * dllIndexsv )(const GLshort *c);
static void ( APIENTRY * dllIndexub )(GLubyte c);
static void ( APIENTRY * dllIndexubv )(const GLubyte *c);
static void ( APIENTRY * dllInitNames )(void);
static void ( APIENTRY * dllInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean ( APIENTRY * dllIsEnabled )(GLenum cap);
GLboolean ( APIENTRY * dllIsList )(GLuint list);
GLboolean ( APIENTRY * dllIsTexture )(GLuint texture);
static void ( APIENTRY * dllLightModelf )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllLightModelfv )(GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllLightModeli )(GLenum pname, GLint param);
static void ( APIENTRY * dllLightModeliv )(GLenum pname, const GLint *params);
static void ( APIENTRY * dllLightf )(GLenum light, GLenum pname, GLfloat param);
static void ( APIENTRY * dllLightfv )(GLenum light, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllLighti )(GLenum light, GLenum pname, GLint param);
static void ( APIENTRY * dllLightiv )(GLenum light, GLenum pname, const GLint *params);
static void ( APIENTRY * dllLineStipple )(GLint factor, GLushort pattern);
static void ( APIENTRY * dllLineWidth )(GLfloat width);
static void ( APIENTRY * dllListBase )(GLuint base);
static void ( APIENTRY * dllLoadIdentity )(void);
static void ( APIENTRY * dllLoadMatrixd )(const GLdouble *m);
static void ( APIENTRY * dllLoadMatrixf )(const GLfloat *m);
static void ( APIENTRY * dllLoadName )(GLuint name);
static void ( APIENTRY * dllLogicOp )(GLenum opcode);
static void ( APIENTRY * dllMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
static void ( APIENTRY * dllMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
static void ( APIENTRY * dllMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
static void ( APIENTRY * dllMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
static void ( APIENTRY * dllMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
static void ( APIENTRY * dllMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
static void ( APIENTRY * dllMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
static void ( APIENTRY * dllMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
static void ( APIENTRY * dllMaterialf )(GLenum face, GLenum pname, GLfloat param);
static void ( APIENTRY * dllMaterialfv )(GLenum face, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllMateriali )(GLenum face, GLenum pname, GLint param);
static void ( APIENTRY * dllMaterialiv )(GLenum face, GLenum pname, const GLint *params);
static void ( APIENTRY * dllMatrixMode )(GLenum mode);
static void ( APIENTRY * dllMultMatrixd )(const GLdouble *m);
static void ( APIENTRY * dllMultMatrixf )(const GLfloat *m);
static void ( APIENTRY * dllNewList )(GLuint list, GLenum mode);
static void ( APIENTRY * dllNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
static void ( APIENTRY * dllNormal3bv )(const GLbyte *v);
static void ( APIENTRY * dllNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
static void ( APIENTRY * dllNormal3dv )(const GLdouble *v);
static void ( APIENTRY * dllNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
static void ( APIENTRY * dllNormal3fv )(const GLfloat *v);
static void ( APIENTRY * dllNormal3i )(GLint nx, GLint ny, GLint nz);
static void ( APIENTRY * dllNormal3iv )(const GLint *v);
static void ( APIENTRY * dllNormal3s )(GLshort nx, GLshort ny, GLshort nz);
static void ( APIENTRY * dllNormal3sv )(const GLshort *v);
static void ( APIENTRY * dllNormalPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static void ( APIENTRY * dllPassThrough )(GLfloat token);
static void ( APIENTRY * dllPixelMapfv )(GLenum map, GLsizei mapsize, const GLfloat *values);
static void ( APIENTRY * dllPixelMapuiv )(GLenum map, GLsizei mapsize, const GLuint *values);
static void ( APIENTRY * dllPixelMapusv )(GLenum map, GLsizei mapsize, const GLushort *values);
static void ( APIENTRY * dllPixelStoref )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllPixelStorei )(GLenum pname, GLint param);
static void ( APIENTRY * dllPixelTransferf )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllPixelTransferi )(GLenum pname, GLint param);
static void ( APIENTRY * dllPixelZoom )(GLfloat xfactor, GLfloat yfactor);
static void ( APIENTRY * dllPointSize )(GLfloat size);
static void ( APIENTRY * dllPolygonMode )(GLenum face, GLenum mode);
static void ( APIENTRY * dllPolygonOffset )(GLfloat factor, GLfloat units);
static void ( APIENTRY * dllPolygonStipple )(const GLubyte *mask);
static void ( APIENTRY * dllPopAttrib )(void);
static void ( APIENTRY * dllPopClientAttrib )(void);
static void ( APIENTRY * dllPopMatrix )(void);
static void ( APIENTRY * dllPopName )(void);
static void ( APIENTRY * dllPrioritizeTextures )(GLsizei n, const GLuint *textures, const GLclampf *priorities);
static void ( APIENTRY * dllPushAttrib )(GLbitfield mask);
static void ( APIENTRY * dllPushClientAttrib )(GLbitfield mask);
static void ( APIENTRY * dllPushMatrix )(void);
static void ( APIENTRY * dllPushName )(GLuint name);
static void ( APIENTRY * dllRasterPos2d )(GLdouble x, GLdouble y);
static void ( APIENTRY * dllRasterPos2dv )(const GLdouble *v);
static void ( APIENTRY * dllRasterPos2f )(GLfloat x, GLfloat y);
static void ( APIENTRY * dllRasterPos2fv )(const GLfloat *v);
static void ( APIENTRY * dllRasterPos2i )(GLint x, GLint y);
static void ( APIENTRY * dllRasterPos2iv )(const GLint *v);
static void ( APIENTRY * dllRasterPos2s )(GLshort x, GLshort y);
static void ( APIENTRY * dllRasterPos2sv )(const GLshort *v);
static void ( APIENTRY * dllRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllRasterPos3dv )(const GLdouble *v);
static void ( APIENTRY * dllRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllRasterPos3fv )(const GLfloat *v);
static void ( APIENTRY * dllRasterPos3i )(GLint x, GLint y, GLint z);
static void ( APIENTRY * dllRasterPos3iv )(const GLint *v);
static void ( APIENTRY * dllRasterPos3s )(GLshort x, GLshort y, GLshort z);
static void ( APIENTRY * dllRasterPos3sv )(const GLshort *v);
static void ( APIENTRY * dllRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static void ( APIENTRY * dllRasterPos4dv )(const GLdouble *v);
static void ( APIENTRY * dllRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static void ( APIENTRY * dllRasterPos4fv )(const GLfloat *v);
static void ( APIENTRY * dllRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
static void ( APIENTRY * dllRasterPos4iv )(const GLint *v);
static void ( APIENTRY * dllRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
static void ( APIENTRY * dllRasterPos4sv )(const GLshort *v);
static void ( APIENTRY * dllReadBuffer )(GLenum mode);
static void ( APIENTRY * dllReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
static void ( APIENTRY * dllRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
static void ( APIENTRY * dllRectdv )(const GLdouble *v1, const GLdouble *v2);
static void ( APIENTRY * dllRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
static void ( APIENTRY * dllRectfv )(const GLfloat *v1, const GLfloat *v2);
static void ( APIENTRY * dllRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
static void ( APIENTRY * dllRectiv )(const GLint *v1, const GLint *v2);
static void ( APIENTRY * dllRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
static void ( APIENTRY * dllRectsv )(const GLshort *v1, const GLshort *v2);
GLint ( APIENTRY * dllRenderMode )(GLenum mode);
static void ( APIENTRY * dllRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllScaled )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllScalef )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
static void ( APIENTRY * dllSelectBuffer )(GLsizei size, GLuint *buffer);
static void ( APIENTRY * dllShadeModel )(GLenum mode);
static void ( APIENTRY * dllStencilFunc )(GLenum func, GLint ref, GLuint mask);
static void ( APIENTRY * dllStencilMask )(GLuint mask);
static void ( APIENTRY * dllStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
static void ( APIENTRY * dllTexCoord1d )(GLdouble s);
static void ( APIENTRY * dllTexCoord1dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord1f )(GLfloat s);
static void ( APIENTRY * dllTexCoord1fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord1i )(GLint s);
static void ( APIENTRY * dllTexCoord1iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord1s )(GLshort s);
static void ( APIENTRY * dllTexCoord1sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoord2d )(GLdouble s, GLdouble t);
static void ( APIENTRY * dllTexCoord2dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord2f )(GLfloat s, GLfloat t);
static void ( APIENTRY * dllTexCoord2fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord2i )(GLint s, GLint t);
static void ( APIENTRY * dllTexCoord2iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord2s )(GLshort s, GLshort t);
static void ( APIENTRY * dllTexCoord2sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
static void ( APIENTRY * dllTexCoord3dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
static void ( APIENTRY * dllTexCoord3fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord3i )(GLint s, GLint t, GLint r);
static void ( APIENTRY * dllTexCoord3iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord3s )(GLshort s, GLshort t, GLshort r);
static void ( APIENTRY * dllTexCoord3sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
static void ( APIENTRY * dllTexCoord4dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
static void ( APIENTRY * dllTexCoord4fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
static void ( APIENTRY * dllTexCoord4iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
static void ( APIENTRY * dllTexCoord4sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllTexEnvf )(GLenum target, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllTexEnvi )(GLenum target, GLenum pname, GLint param);
static void ( APIENTRY * dllTexEnviv )(GLenum target, GLenum pname, const GLint *params);
static void ( APIENTRY * dllTexGend )(GLenum coord, GLenum pname, GLdouble param);
static void ( APIENTRY * dllTexGendv )(GLenum coord, GLenum pname, const GLdouble *params);
static void ( APIENTRY * dllTexGenf )(GLenum coord, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllTexGeni )(GLenum coord, GLenum pname, GLint param);
static void ( APIENTRY * dllTexGeniv )(GLenum coord, GLenum pname, const GLint *params);
static void ( APIENTRY * dllTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexParameterf )(GLenum target, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllTexParameteri )(GLenum target, GLenum pname, GLint param);
static void ( APIENTRY * dllTexParameteriv )(GLenum target, GLenum pname, const GLint *params);
static void ( APIENTRY * dllTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTranslated )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllTranslatef )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllVertex2d )(GLdouble x, GLdouble y);
static void ( APIENTRY * dllVertex2dv )(const GLdouble *v);
static void ( APIENTRY * dllVertex2f )(GLfloat x, GLfloat y);
static void ( APIENTRY * dllVertex2fv )(const GLfloat *v);
static void ( APIENTRY * dllVertex2i )(GLint x, GLint y);
static void ( APIENTRY * dllVertex2iv )(const GLint *v);
static void ( APIENTRY * dllVertex2s )(GLshort x, GLshort y);
static void ( APIENTRY * dllVertex2sv )(const GLshort *v);
static void ( APIENTRY * dllVertex3d )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllVertex3dv )(const GLdouble *v);
static void ( APIENTRY * dllVertex3f )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllVertex3fv )(const GLfloat *v);
static void ( APIENTRY * dllVertex3i )(GLint x, GLint y, GLint z);
static void ( APIENTRY * dllVertex3iv )(const GLint *v);
static void ( APIENTRY * dllVertex3s )(GLshort x, GLshort y, GLshort z);
static void ( APIENTRY * dllVertex3sv )(const GLshort *v);
static void ( APIENTRY * dllVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static void ( APIENTRY * dllVertex4dv )(const GLdouble *v);
static void ( APIENTRY * dllVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static void ( APIENTRY * dllVertex4fv )(const GLfloat *v);
static void ( APIENTRY * dllVertex4i )(GLint x, GLint y, GLint z, GLint w);
static void ( APIENTRY * dllVertex4iv )(const GLint *v);
static void ( APIENTRY * dllVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
static void ( APIENTRY * dllVertex4sv )(const GLshort *v);
static void ( APIENTRY * dllVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllViewport )(GLint x, GLint y, GLsizei width, GLsizei height);

typedef struct {
	GLenum	e;
	const char *name;
} glEnumName_t;

#define	DEF(x) { x, #x },

glEnumName_t	glEnumNames[] = {
DEF(GL_FALSE)
DEF(GL_TRUE)

	{ GL_BYTE, "GL_BYTE" },
	{ GL_UNSIGNED_BYTE, "GL_UNSIGNED_BYTE" },
	{ GL_SHORT, "GL_SHORT" },
	{ GL_UNSIGNED_SHORT, "GL_UNSIGNED_SHORT" },
	{ GL_INT, "GL_INT" },
	{ GL_UNSIGNED_INT, "GL_UNSIGNED_INT" },
	{ GL_FLOAT, "GL_FLOAT" },
	{ GL_DOUBLE, "GL_DOUBLE" },

	{ GL_TEXTURE_CUBE_MAP_EXT, "GL_TEXTURE_CUBE_MAP_EXT" },
	{ GL_TEXTURE_3D, "GL_TEXTURE_3D" },
	{ GL_TEXTURE_2D, "GL_TEXTURE_2D" },
	{ GL_BLEND, "GL_BLEND" },
	{ GL_DEPTH_TEST, "GL_DEPTH_TEST" },
	{ GL_CULL_FACE, "GL_CULL_FACE" },
	{ GL_CLIP_PLANE0, "GL_CLIP_PLANE0" },
	{ GL_COLOR_ARRAY, "GL_COLOR_ARRAY" },
	{ GL_TEXTURE_COORD_ARRAY, "GL_TEXTURE_COORD_ARRAY" },
	{ GL_VERTEX_ARRAY, "GL_VERTEX_ARRAY" },
	{ GL_ALPHA_TEST, "GL_ALPHA_TEST" },
	{ GL_TEXTURE_GEN_S, "GL_TEXTURE_GEN_S" },
	{ GL_TEXTURE_GEN_T, "GL_TEXTURE_GEN_T" },
	{ GL_TEXTURE_GEN_R, "GL_TEXTURE_GEN_R" },
	{ GL_TEXTURE_GEN_Q, "GL_TEXTURE_GEN_Q" },
	{ GL_STENCIL_TEST, "GL_STENCIL_TEST" },
	{ GL_POLYGON_OFFSET_FILL, "GL_POLYGON_OFFSET_FILL" },

	{ GL_TRIANGLES,	"GL_TRIANGLES" },
	{ GL_TRIANGLE_STRIP, "GL_TRIANGLE_STRIP" },
	{ GL_TRIANGLE_FAN, "GL_TRIANGLE_FAN" },
	{ GL_QUADS, "GL_QUADS" },
	{ GL_QUAD_STRIP, "GL_QUAD_STRIP" },
	{ GL_POLYGON, "GL_POLYGON" },
	{ GL_POINTS, "GL_POINTS" },
	{ GL_LINES, "GL_LINES" },
	{ GL_LINE_STRIP, "GL_LINE_STRIP" },
	{ GL_LINE_LOOP, "GL_LINE_LOOP" },

	{ GL_ALWAYS, "GL_ALWAYS" },
	{ GL_NEVER, "GL_NEVER" },
	{ GL_LEQUAL, "GL_LEQUAL" },
	{ GL_LESS, "GL_LESS" },
	{ GL_EQUAL, "GL_EQUAL" },
	{ GL_GREATER, "GL_GREATER" },
	{ GL_GEQUAL, "GL_GEQUAL" },
	{ GL_NOTEQUAL, "GL_NOTEQUAL" },

	{ GL_ONE, "GL_ONE" },
	{ GL_ZERO, "GL_ZERO" },
	{ GL_SRC_ALPHA, "GL_SRC_ALPHA" },
	{ GL_ONE_MINUS_SRC_ALPHA, "GL_ONE_MINUS_SRC_ALPHA" },
	{ GL_DST_COLOR, "GL_DST_COLOR" },
	{ GL_ONE_MINUS_DST_COLOR, "GL_ONE_MINUS_DST_COLOR" },
	{ GL_DST_ALPHA, "GL_DST_ALPHA" },

	{ GL_MODELVIEW, "GL_MODELVIEW" },
	{ GL_PROJECTION, "GL_PROJECTION" },
	{ GL_TEXTURE, "GL_TEXTURE" },

/* DrawBufferMode */
DEF(GL_NONE)
DEF(GL_FRONT_LEFT)
DEF(GL_FRONT_RIGHT)
DEF(GL_BACK_LEFT)
DEF(GL_BACK_RIGHT)
DEF(GL_FRONT)
DEF(GL_BACK)
DEF(GL_LEFT)
DEF(GL_RIGHT)
DEF(GL_FRONT_AND_BACK)
DEF(GL_AUX0)
DEF(GL_AUX1)
DEF(GL_AUX2)
DEF(GL_AUX3)

/* GetTarget */
DEF(GL_CURRENT_COLOR)
DEF(GL_CURRENT_INDEX)
DEF(GL_CURRENT_NORMAL)
DEF(GL_CURRENT_TEXTURE_COORDS)
DEF(GL_CURRENT_RASTER_COLOR)
DEF(GL_CURRENT_RASTER_INDEX)
DEF(GL_CURRENT_RASTER_TEXTURE_COORDS)
DEF(GL_CURRENT_RASTER_POSITION)
DEF(GL_CURRENT_RASTER_POSITION_VALID)
DEF(GL_CURRENT_RASTER_DISTANCE)
DEF(GL_POINT_SMOOTH)
DEF(GL_POINT_SIZE)
DEF(GL_POINT_SIZE_RANGE)
DEF(GL_POINT_SIZE_GRANULARITY)
DEF(GL_LINE_SMOOTH)
DEF(GL_LINE_WIDTH)
DEF(GL_LINE_WIDTH_RANGE)
DEF(GL_LINE_WIDTH_GRANULARITY)
DEF(GL_LINE_STIPPLE)
DEF(GL_LINE_STIPPLE_PATTERN)
DEF(GL_LINE_STIPPLE_REPEAT)
DEF(GL_LIST_MODE)
DEF(GL_MAX_LIST_NESTING)
DEF(GL_LIST_BASE)
DEF(GL_LIST_INDEX)
DEF(GL_POLYGON_MODE)
DEF(GL_POLYGON_SMOOTH)
DEF(GL_POLYGON_STIPPLE)
DEF(GL_EDGE_FLAG)
DEF(GL_CULL_FACE)
DEF(GL_CULL_FACE_MODE)
DEF(GL_FRONT_FACE)
DEF(GL_LIGHTING)
DEF(GL_LIGHT_MODEL_LOCAL_VIEWER)
DEF(GL_LIGHT_MODEL_TWO_SIDE)
DEF(GL_LIGHT_MODEL_AMBIENT)
DEF(GL_SHADE_MODEL)
DEF(GL_COLOR_MATERIAL_FACE)
DEF(GL_COLOR_MATERIAL_PARAMETER)
DEF(GL_COLOR_MATERIAL)
DEF(GL_FOG)
DEF(GL_FOG_INDEX)
DEF(GL_FOG_DENSITY)
DEF(GL_FOG_START)
DEF(GL_FOG_END)
DEF(GL_FOG_MODE)
DEF(GL_FOG_COLOR)
DEF(GL_DEPTH_RANGE)
DEF(GL_DEPTH_TEST)
DEF(GL_DEPTH_WRITEMASK)
DEF(GL_DEPTH_CLEAR_VALUE)
DEF(GL_DEPTH_FUNC)
DEF(GL_ACCUM_CLEAR_VALUE)
DEF(GL_STENCIL_TEST)
DEF(GL_STENCIL_CLEAR_VALUE)
DEF(GL_STENCIL_FUNC)
DEF(GL_STENCIL_VALUE_MASK)
DEF(GL_STENCIL_FAIL)
DEF(GL_STENCIL_PASS_DEPTH_FAIL)
DEF(GL_STENCIL_PASS_DEPTH_PASS)
DEF(GL_STENCIL_REF)
DEF(GL_STENCIL_WRITEMASK)
DEF(GL_MATRIX_MODE)
DEF(GL_NORMALIZE)
DEF(GL_VIEWPORT)
DEF(GL_MODELVIEW_STACK_DEPTH)
DEF(GL_PROJECTION_STACK_DEPTH)
DEF(GL_TEXTURE_STACK_DEPTH)
DEF(GL_MODELVIEW_MATRIX)
DEF(GL_PROJECTION_MATRIX)
DEF(GL_TEXTURE_MATRIX)
DEF(GL_ATTRIB_STACK_DEPTH)
DEF(GL_CLIENT_ATTRIB_STACK_DEPTH)
DEF(GL_ALPHA_TEST)
DEF(GL_ALPHA_TEST_FUNC)
DEF(GL_ALPHA_TEST_REF)
DEF(GL_DITHER)
DEF(GL_BLEND_DST)
DEF(GL_BLEND_SRC)
DEF(GL_BLEND)
DEF(GL_LOGIC_OP_MODE)
DEF(GL_INDEX_LOGIC_OP)
DEF(GL_COLOR_LOGIC_OP)
DEF(GL_AUX_BUFFERS)
DEF(GL_DRAW_BUFFER)
DEF(GL_READ_BUFFER)
DEF(GL_SCISSOR_BOX)
DEF(GL_SCISSOR_TEST)
DEF(GL_INDEX_CLEAR_VALUE)
DEF(GL_INDEX_WRITEMASK)
DEF(GL_COLOR_CLEAR_VALUE)
DEF(GL_COLOR_WRITEMASK)
DEF(GL_INDEX_MODE)
DEF(GL_RGBA_MODE)
DEF(GL_DOUBLEBUFFER)
DEF(GL_STEREO)
DEF(GL_RENDER_MODE)
DEF(GL_PERSPECTIVE_CORRECTION_HINT)
DEF(GL_POINT_SMOOTH_HINT)
DEF(GL_LINE_SMOOTH_HINT)
DEF(GL_POLYGON_SMOOTH_HINT)
DEF(GL_FOG_HINT)
DEF(GL_TEXTURE_GEN_S)
DEF(GL_TEXTURE_GEN_T)
DEF(GL_TEXTURE_GEN_R)
DEF(GL_TEXTURE_GEN_Q)
DEF(GL_PIXEL_MAP_I_TO_I)
DEF(GL_PIXEL_MAP_S_TO_S)
DEF(GL_PIXEL_MAP_I_TO_R)
DEF(GL_PIXEL_MAP_I_TO_G)
DEF(GL_PIXEL_MAP_I_TO_B)
DEF(GL_PIXEL_MAP_I_TO_A)
DEF(GL_PIXEL_MAP_R_TO_R)
DEF(GL_PIXEL_MAP_G_TO_G)
DEF(GL_PIXEL_MAP_B_TO_B)
DEF(GL_PIXEL_MAP_A_TO_A)
DEF(GL_PIXEL_MAP_I_TO_I_SIZE)
DEF(GL_PIXEL_MAP_S_TO_S_SIZE)
DEF(GL_PIXEL_MAP_I_TO_R_SIZE)
DEF(GL_PIXEL_MAP_I_TO_G_SIZE)
DEF(GL_PIXEL_MAP_I_TO_B_SIZE)
DEF(GL_PIXEL_MAP_I_TO_A_SIZE)
DEF(GL_PIXEL_MAP_R_TO_R_SIZE)
DEF(GL_PIXEL_MAP_G_TO_G_SIZE)
DEF(GL_PIXEL_MAP_B_TO_B_SIZE)
DEF(GL_PIXEL_MAP_A_TO_A_SIZE)
DEF(GL_UNPACK_SWAP_BYTES)
DEF(GL_UNPACK_LSB_FIRST)
DEF(GL_UNPACK_ROW_LENGTH)
DEF(GL_UNPACK_SKIP_ROWS)
DEF(GL_UNPACK_SKIP_PIXELS)
DEF(GL_UNPACK_ALIGNMENT)
DEF(GL_PACK_SWAP_BYTES)
DEF(GL_PACK_LSB_FIRST)
DEF(GL_PACK_ROW_LENGTH)
DEF(GL_PACK_SKIP_ROWS)
DEF(GL_PACK_SKIP_PIXELS)
DEF(GL_PACK_ALIGNMENT)
DEF(GL_MAP_COLOR)
DEF(GL_MAP_STENCIL)
DEF(GL_INDEX_SHIFT)
DEF(GL_INDEX_OFFSET)
DEF(GL_RED_SCALE)
DEF(GL_RED_BIAS)
DEF(GL_ZOOM_X)
DEF(GL_ZOOM_Y)
DEF(GL_GREEN_SCALE)
DEF(GL_GREEN_BIAS)
DEF(GL_BLUE_SCALE)
DEF(GL_BLUE_BIAS)
DEF(GL_ALPHA_SCALE)
DEF(GL_ALPHA_BIAS)
DEF(GL_DEPTH_SCALE)
DEF(GL_DEPTH_BIAS)
DEF(GL_MAX_EVAL_ORDER)
DEF(GL_MAX_LIGHTS)
DEF(GL_MAX_CLIP_PLANES)
DEF(GL_MAX_TEXTURE_SIZE)
DEF(GL_MAX_PIXEL_MAP_TABLE)
DEF(GL_MAX_ATTRIB_STACK_DEPTH)
DEF(GL_MAX_MODELVIEW_STACK_DEPTH)
DEF(GL_MAX_NAME_STACK_DEPTH)
DEF(GL_MAX_PROJECTION_STACK_DEPTH)
DEF(GL_MAX_TEXTURE_STACK_DEPTH)
DEF(GL_MAX_VIEWPORT_DIMS)
DEF(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH)
DEF(GL_SUBPIXEL_BITS)
DEF(GL_INDEX_BITS)
DEF(GL_RED_BITS)
DEF(GL_GREEN_BITS)
DEF(GL_BLUE_BITS)
DEF(GL_ALPHA_BITS)
DEF(GL_DEPTH_BITS)
DEF(GL_STENCIL_BITS)
DEF(GL_ACCUM_RED_BITS)
DEF(GL_ACCUM_GREEN_BITS)
DEF(GL_ACCUM_BLUE_BITS)
DEF(GL_ACCUM_ALPHA_BITS)
DEF(GL_NAME_STACK_DEPTH)
DEF(GL_AUTO_NORMAL)
DEF(GL_MAP1_COLOR_4)
DEF(GL_MAP1_INDEX)
DEF(GL_MAP1_NORMAL)
DEF(GL_MAP1_TEXTURE_COORD_1)
DEF(GL_MAP1_TEXTURE_COORD_2)
DEF(GL_MAP1_TEXTURE_COORD_3)
DEF(GL_MAP1_TEXTURE_COORD_4)
DEF(GL_MAP1_VERTEX_3)
DEF(GL_MAP1_VERTEX_4)
DEF(GL_MAP2_COLOR_4)
DEF(GL_MAP2_INDEX)
DEF(GL_MAP2_NORMAL)
DEF(GL_MAP2_TEXTURE_COORD_1)
DEF(GL_MAP2_TEXTURE_COORD_2)
DEF(GL_MAP2_TEXTURE_COORD_3)
DEF(GL_MAP2_TEXTURE_COORD_4)
DEF(GL_MAP2_VERTEX_3)
DEF(GL_MAP2_VERTEX_4)
DEF(GL_MAP1_GRID_DOMAIN)
DEF(GL_MAP1_GRID_SEGMENTS)
DEF(GL_MAP2_GRID_DOMAIN)
DEF(GL_MAP2_GRID_SEGMENTS)
DEF(GL_TEXTURE_1D)
DEF(GL_TEXTURE_2D)
DEF(GL_FEEDBACK_BUFFER_POINTER)
DEF(GL_FEEDBACK_BUFFER_SIZE)
DEF(GL_FEEDBACK_BUFFER_TYPE)
DEF(GL_SELECTION_BUFFER_POINTER)
DEF(GL_SELECTION_BUFFER_SIZE)

/* PixelCopyType */
DEF(GL_COLOR)
DEF(GL_DEPTH)
DEF(GL_STENCIL)

/* PixelFormat */
DEF(GL_COLOR_INDEX)
DEF(GL_STENCIL_INDEX)
DEF(GL_DEPTH_COMPONENT)
DEF(GL_RED)
DEF(GL_GREEN)
DEF(GL_BLUE)
DEF(GL_ALPHA)
DEF(GL_RGB)
DEF(GL_RGBA)
DEF(GL_LUMINANCE)
DEF(GL_LUMINANCE_ALPHA)

/* PixelMap */
/*      GL_PIXEL_MAP_I_TO_I */
/*      GL_PIXEL_MAP_S_TO_S */
/*      GL_PIXEL_MAP_I_TO_R */
/*      GL_PIXEL_MAP_I_TO_G */
/*      GL_PIXEL_MAP_I_TO_B */
/*      GL_PIXEL_MAP_I_TO_A */
/*      GL_PIXEL_MAP_R_TO_R */
/*      GL_PIXEL_MAP_G_TO_G */
/*      GL_PIXEL_MAP_B_TO_B */
/*      GL_PIXEL_MAP_A_TO_A */

/* PixelStore */
/*      GL_UNPACK_SWAP_BYTES */
/*      GL_UNPACK_LSB_FIRST */
/*      GL_UNPACK_ROW_LENGTH */
/*      GL_UNPACK_SKIP_ROWS */
/*      GL_UNPACK_SKIP_PIXELS */
/*      GL_UNPACK_ALIGNMENT */
/*      GL_PACK_SWAP_BYTES */
/*      GL_PACK_LSB_FIRST */
/*      GL_PACK_ROW_LENGTH */
/*      GL_PACK_SKIP_ROWS */
/*      GL_PACK_SKIP_PIXELS */
/*      GL_PACK_ALIGNMENT */

/* PixelTransfer */
/*      GL_MAP_COLOR */
/*      GL_MAP_STENCIL */
/*      GL_INDEX_SHIFT */
/*      GL_INDEX_OFFSET */
/*      GL_RED_SCALE */
/*      GL_RED_BIAS */
/*      GL_GREEN_SCALE */
/*      GL_GREEN_BIAS */
/*      GL_BLUE_SCALE */
/*      GL_BLUE_BIAS */
/*      GL_ALPHA_SCALE */
/*      GL_ALPHA_BIAS */
/*      GL_DEPTH_SCALE */
/*      GL_DEPTH_BIAS */

/* PixelType */
DEF(GL_BITMAP)
/*      GL_BYTE */
/*      GL_UNSIGNED_BYTE */
/*      GL_SHORT */
/*      GL_UNSIGNED_SHORT */
/*      GL_INT */
/*      GL_UNSIGNED_INT */
/*      GL_FLOAT */

/* PolygonMode */
DEF(GL_POINT)
DEF(GL_LINE)
DEF(GL_FILL)

/* RenderingMode */
DEF(GL_RENDER)
DEF(GL_FEEDBACK)
DEF(GL_SELECT)

/* ShadingModel */
DEF(GL_FLAT)
DEF(GL_SMOOTH)

/* StencilOp */
/*      GL_ZERO */
DEF(GL_KEEP)
DEF(GL_REPLACE)
DEF(GL_INCR)
DEF(GL_DECR)
/*      GL_INVERT */

/* StringName */
DEF(GL_VENDOR)
DEF(GL_RENDERER)
DEF(GL_VERSION)
DEF(GL_EXTENSIONS)

/* TextureCoordName */
DEF(GL_S)
DEF(GL_T)
DEF(GL_R)
DEF(GL_Q)

/* TexCoordPointerType */
/*      GL_SHORT */
/*      GL_INT */
/*      GL_FLOAT */
/*      GL_DOUBLE */

/* TextureEnvMode */
DEF(GL_MODULATE)
DEF(GL_DECAL)
/*      GL_BLEND */
/*      GL_REPLACE */

/* TextureEnvParameter */
DEF(GL_TEXTURE_ENV_MODE)
DEF(GL_TEXTURE_ENV_COLOR)

/* TextureEnvTarget */
DEF(GL_TEXTURE_ENV)

/* TextureGenMode */
DEF(GL_EYE_LINEAR)
DEF(GL_OBJECT_LINEAR)
DEF(GL_SPHERE_MAP)

/* TextureGenParameter */
DEF(GL_TEXTURE_GEN_MODE)
DEF(GL_OBJECT_PLANE)
DEF(GL_EYE_PLANE)

/* TextureMagFilter */
DEF(GL_NEAREST)
DEF(GL_LINEAR)

/* TextureMinFilter */
/*      GL_NEAREST */
/*      GL_LINEAR */
DEF(GL_NEAREST_MIPMAP_NEAREST)
DEF(GL_LINEAR_MIPMAP_NEAREST)
DEF(GL_NEAREST_MIPMAP_LINEAR)
DEF(GL_LINEAR_MIPMAP_LINEAR)

/* TextureParameterName */
DEF(GL_TEXTURE_MAG_FILTER)
DEF(GL_TEXTURE_MIN_FILTER)
DEF(GL_TEXTURE_WRAP_S)
DEF(GL_TEXTURE_WRAP_T)
/*      GL_TEXTURE_BORDER_COLOR */
/*      GL_TEXTURE_PRIORITY */

/* TextureTarget */
/*      GL_TEXTURE_1D */
/*      GL_TEXTURE_2D */
/*      GL_PROXY_TEXTURE_1D */
/*      GL_PROXY_TEXTURE_2D */

/* TextureWrapMode */
DEF(GL_CLAMP)
DEF(GL_REPEAT)

/* VertexPointerType */
/*      GL_SHORT */
/*      GL_INT */
/*      GL_FLOAT */
/*      GL_DOUBLE */

/* ClientAttribMask */
DEF(GL_CLIENT_PIXEL_STORE_BIT)
DEF(GL_CLIENT_VERTEX_ARRAY_BIT)
DEF(GL_CLIENT_ALL_ATTRIB_BITS)

/* polygon_offset */
DEF(GL_POLYGON_OFFSET_FACTOR)
DEF(GL_POLYGON_OFFSET_UNITS)
DEF(GL_POLYGON_OFFSET_POINT)
DEF(GL_POLYGON_OFFSET_LINE)
DEF(GL_POLYGON_OFFSET_FILL)

	{ 0, NULL }
};

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.  This
** is only called during a hard shutdown of the OGL subsystem (e.g. vid_restart).
*/
void QGL_Shutdown( void )
{
	common->Printf( "...shutting down QGL\n" );

	if ( win32.hinstOpenGL )
	{
		common->Printf( "...unloading OpenGL DLL\n" );
		FreeLibrary( win32.hinstOpenGL );
	}

	win32.hinstOpenGL = NULL;

	qglAccum                     = NULL;
	qglAlphaFunc                 = NULL;
	qglAreTexturesResident       = NULL;
	qglArrayElement              = NULL;
	qglBegin                     = NULL;
	qglBindTexture               = NULL;
	qglBitmap                    = NULL;
	qglBlendFunc                 = NULL;
	qglCallList                  = NULL;
	qglCallLists                 = NULL;
	qglClear                     = NULL;
	qglClearAccum                = NULL;
	qglClearColor                = NULL;
	qglClearDepth                = NULL;
	qglClearIndex                = NULL;
	qglClearStencil              = NULL;
	qglClipPlane                 = NULL;
	qglColor3b                   = NULL;
	qglColor3bv                  = NULL;
	qglColor3d                   = NULL;
	qglColor3dv                  = NULL;
	qglColor3f                   = NULL;
	qglColor3fv                  = NULL;
	qglColor3i                   = NULL;
	qglColor3iv                  = NULL;
	qglColor3s                   = NULL;
	qglColor3sv                  = NULL;
	qglColor3ub                  = NULL;
	qglColor3ubv                 = NULL;
	qglColor3ui                  = NULL;
	qglColor3uiv                 = NULL;
	qglColor3us                  = NULL;
	qglColor3usv                 = NULL;
	qglColor4b                   = NULL;
	qglColor4bv                  = NULL;
	qglColor4d                   = NULL;
	qglColor4dv                  = NULL;
	qglColor4f                   = NULL;
	qglColor4fv                  = NULL;
	qglColor4i                   = NULL;
	qglColor4iv                  = NULL;
	qglColor4s                   = NULL;
	qglColor4sv                  = NULL;
	qglColor4ub                  = NULL;
	qglColor4ubv                 = NULL;
	qglColor4ui                  = NULL;
	qglColor4uiv                 = NULL;
	qglColor4us                  = NULL;
	qglColor4usv                 = NULL;
	qglColorMask                 = NULL;
	qglColorMaterial             = NULL;
	qglColorPointer              = NULL;
	qglCopyPixels                = NULL;
	qglCopyTexImage1D            = NULL;
	qglCopyTexImage2D            = NULL;
	qglCopyTexSubImage1D         = NULL;
	qglCopyTexSubImage2D         = NULL;
	qglCullFace                  = NULL;
	qglDeleteLists               = NULL;
	qglDeleteTextures            = NULL;
	qglDepthFunc                 = NULL;
	qglDepthMask                 = NULL;
	qglDepthRange                = NULL;
	qglDisable                   = NULL;
	qglDisableClientState        = NULL;
	qglDrawArrays                = NULL;
	qglDrawBuffer                = NULL;
	qglDrawElements              = NULL;
	qglDrawPixels                = NULL;
	qglEdgeFlag                  = NULL;
	qglEdgeFlagPointer           = NULL;
	qglEdgeFlagv                 = NULL;
	qglEnable                    = NULL;
	qglEnableClientState         = NULL;
	qglEnd                       = NULL;
	qglEndList                   = NULL;
	qglEvalCoord1d               = NULL;
	qglEvalCoord1dv              = NULL;
	qglEvalCoord1f               = NULL;
	qglEvalCoord1fv              = NULL;
	qglEvalCoord2d               = NULL;
	qglEvalCoord2dv              = NULL;
	qglEvalCoord2f               = NULL;
	qglEvalCoord2fv              = NULL;
	qglEvalMesh1                 = NULL;
	qglEvalMesh2                 = NULL;
	qglEvalPoint1                = NULL;
	qglEvalPoint2                = NULL;
	qglFeedbackBuffer            = NULL;
	qglFinish                    = NULL;
	qglFlush                     = NULL;
	qglFogf                      = NULL;
	qglFogfv                     = NULL;
	qglFogi                      = NULL;
	qglFogiv                     = NULL;
	qglFrontFace                 = NULL;
	qglFrustum                   = NULL;
	qglGenLists                  = NULL;
	qglGenTextures               = NULL;
	qglGetBooleanv               = NULL;
	qglGetClipPlane              = NULL;
	qglGetDoublev                = NULL;
	qglGetError                  = NULL;
	qglGetFloatv                 = NULL;
	qglGetIntegerv               = NULL;
	qglGetLightfv                = NULL;
	qglGetLightiv                = NULL;
	qglGetMapdv                  = NULL;
	qglGetMapfv                  = NULL;
	qglGetMapiv                  = NULL;
	qglGetMaterialfv             = NULL;
	qglGetMaterialiv             = NULL;
	qglGetPixelMapfv             = NULL;
	qglGetPixelMapuiv            = NULL;
	qglGetPixelMapusv            = NULL;
	qglGetPointerv               = NULL;
	qglGetPolygonStipple         = NULL;
	qglGetString                 = NULL;
	qglGetTexEnvfv               = NULL;
	qglGetTexEnviv               = NULL;
	qglGetTexGendv               = NULL;
	qglGetTexGenfv               = NULL;
	qglGetTexGeniv               = NULL;
	qglGetTexImage               = NULL;
	qglGetTexLevelParameterfv    = NULL;
	qglGetTexLevelParameteriv    = NULL;
	qglGetTexParameterfv         = NULL;
	qglGetTexParameteriv         = NULL;
	qglHint                      = NULL;
	qglIndexMask                 = NULL;
	qglIndexPointer              = NULL;
	qglIndexd                    = NULL;
	qglIndexdv                   = NULL;
	qglIndexf                    = NULL;
	qglIndexfv                   = NULL;
	qglIndexi                    = NULL;
	qglIndexiv                   = NULL;
	qglIndexs                    = NULL;
	qglIndexsv                   = NULL;
	qglIndexub                   = NULL;
	qglIndexubv                  = NULL;
	qglInitNames                 = NULL;
	qglInterleavedArrays         = NULL;
	qglIsEnabled                 = NULL;
	qglIsList                    = NULL;
	qglIsTexture                 = NULL;
	qglLightModelf               = NULL;
	qglLightModelfv              = NULL;
	qglLightModeli               = NULL;
	qglLightModeliv              = NULL;
	qglLightf                    = NULL;
	qglLightfv                   = NULL;
	qglLighti                    = NULL;
	qglLightiv                   = NULL;
	qglLineStipple               = NULL;
	qglLineWidth                 = NULL;
	qglListBase                  = NULL;
	qglLoadIdentity              = NULL;
	qglLoadMatrixd               = NULL;
	qglLoadMatrixf               = NULL;
	qglLoadName                  = NULL;
	qglLogicOp                   = NULL;
	qglMap1d                     = NULL;
	qglMap1f                     = NULL;
	qglMap2d                     = NULL;
	qglMap2f                     = NULL;
	qglMapGrid1d                 = NULL;
	qglMapGrid1f                 = NULL;
	qglMapGrid2d                 = NULL;
	qglMapGrid2f                 = NULL;
	qglMaterialf                 = NULL;
	qglMaterialfv                = NULL;
	qglMateriali                 = NULL;
	qglMaterialiv                = NULL;
	qglMatrixMode                = NULL;
	qglMultMatrixd               = NULL;
	qglMultMatrixf               = NULL;
	qglNewList                   = NULL;
	qglNormal3b                  = NULL;
	qglNormal3bv                 = NULL;
	qglNormal3d                  = NULL;
	qglNormal3dv                 = NULL;
	qglNormal3f                  = NULL;
	qglNormal3fv                 = NULL;
	qglNormal3i                  = NULL;
	qglNormal3iv                 = NULL;
	qglNormal3s                  = NULL;
	qglNormal3sv                 = NULL;
	qglNormalPointer             = NULL;
	qglOrtho                     = NULL;
	qglPassThrough               = NULL;
	qglPixelMapfv                = NULL;
	qglPixelMapuiv               = NULL;
	qglPixelMapusv               = NULL;
	qglPixelStoref               = NULL;
	qglPixelStorei               = NULL;
	qglPixelTransferf            = NULL;
	qglPixelTransferi            = NULL;
	qglPixelZoom                 = NULL;
	qglPointSize                 = NULL;
	qglPolygonMode               = NULL;
	qglPolygonOffset             = NULL;
	qglPolygonStipple            = NULL;
	qglPopAttrib                 = NULL;
	qglPopClientAttrib           = NULL;
	qglPopMatrix                 = NULL;
	qglPopName                   = NULL;
	qglPrioritizeTextures        = NULL;
	qglPushAttrib                = NULL;
	qglPushClientAttrib          = NULL;
	qglPushMatrix                = NULL;
	qglPushName                  = NULL;
	qglRasterPos2d               = NULL;
	qglRasterPos2dv              = NULL;
	qglRasterPos2f               = NULL;
	qglRasterPos2fv              = NULL;
	qglRasterPos2i               = NULL;
	qglRasterPos2iv              = NULL;
	qglRasterPos2s               = NULL;
	qglRasterPos2sv              = NULL;
	qglRasterPos3d               = NULL;
	qglRasterPos3dv              = NULL;
	qglRasterPos3f               = NULL;
	qglRasterPos3fv              = NULL;
	qglRasterPos3i               = NULL;
	qglRasterPos3iv              = NULL;
	qglRasterPos3s               = NULL;
	qglRasterPos3sv              = NULL;
	qglRasterPos4d               = NULL;
	qglRasterPos4dv              = NULL;
	qglRasterPos4f               = NULL;
	qglRasterPos4fv              = NULL;
	qglRasterPos4i               = NULL;
	qglRasterPos4iv              = NULL;
	qglRasterPos4s               = NULL;
	qglRasterPos4sv              = NULL;
	qglReadBuffer                = NULL;
	qglReadPixels                = NULL;
	qglRectd                     = NULL;
	qglRectdv                    = NULL;
	qglRectf                     = NULL;
	qglRectfv                    = NULL;
	qglRecti                     = NULL;
	qglRectiv                    = NULL;
	qglRects                     = NULL;
	qglRectsv                    = NULL;
	qglRenderMode                = NULL;
	qglRotated                   = NULL;
	qglRotatef                   = NULL;
	qglScaled                    = NULL;
	qglScalef                    = NULL;
	qglScissor                   = NULL;
	qglSelectBuffer              = NULL;
	qglShadeModel                = NULL;
	qglStencilFunc               = NULL;
	qglStencilMask               = NULL;
	qglStencilOp                 = NULL;
	qglTexCoord1d                = NULL;
	qglTexCoord1dv               = NULL;
	qglTexCoord1f                = NULL;
	qglTexCoord1fv               = NULL;
	qglTexCoord1i                = NULL;
	qglTexCoord1iv               = NULL;
	qglTexCoord1s                = NULL;
	qglTexCoord1sv               = NULL;
	qglTexCoord2d                = NULL;
	qglTexCoord2dv               = NULL;
	qglTexCoord2f                = NULL;
	qglTexCoord2fv               = NULL;
	qglTexCoord2i                = NULL;
	qglTexCoord2iv               = NULL;
	qglTexCoord2s                = NULL;
	qglTexCoord2sv               = NULL;
	qglTexCoord3d                = NULL;
	qglTexCoord3dv               = NULL;
	qglTexCoord3f                = NULL;
	qglTexCoord3fv               = NULL;
	qglTexCoord3i                = NULL;
	qglTexCoord3iv               = NULL;
	qglTexCoord3s                = NULL;
	qglTexCoord3sv               = NULL;
	qglTexCoord4d                = NULL;
	qglTexCoord4dv               = NULL;
	qglTexCoord4f                = NULL;
	qglTexCoord4fv               = NULL;
	qglTexCoord4i                = NULL;
	qglTexCoord4iv               = NULL;
	qglTexCoord4s                = NULL;
	qglTexCoord4sv               = NULL;
	qglTexCoordPointer           = NULL;
	qglTexEnvf                   = NULL;
	qglTexEnvfv                  = NULL;
	qglTexEnvi                   = NULL;
	qglTexEnviv                  = NULL;
	qglTexGend                   = NULL;
	qglTexGendv                  = NULL;
	qglTexGenf                   = NULL;
	qglTexGenfv                  = NULL;
	qglTexGeni                   = NULL;
	qglTexGeniv                  = NULL;
	qglTexImage1D                = NULL;
	qglTexImage2D                = NULL;
	qglTexParameterf             = NULL;
	qglTexParameterfv            = NULL;
	qglTexParameteri             = NULL;
	qglTexParameteriv            = NULL;
	qglTexSubImage1D             = NULL;
	qglTexSubImage2D             = NULL;
	qglTranslated                = NULL;
	qglTranslatef                = NULL;
	qglVertex2d                  = NULL;
	qglVertex2dv                 = NULL;
	qglVertex2f                  = NULL;
	qglVertex2fv                 = NULL;
	qglVertex2i                  = NULL;
	qglVertex2iv                 = NULL;
	qglVertex2s                  = NULL;
	qglVertex2sv                 = NULL;
	qglVertex3d                  = NULL;
	qglVertex3dv                 = NULL;
	qglVertex3f                  = NULL;
	qglVertex3fv                 = NULL;
	qglVertex3i                  = NULL;
	qglVertex3iv                 = NULL;
	qglVertex3s                  = NULL;
	qglVertex3sv                 = NULL;
	qglVertex4d                  = NULL;
	qglVertex4dv                 = NULL;
	qglVertex4f                  = NULL;
	qglVertex4fv                 = NULL;
	qglVertex4i                  = NULL;
	qglVertex4iv                 = NULL;
	qglVertex4s                  = NULL;
	qglVertex4sv                 = NULL;
	qglVertexPointer             = NULL;
	qglViewport                  = NULL;

	qwglCopyContext              = NULL;
	qwglCreateContext            = NULL;
	qwglCreateLayerContext       = NULL;
	qwglDeleteContext            = NULL;
	qwglDescribeLayerPlane       = NULL;
	qwglGetCurrentContext        = NULL;
	qwglGetCurrentDC             = NULL;
	qwglGetLayerPaletteEntries   = NULL;
	qwglGetProcAddress           = NULL;
	qwglMakeCurrent              = NULL;
	qwglRealizeLayerPalette      = NULL;
	qwglSetLayerPaletteEntries   = NULL;
	qwglShareLists               = NULL;
	qwglSwapLayerBuffers         = NULL;
	qwglUseFontBitmaps           = NULL;
	qwglUseFontOutlines          = NULL;

	qwglChoosePixelFormat        = NULL;
	qwglDescribePixelFormat      = NULL;
	qwglGetPixelFormat           = NULL;
	qwglSetPixelFormat           = NULL;
	qwglSwapBuffers              = NULL;
}

#define GR_NUM_BOARDS 0x0f


#pragma warning (disable : 4113 4133 4047 )
#define GPA( a ) GetProcAddress( win32.hinstOpenGL, a )

/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to 
** the appropriate GL stuff.  In Windows this means doing a 
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
*/
bool QGL_Init( const char *dllname )
{
	assert( win32.hinstOpenGL == 0 );

	common->Printf( "...initializing QGL\n" );

	common->Printf( "...calling LoadLibrary( '%s' ): ", dllname );

	if ( ( win32.hinstOpenGL = LoadLibrary( dllname ) ) == 0 )
	{
		common->Printf( "failed\n" );
		return false;
	}
	common->Printf( "succeeded\n" );

	qglAccum                     = dllAccum = glAccum;
	qglAlphaFunc                 = dllAlphaFunc = glAlphaFunc;
	qglAreTexturesResident       = dllAreTexturesResident = glAreTexturesResident;
	qglArrayElement              = dllArrayElement = glArrayElement;
	qglBegin                     = dllBegin = glBegin;
	qglBindTexture               = dllBindTexture = glBindTexture;
	qglBitmap                    = dllBitmap = glBitmap;
	qglBlendFunc                 = dllBlendFunc = glBlendFunc;
	qglCallList                  = dllCallList = glCallList;
	qglCallLists                 = dllCallLists = glCallLists;
	qglClear                     = dllClear = glClear;
	qglClearAccum                = dllClearAccum = glClearAccum;
	qglClearColor                = dllClearColor = glClearColor;
	qglClearDepth                = dllClearDepth = glClearDepth;
	qglClearIndex                = dllClearIndex = glClearIndex;
	qglClearStencil              = dllClearStencil = glClearStencil;
	qglClipPlane                 = dllClipPlane = glClipPlane;
	qglColor3b                   = dllColor3b = glColor3b;
	qglColor3bv                  = dllColor3bv = glColor3bv;
	qglColor3d                   = dllColor3d = glColor3d;
	qglColor3dv                  = dllColor3dv = glColor3dv;
	qglColor3f                   = dllColor3f = glColor3f;
	qglColor3fv                  = dllColor3fv = glColor3fv;
	qglColor3i                   = dllColor3i = glColor3i;
	qglColor3iv                  = dllColor3iv = glColor3iv;
	qglColor3s                   = dllColor3s = glColor3s;
	qglColor3sv                  = dllColor3sv = glColor3sv;
	qglColor3ub                  = dllColor3ub = glColor3ub;
	qglColor3ubv                 = dllColor3ubv = glColor3ubv;
	qglColor3ui                  = dllColor3ui = glColor3ui;
	qglColor3uiv                 = dllColor3uiv = glColor3uiv;
	qglColor3us                  = dllColor3us = glColor3us;
	qglColor3usv                 = dllColor3usv = glColor3usv;
	qglColor4b                   = dllColor4b = glColor4b;
	qglColor4bv                  = dllColor4bv = glColor4bv;
	qglColor4d                   = dllColor4d = glColor4d;
	qglColor4dv                  = dllColor4dv = glColor4dv;
	qglColor4f                   = dllColor4f = glColor4f;
	qglColor4fv                  = dllColor4fv = glColor4fv;
	qglColor4i                   = dllColor4i = glColor4i;
	qglColor4iv                  = dllColor4iv = glColor4iv;
	qglColor4s                   = dllColor4s = glColor4s;
	qglColor4sv                  = dllColor4sv = glColor4sv;
	qglColor4ub                  = dllColor4ub = glColor4ub;
	qglColor4ubv                 = dllColor4ubv = glColor4ubv;
	qglColor4ui                  = dllColor4ui = glColor4ui;
	qglColor4uiv                 = dllColor4uiv = glColor4uiv;
	qglColor4us                  = dllColor4us = glColor4us;
	qglColor4usv                 = dllColor4usv = glColor4usv;
	qglColorMask                 = dllColorMask = glColorMask;
	qglColorMaterial             = dllColorMaterial = glColorMaterial;
	qglColorPointer              = dllColorPointer = glColorPointer;
	qglCopyPixels                = dllCopyPixels = glCopyPixels;
	qglCopyTexImage1D            = dllCopyTexImage1D = glCopyTexImage1D;
	qglCopyTexImage2D            = dllCopyTexImage2D = glCopyTexImage2D;
	qglCopyTexSubImage1D         = dllCopyTexSubImage1D = glCopyTexSubImage1D;
	qglCopyTexSubImage2D         = dllCopyTexSubImage2D = glCopyTexSubImage2D;
	qglCullFace                  = dllCullFace = glCullFace;
	qglDeleteLists               = dllDeleteLists = glDeleteLists;
	qglDeleteTextures            = dllDeleteTextures = glDeleteTextures;
	qglDepthFunc                 = dllDepthFunc = glDepthFunc;
	qglDepthMask                 = dllDepthMask = glDepthMask;
	qglDepthRange                = dllDepthRange = glDepthRange;
	qglDisable                   = dllDisable = glDisable;
	qglDisableClientState        = dllDisableClientState = glDisableClientState;
	qglDrawArrays                = dllDrawArrays = glDrawArrays;
	qglDrawBuffer                = dllDrawBuffer = glDrawBuffer;
	qglDrawElements              = dllDrawElements = glDrawElements;
	qglDrawPixels                = dllDrawPixels = glDrawPixels;
	qglEdgeFlag                  = dllEdgeFlag = glEdgeFlag;
	qglEdgeFlagPointer           = dllEdgeFlagPointer = glEdgeFlagPointer;
	qglEdgeFlagv                 = dllEdgeFlagv = glEdgeFlagv;
	qglEnable                    = 	dllEnable                    = glEnable;
	qglEnableClientState         = 	dllEnableClientState         = glEnableClientState;
	qglEnd                       = 	dllEnd                       = glEnd;
	qglEndList                   = 	dllEndList                   = glEndList;
	qglEvalCoord1d				 = 	dllEvalCoord1d				 = glEvalCoord1d;
	qglEvalCoord1dv              = 	dllEvalCoord1dv              = glEvalCoord1dv;
	qglEvalCoord1f               = 	dllEvalCoord1f               = glEvalCoord1f;
	qglEvalCoord1fv              = 	dllEvalCoord1fv              = glEvalCoord1fv;
	qglEvalCoord2d               = 	dllEvalCoord2d               = glEvalCoord2d;
	qglEvalCoord2dv              = 	dllEvalCoord2dv              = glEvalCoord2dv;
	qglEvalCoord2f               = 	dllEvalCoord2f               = glEvalCoord2f;
	qglEvalCoord2fv              = 	dllEvalCoord2fv              = glEvalCoord2fv;
	qglEvalMesh1                 = 	dllEvalMesh1                 = glEvalMesh1;
	qglEvalMesh2                 = 	dllEvalMesh2                 = glEvalMesh2;
	qglEvalPoint1                = 	dllEvalPoint1                = glEvalPoint1;
	qglEvalPoint2                = 	dllEvalPoint2                = glEvalPoint2;
	qglFeedbackBuffer            = 	dllFeedbackBuffer            = glFeedbackBuffer;
	qglFinish                    = 	dllFinish                    = glFinish;
	qglFlush                     = 	dllFlush                     = glFlush;
	qglFogf                      = 	dllFogf                      = glFogf;
	qglFogfv                     = 	dllFogfv                     = glFogfv;
	qglFogi                      = 	dllFogi                      = glFogi;
	qglFogiv                     = 	dllFogiv                     = glFogiv;
	qglFrontFace                 = 	dllFrontFace                 = glFrontFace;
	qglFrustum                   = 	dllFrustum                   = glFrustum;
	qglGenLists                  = 	dllGenLists                  = ( GLuint (__stdcall * )(int) ) glGenLists;
	qglGenTextures               = 	dllGenTextures               = glGenTextures;
	qglGetBooleanv               = 	dllGetBooleanv               = glGetBooleanv;
	qglGetClipPlane              = 	dllGetClipPlane              = glGetClipPlane;
	qglGetDoublev                = 	dllGetDoublev                = glGetDoublev;
	qglGetError                  = 	dllGetError                  = ( GLenum (__stdcall * )(void) ) glGetError;
	qglGetFloatv                 = 	dllGetFloatv                 = glGetFloatv;
	qglGetIntegerv               = 	dllGetIntegerv               = glGetIntegerv;
	qglGetLightfv                = 	dllGetLightfv                = glGetLightfv;
	qglGetLightiv                = 	dllGetLightiv                = glGetLightiv;
	qglGetMapdv                  = 	dllGetMapdv                  = glGetMapdv;
	qglGetMapfv                  = 	dllGetMapfv                  = glGetMapfv;
	qglGetMapiv                  = 	dllGetMapiv                  = glGetMapiv;
	qglGetMaterialfv             = 	dllGetMaterialfv             = glGetMaterialfv;
	qglGetMaterialiv             = 	dllGetMaterialiv             = glGetMaterialiv;
	qglGetPixelMapfv             = 	dllGetPixelMapfv             = glGetPixelMapfv;
	qglGetPixelMapuiv            = 	dllGetPixelMapuiv            = glGetPixelMapuiv;
	qglGetPixelMapusv            = 	dllGetPixelMapusv            = glGetPixelMapusv;
	qglGetPointerv               = 	dllGetPointerv               = glGetPointerv;
	qglGetPolygonStipple         = 	dllGetPolygonStipple         = glGetPolygonStipple;
	qglGetString                 = 	dllGetString                 = glGetString;
	qglGetTexEnvfv               = 	dllGetTexEnvfv               = glGetTexEnvfv;
	qglGetTexEnviv               = 	dllGetTexEnviv               = glGetTexEnviv;
	qglGetTexGendv               = 	dllGetTexGendv               = glGetTexGendv;
	qglGetTexGenfv               = 	dllGetTexGenfv               = glGetTexGenfv;
	qglGetTexGeniv               = 	dllGetTexGeniv               = glGetTexGeniv;
	qglGetTexImage               = 	dllGetTexImage               = glGetTexImage;
	qglGetTexLevelParameterfv    = 	dllGetTexLevelParameterfv    = glGetTexLevelParameterfv;
	qglGetTexLevelParameteriv    = 	dllGetTexLevelParameteriv    = glGetTexLevelParameteriv;
	qglGetTexParameterfv         = 	dllGetTexParameterfv         = glGetTexParameterfv;
	qglGetTexParameteriv         = 	dllGetTexParameteriv         = glGetTexParameteriv;
	qglHint                      = 	dllHint                      = glHint;
	qglIndexMask                 = 	dllIndexMask                 = glIndexMask;
	qglIndexPointer              = 	dllIndexPointer              = glIndexPointer;
	qglIndexd                    = 	dllIndexd                    = glIndexd;
	qglIndexdv                   = 	dllIndexdv                   = glIndexdv;
	qglIndexf                    = 	dllIndexf                    = glIndexf;
	qglIndexfv                   = 	dllIndexfv                   = glIndexfv;
	qglIndexi                    = 	dllIndexi                    = glIndexi;
	qglIndexiv                   = 	dllIndexiv                   = glIndexiv;
	qglIndexs                    = 	dllIndexs                    = glIndexs;
	qglIndexsv                   = 	dllIndexsv                   = glIndexsv;
	qglIndexub                   = 	dllIndexub                   = glIndexub;
	qglIndexubv                  = 	dllIndexubv                  = glIndexubv;
	qglInitNames                 = 	dllInitNames                 = glInitNames;
	qglInterleavedArrays         = 	dllInterleavedArrays         = glInterleavedArrays;
	qglIsEnabled                 = 	dllIsEnabled                 = glIsEnabled;
	qglIsList                    = 	dllIsList                    = glIsList;
	qglIsTexture                 = 	dllIsTexture                 = glIsTexture;
	qglLightModelf               = 	dllLightModelf               = glLightModelf;
	qglLightModelfv              = 	dllLightModelfv              = glLightModelfv;
	qglLightModeli               = 	dllLightModeli               = glLightModeli;
	qglLightModeliv              = 	dllLightModeliv              = glLightModeliv;
	qglLightf                    = 	dllLightf                    = glLightf;
	qglLightfv                   = 	dllLightfv                   = glLightfv;
	qglLighti                    = 	dllLighti                    = glLighti;
	qglLightiv                   = 	dllLightiv                   = glLightiv;
	qglLineStipple               = 	dllLineStipple               = glLineStipple;
	qglLineWidth                 = 	dllLineWidth                 = glLineWidth;
	qglListBase                  = 	dllListBase                  = glListBase;
	qglLoadIdentity              = 	dllLoadIdentity              = glLoadIdentity;
	qglLoadMatrixd               = 	dllLoadMatrixd               = glLoadMatrixd;
	qglLoadMatrixf               = 	dllLoadMatrixf               = glLoadMatrixf;
	qglLoadName                  = 	dllLoadName                  = glLoadName;
	qglLogicOp                   = 	dllLogicOp                   = glLogicOp;
	qglMap1d                     = 	dllMap1d                     = glMap1d;
	qglMap1f                     = 	dllMap1f                     = glMap1f;
	qglMap2d                     = 	dllMap2d                     = glMap2d;
	qglMap2f                     = 	dllMap2f                     = glMap2f;
	qglMapGrid1d                 = 	dllMapGrid1d                 = glMapGrid1d;
	qglMapGrid1f                 = 	dllMapGrid1f                 = glMapGrid1f;
	qglMapGrid2d                 = 	dllMapGrid2d                 = glMapGrid2d;
	qglMapGrid2f                 = 	dllMapGrid2f                 = glMapGrid2f;
	qglMaterialf                 = 	dllMaterialf                 = glMaterialf;
	qglMaterialfv                = 	dllMaterialfv                = glMaterialfv;
	qglMateriali                 = 	dllMateriali                 = glMateriali;
	qglMaterialiv                = 	dllMaterialiv                = glMaterialiv;
	qglMatrixMode                = 	dllMatrixMode                = glMatrixMode;
	qglMultMatrixd               = 	dllMultMatrixd               = glMultMatrixd;
	qglMultMatrixf               = 	dllMultMatrixf               = glMultMatrixf;
	qglNewList                   = 	dllNewList                   = glNewList;
	qglNormal3b                  = 	dllNormal3b                  = glNormal3b;
	qglNormal3bv                 = 	dllNormal3bv                 = glNormal3bv;
	qglNormal3d                  = 	dllNormal3d                  = glNormal3d;
	qglNormal3dv                 = 	dllNormal3dv                 = glNormal3dv;
	qglNormal3f                  = 	dllNormal3f                  = glNormal3f;
	qglNormal3fv                 = 	dllNormal3fv                 = glNormal3fv;
	qglNormal3i                  = 	dllNormal3i                  = glNormal3i;
	qglNormal3iv                 = 	dllNormal3iv                 = glNormal3iv;
	qglNormal3s                  = 	dllNormal3s                  = glNormal3s;
	qglNormal3sv                 = 	dllNormal3sv                 = glNormal3sv;
	qglNormalPointer             = 	dllNormalPointer             = glNormalPointer;
	qglOrtho                     = 	dllOrtho                     = glOrtho;
	qglPassThrough               = 	dllPassThrough               = glPassThrough;
	qglPixelMapfv                = 	dllPixelMapfv                = glPixelMapfv;
	qglPixelMapuiv               = 	dllPixelMapuiv               = glPixelMapuiv;
	qglPixelMapusv               = 	dllPixelMapusv               = glPixelMapusv;
	qglPixelStoref               = 	dllPixelStoref               = glPixelStoref;
	qglPixelStorei               = 	dllPixelStorei               = glPixelStorei;
	qglPixelTransferf            = 	dllPixelTransferf            = glPixelTransferf;
	qglPixelTransferi            = 	dllPixelTransferi            = glPixelTransferi;
	qglPixelZoom                 = 	dllPixelZoom                 = glPixelZoom;
	qglPointSize                 = 	dllPointSize                 = glPointSize;
	qglPolygonMode               = 	dllPolygonMode               = glPolygonMode;
	qglPolygonOffset             = 	dllPolygonOffset             = glPolygonOffset;
	qglPolygonStipple            = 	dllPolygonStipple            = glPolygonStipple;
	qglPopAttrib                 = 	dllPopAttrib                 = glPopAttrib;
	qglPopClientAttrib           = 	dllPopClientAttrib           = glPopClientAttrib;
	qglPopMatrix                 = 	dllPopMatrix                 = glPopMatrix;
	qglPopName                   = 	dllPopName                   = glPopName;
	qglPrioritizeTextures        = 	dllPrioritizeTextures        = glPrioritizeTextures;
	qglPushAttrib                = 	dllPushAttrib                = glPushAttrib;
	qglPushClientAttrib          = 	dllPushClientAttrib          = glPushClientAttrib;
	qglPushMatrix                = 	dllPushMatrix                = glPushMatrix;
	qglPushName                  = 	dllPushName                  = glPushName;
	qglRasterPos2d               = 	dllRasterPos2d               = glRasterPos2d;
	qglRasterPos2dv              = 	dllRasterPos2dv              = glRasterPos2dv;
	qglRasterPos2f               = 	dllRasterPos2f               = glRasterPos2f;
	qglRasterPos2fv              = 	dllRasterPos2fv              = glRasterPos2fv;
	qglRasterPos2i               = 	dllRasterPos2i               = glRasterPos2i;
	qglRasterPos2iv              = 	dllRasterPos2iv              = glRasterPos2iv;
	qglRasterPos2s               = 	dllRasterPos2s               = glRasterPos2s;
	qglRasterPos2sv              = 	dllRasterPos2sv              = glRasterPos2sv;
	qglRasterPos3d               = 	dllRasterPos3d               = glRasterPos3d;
	qglRasterPos3dv              = 	dllRasterPos3dv              = glRasterPos3dv;
	qglRasterPos3f               = 	dllRasterPos3f               = glRasterPos3f;
	qglRasterPos3fv              = 	dllRasterPos3fv              = glRasterPos3fv;
	qglRasterPos3i               = 	dllRasterPos3i               = glRasterPos3i;
	qglRasterPos3iv              = 	dllRasterPos3iv              = glRasterPos3iv;
	qglRasterPos3s               = 	dllRasterPos3s               = glRasterPos3s;
	qglRasterPos3sv              = 	dllRasterPos3sv              = glRasterPos3sv;
	qglRasterPos4d               = 	dllRasterPos4d               = glRasterPos4d;
	qglRasterPos4dv              = 	dllRasterPos4dv              = glRasterPos4dv;
	qglRasterPos4f               = 	dllRasterPos4f               = glRasterPos4f;
	qglRasterPos4fv              = 	dllRasterPos4fv              = glRasterPos4fv;
	qglRasterPos4i               = 	dllRasterPos4i               = glRasterPos4i;
	qglRasterPos4iv              = 	dllRasterPos4iv              = glRasterPos4iv;
	qglRasterPos4s               = 	dllRasterPos4s               = glRasterPos4s;
	qglRasterPos4sv              = 	dllRasterPos4sv              = glRasterPos4sv;
	qglReadBuffer                = 	dllReadBuffer                = glReadBuffer;
	qglReadPixels                = 	dllReadPixels                = glReadPixels;
	qglRectd                     = 	dllRectd                     = glRectd;
	qglRectdv                    = 	dllRectdv                    = glRectdv;
	qglRectf                     = 	dllRectf                     = glRectf;
	qglRectfv                    = 	dllRectfv                    = glRectfv;
	qglRecti                     = 	dllRecti                     = glRecti;
	qglRectiv                    = 	dllRectiv                    = glRectiv;
	qglRects                     = 	dllRects                     = glRects;
	qglRectsv                    = 	dllRectsv                    = glRectsv;
	qglRenderMode                = 	dllRenderMode                = glRenderMode;
	qglRotated                   = 	dllRotated                   = glRotated;
	qglRotatef                   = 	dllRotatef                   = glRotatef;
	qglScaled                    = 	dllScaled                    = glScaled;
	qglScalef                    = 	dllScalef                    = glScalef;
	qglScissor                   = 	dllScissor                   = glScissor;
	qglSelectBuffer              = 	dllSelectBuffer              = glSelectBuffer;
	qglShadeModel                = 	dllShadeModel                = glShadeModel;
	qglStencilFunc               = 	dllStencilFunc               = glStencilFunc;
	qglStencilMask               = 	dllStencilMask               = glStencilMask;
	qglStencilOp                 = 	dllStencilOp                 = glStencilOp;
	qglTexCoord1d                = 	dllTexCoord1d                = glTexCoord1d;
	qglTexCoord1dv               = 	dllTexCoord1dv               = glTexCoord1dv;
	qglTexCoord1f                = 	dllTexCoord1f                = glTexCoord1f;
	qglTexCoord1fv               = 	dllTexCoord1fv               = glTexCoord1fv;
	qglTexCoord1i                = 	dllTexCoord1i                = glTexCoord1i;
	qglTexCoord1iv               = 	dllTexCoord1iv               = glTexCoord1iv;
	qglTexCoord1s                = 	dllTexCoord1s                = glTexCoord1s;
	qglTexCoord1sv               = 	dllTexCoord1sv               = glTexCoord1sv;
	qglTexCoord2d                = 	dllTexCoord2d                = glTexCoord2d;
	qglTexCoord2dv               = 	dllTexCoord2dv               = glTexCoord2dv;
	qglTexCoord2f                = 	dllTexCoord2f                = glTexCoord2f;
	qglTexCoord2fv               = 	dllTexCoord2fv               = glTexCoord2fv;
	qglTexCoord2i                = 	dllTexCoord2i                = glTexCoord2i;
	qglTexCoord2iv               = 	dllTexCoord2iv               = glTexCoord2iv;
	qglTexCoord2s                = 	dllTexCoord2s                = glTexCoord2s;
	qglTexCoord2sv               = 	dllTexCoord2sv               = glTexCoord2sv;
	qglTexCoord3d                = 	dllTexCoord3d                = glTexCoord3d;
	qglTexCoord3dv               = 	dllTexCoord3dv               = glTexCoord3dv;
	qglTexCoord3f                = 	dllTexCoord3f                = glTexCoord3f;
	qglTexCoord3fv               = 	dllTexCoord3fv               = glTexCoord3fv;
	qglTexCoord3i                = 	dllTexCoord3i                = glTexCoord3i;
	qglTexCoord3iv               = 	dllTexCoord3iv               = glTexCoord3iv;
	qglTexCoord3s                = 	dllTexCoord3s                = glTexCoord3s;
	qglTexCoord3sv               = 	dllTexCoord3sv               = glTexCoord3sv;
	qglTexCoord4d                = 	dllTexCoord4d                = glTexCoord4d;
	qglTexCoord4dv               = 	dllTexCoord4dv               = glTexCoord4dv;
	qglTexCoord4f                = 	dllTexCoord4f                = glTexCoord4f;
	qglTexCoord4fv               = 	dllTexCoord4fv               = glTexCoord4fv;
	qglTexCoord4i                = 	dllTexCoord4i                = glTexCoord4i;
	qglTexCoord4iv               = 	dllTexCoord4iv               = glTexCoord4iv;
	qglTexCoord4s                = 	dllTexCoord4s                = glTexCoord4s;
	qglTexCoord4sv               = 	dllTexCoord4sv               = glTexCoord4sv;
	qglTexCoordPointer           = 	dllTexCoordPointer           = glTexCoordPointer;
	qglTexEnvf                   = 	dllTexEnvf                   = glTexEnvf;
	qglTexEnvfv                  = 	dllTexEnvfv                  = glTexEnvfv;
	qglTexEnvi                   = 	dllTexEnvi                   = glTexEnvi;
	qglTexEnviv                  = 	dllTexEnviv                  = glTexEnviv;
	qglTexGend                   = 	dllTexGend                   = glTexGend;
	qglTexGendv                  = 	dllTexGendv                  = glTexGendv;
	qglTexGenf                   = 	dllTexGenf                   = glTexGenf;
	qglTexGenfv                  = 	dllTexGenfv                  = glTexGenfv;
	qglTexGeni                   = 	dllTexGeni                   = glTexGeni;
	qglTexGeniv                  = 	dllTexGeniv                  = glTexGeniv;
	qglTexImage1D                = 	dllTexImage1D                = glTexImage1D;
	qglTexImage2D                = 	dllTexImage2D                = glTexImage2D;
	qglTexParameterf             = 	dllTexParameterf             = glTexParameterf;
	qglTexParameterfv            = 	dllTexParameterfv            = glTexParameterfv;
	qglTexParameteri             = 	dllTexParameteri             = glTexParameteri;
	qglTexParameteriv            = 	dllTexParameteriv            = glTexParameteriv;
	qglTexSubImage1D             = 	dllTexSubImage1D             = glTexSubImage1D;
	qglTexSubImage2D             = 	dllTexSubImage2D             = glTexSubImage2D;
	qglTranslated                = 	dllTranslated                = glTranslated;
	qglTranslatef                = 	dllTranslatef                = glTranslatef;
	qglVertex2d                  = 	dllVertex2d                  = glVertex2d;
	qglVertex2dv                 = 	dllVertex2dv                 = glVertex2dv;
	qglVertex2f                  = 	dllVertex2f                  = glVertex2f;
	qglVertex2fv                 = 	dllVertex2fv                 = glVertex2fv;
	qglVertex2i                  = 	dllVertex2i                  = glVertex2i;
	qglVertex2iv                 = 	dllVertex2iv                 = glVertex2iv;
	qglVertex2s                  = 	dllVertex2s                  = glVertex2s;
	qglVertex2sv                 = 	dllVertex2sv                 = glVertex2sv;
	qglVertex3d                  = 	dllVertex3d                  = glVertex3d;
	qglVertex3dv                 = 	dllVertex3dv                 = glVertex3dv;
	qglVertex3f                  = 	dllVertex3f                  = glVertex3f;
	qglVertex3fv                 = 	dllVertex3fv                 = glVertex3fv;
	qglVertex3i                  = 	dllVertex3i                  = glVertex3i;
	qglVertex3iv                 = 	dllVertex3iv                 = glVertex3iv;
	qglVertex3s                  = 	dllVertex3s                  = glVertex3s;
	qglVertex3sv                 = 	dllVertex3sv                 = glVertex3sv;
	qglVertex4d                  = 	dllVertex4d                  = glVertex4d;
	qglVertex4dv                 = 	dllVertex4dv                 = glVertex4dv;
	qglVertex4f                  = 	dllVertex4f                  = glVertex4f;
	qglVertex4fv                 = 	dllVertex4fv                 = glVertex4fv;
	qglVertex4i                  = 	dllVertex4i                  = glVertex4i;
	qglVertex4iv                 = 	dllVertex4iv                 = glVertex4iv;
	qglVertex4s                  = 	dllVertex4s                  = glVertex4s;
	qglVertex4sv                 = 	dllVertex4sv                 = glVertex4sv;
	qglVertexPointer             = 	dllVertexPointer             = glVertexPointer;
	qglViewport                  = 	dllViewport                  = glViewport;

	qwglCopyContext              = wglCopyContext;
	qwglCreateContext            = wglCreateContext;
	qwglCreateLayerContext       = wglCreateLayerContext;
	qwglDeleteContext            = wglDeleteContext;
	qwglDescribeLayerPlane       = wglDescribeLayerPlane;
	qwglGetCurrentContext        = wglGetCurrentContext;
	qwglGetCurrentDC             = wglGetCurrentDC;
	qwglGetLayerPaletteEntries   = wglGetLayerPaletteEntries;
	qwglGetProcAddress           = wglGetProcAddress;
	qwglMakeCurrent              = wglMakeCurrent;
	qwglRealizeLayerPalette      = wglRealizeLayerPalette;
	qwglSetLayerPaletteEntries   = wglSetLayerPaletteEntries;
	qwglShareLists               = wglShareLists;
	qwglSwapLayerBuffers         = wglSwapLayerBuffers;
	qwglUseFontBitmaps           = wglUseFontBitmapsA;
	qwglUseFontOutlines          = wglUseFontOutlinesA;

	qwglChoosePixelFormat        = ChoosePixelFormat;
	qwglDescribePixelFormat      = DescribePixelFormat;
	qwglGetPixelFormat           = GetPixelFormat;
	qwglSetPixelFormat           = SetPixelFormat;
	qwglSwapBuffers              = SwapBuffers;

	return true;
}

/*
==================
GLimp_EnableLogging
==================
*/
void GLimp_EnableLogging( bool enable ) {

}
