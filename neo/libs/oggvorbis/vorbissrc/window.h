/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: window functions
 last mod: $Id: window.h,v 1.14 2003/09/05 22:17:41 giles Exp $

 ********************************************************************/

#ifndef _V_WINDOW_
#define _V_WINDOW_

extern float* _vorbis_window_get( int n );
extern void _vorbis_apply_window( float* d, int* winno, long* blocksizes,
								  int lW, int W, int nW );


#endif
