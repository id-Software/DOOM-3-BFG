/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.
 last mod: $Id: os_types.h,v 1.14 2003/09/02 05:09:14 xiphmont Exp $

 ********************************************************************/
#ifndef _OS_TYPES_H
#define _OS_TYPES_H

/* make it easy on the folks that want to compile the libs with a
   different malloc than stdlib */

// using private thread safe memory allocator for DOOM
#if 1

	#include <stddef.h>
	#if !__MACH__ && __MWERKS__
		#include <types.h>
	#else
		#include <sys/types.h>
	#endif

	void* _decoder_malloc( size_t size );
	void* _decoder_calloc( size_t num, size_t size );
	void* _decoder_realloc( void* memblock, size_t size );
	void _decoder_free( void* memblock );

	#define _ogg_malloc		_decoder_malloc
	#define _ogg_calloc		_decoder_calloc
	#define _ogg_realloc	_decoder_realloc
	#define _ogg_free		_decoder_free

#else

	#define _ogg_malloc  malloc
	#define _ogg_calloc  calloc
	#define _ogg_realloc realloc
	#define _ogg_free    free

#endif

#ifdef _WIN32

	#ifndef __GNUC__
		/* MSVC/Borland */
		typedef __int64 ogg_int64_t;
		typedef __int32 ogg_int32_t;
		typedef unsigned __int32 ogg_uint32_t;
		typedef __int16 ogg_int16_t;
		typedef unsigned __int16 ogg_uint16_t;

		#pragma warning(disable : 4018)		// signed/unsigned mismatch
		#pragma warning(disable : 4146)		// unary minus operator applied to unsigned type, result still unsigned
		#pragma warning(disable : 4244)		// conversion to smaller type, possible loss of data
		#pragma warning(disable : 4305)		// truncation from 'double' to 'float'

	#else
		/* Cygwin */
		#include <_G_config.h>
		typedef _G_int64_t ogg_int64_t;
		typedef _G_int32_t ogg_int32_t;
		typedef _G_uint32_t ogg_uint32_t;
		typedef _G_int16_t ogg_int16_t;
		typedef _G_uint16_t ogg_uint16_t;
	#endif

#elif defined(MACOS_X) /* MacOS X Framework build */

	#include <sys/types.h>
	typedef int16_t ogg_int16_t;
	typedef u_int16_t ogg_uint16_t;
	typedef int32_t ogg_int32_t;
	typedef u_int32_t ogg_uint32_t;
	typedef int64_t ogg_int64_t;

#elif defined(__MACOS__)

	#include <types.h>
	typedef SInt16 ogg_int16_t;
	typedef UInt16 ogg_uint16_t;
	typedef SInt32 ogg_int32_t;
	typedef UInt32 ogg_uint32_t;
	typedef SInt64 ogg_int64_t;

#elif defined(__BEOS__)

	/* Be */
	#include <inttypes.h>
	typedef int16_t ogg_int16_t;
	typedef u_int16_t ogg_uint16_t;
	typedef int32_t ogg_int32_t;
	typedef u_int32_t ogg_uint32_t;
	typedef int64_t ogg_int64_t;

#elif defined (__EMX__)

	/* OS/2 GCC */
	typedef short ogg_int16_t;
	typedef unsigned short ogg_uint16_t;
	typedef int ogg_int32_t;
	typedef unsigned int ogg_uint32_t;
	typedef long long ogg_int64_t;

#elif defined (DJGPP)

	/* DJGPP */
	typedef short ogg_int16_t;
	typedef int ogg_int32_t;
	typedef unsigned int ogg_uint32_t;
	typedef long long ogg_int64_t;

#elif defined(R5900)

	/* PS2 EE */
	typedef long ogg_int64_t;
	typedef int ogg_int32_t;
	typedef unsigned ogg_uint32_t;
	typedef short ogg_int16_t;

#else

	#include <sys/types.h>
	//#  include <ogg/config_types.h>
	// copied from a system install of config_types.h
	/* these are filled in by configure */
	typedef int16_t ogg_int16_t;
	typedef u_int16_t ogg_uint16_t;
	typedef int32_t ogg_int32_t;
	typedef u_int32_t ogg_uint32_t;
	typedef int64_t ogg_int64_t;


#endif

#endif  /* _OS_TYPES_H */
