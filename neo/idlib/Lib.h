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

#ifndef __LIB_H__
#define __LIB_H__

#include <stddef.h>

/*
===============================================================================

	idLib contains stateless support classes and concrete types. Some classes
	do have static variables, but such variables are initialized once and
	read-only after initialization (they do not maintain a modifiable state).

	The interface pointers idSys, idCommon, idCVarSystem and idFileSystem
	should be set before using idLib. The pointers stored here should not
	be used by any part of the engine except for idLib.

===============================================================================
*/

class idLib
{
private:
	static bool					mainThreadInitialized;
	static ID_TLS				isMainThread;

public:
	static class idSys* 		sys;
	static class idCommon* 		common;
	static class idCVarSystem* 	cvarSystem;
	static class idFileSystem* 	fileSystem;
	static int					frameNumber;

	static void					Init();
	static void					ShutDown();

	// wrapper to idCommon functions
	static void       			Printf( VERIFY_FORMAT_STRING const char* fmt, ... ) ID_STATIC_ATTRIBUTE_PRINTF( 1, 2 );
	static void       			PrintfIf( const bool test, VERIFY_FORMAT_STRING const char* fmt, ... ) ID_STATIC_ATTRIBUTE_PRINTF( 2, 3 );
	NO_RETURN static void       Error( VERIFY_FORMAT_STRING const char* fmt, ... ) ID_STATIC_ATTRIBUTE_PRINTF( 1, 2 );
	NO_RETURN static void       FatalError( VERIFY_FORMAT_STRING const char* fmt, ... ) ID_STATIC_ATTRIBUTE_PRINTF( 1, 2 );
	static void       			Warning( VERIFY_FORMAT_STRING const char* fmt, ... ) ID_STATIC_ATTRIBUTE_PRINTF( 1, 2 );
	static void       			WarningIf( const bool test, VERIFY_FORMAT_STRING const char* fmt, ... ) ID_STATIC_ATTRIBUTE_PRINTF( 2, 3 );

	// the extra check for mainThreadInitialized is necessary for this to be accurate
	// when called by startup code that happens before idLib::Init
	static bool					IsMainThread()
	{
		return ( 0 == mainThreadInitialized ) || ( 1 == isMainThread );
	}
};


/*
===============================================================================

	Types and defines used throughout the engine.

===============================================================================
*/

typedef int						qhandle_t;

class idFile;
class idVec3;
class idVec4;

#ifndef NULL
	#define NULL					((void *)0)
#endif

#ifndef BIT
	#define BIT( num )				( 1ULL << ( num ) )
#endif

#define	MAX_STRING_CHARS		1024		// max length of a string
#define MAX_PRINT_MSG			16384		// buffer size for our various printf routines

// maximum world size
#define MAX_WORLD_COORD			( 128 * 1024 )
#define MIN_WORLD_COORD			( -128 * 1024 )
#define MAX_WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

#define SIZE_KB( x )						( ( (x) + 1023 ) / 1024 )
#define SIZE_MB( x )						( ( ( SIZE_KB( x ) ) + 1023 ) / 1024 )
#define SIZE_GB( x )						( ( ( SIZE_MB( x ) ) + 1023 ) / 1024 )

// basic colors
extern	idVec4 colorBlack;
extern	idVec4 colorWhite;
extern	idVec4 colorRed;
extern	idVec4 colorGreen;
extern	idVec4 colorBlue;
extern	idVec4 colorYellow;
extern	idVec4 colorMagenta;
extern	idVec4 colorCyan;
extern	idVec4 colorOrange;
extern	idVec4 colorPurple;
extern	idVec4 colorPink;
extern	idVec4 colorBrown;
extern	idVec4 colorLtGrey;
extern	idVec4 colorMdGrey;
extern	idVec4 colorDkGrey;
// jmarshall
extern idVec4  colorGold;
// jmarshall end

// packs color floats in the range [0,1] into an integer
dword	PackColor( const idVec3& color );
void	UnpackColor( const dword color, idVec3& unpackedColor );
dword	PackColor( const idVec4& color );
void	UnpackColor( const dword color, idVec4& unpackedColor );

// little/big endian conversion
short	BigShort( short l );
short	LittleShort( short l );
int		BigLong( int l );
int		LittleLong( int l );
float	BigFloat( float l );
float	LittleFloat( float l );
void	BigRevBytes( void* bp, int elsize, int elcount );
void	LittleRevBytes( void* bp, int elsize, int elcount );
void	LittleBitField( void* bp, int elsize );
void	Swap_Init();

bool	Swap_IsBigEndian();

// for base64
void	SixtetsForInt( byte* out, int src );
int		IntForSixtets( byte* in );

/*
================================================
idException
================================================
*/
class idException
{
public:
	static const int MAX_ERROR_LEN = 2048;

	idException( const char* text = "" )
	{
		strncpy( error, text, MAX_ERROR_LEN );
	}

	// this really, really should be a const function, but it's referenced too many places to change right now
	const char* 	GetError()
	{
		return error;
	}

protected:
	// if GetError() were correctly const this would be named GetError(), too
	char* 		GetErrorBuffer()
	{
		return error;
	}
	int			GetErrorBufferSize()
	{
		return MAX_ERROR_LEN;
	}

private:
	friend class idFatalException;
	static char error[MAX_ERROR_LEN];
};

/*
================================================
idFatalException
================================================
*/
class idFatalException
{
public:
	static const int MAX_ERROR_LEN = 2048;

	idFatalException( const char* text = "" )
	{
		strncpy( idException::error, text, MAX_ERROR_LEN );
	}

	// this really, really should be a const function, but it's referenced too many places to change right now
	const char* 	GetError()
	{
		return idException::error;
	}

protected:
	// if GetError() were correctly const this would be named GetError(), too
	char* 		GetErrorBuffer()
	{
		return idException::error;
	}
	int			GetErrorBufferSize()
	{
		return MAX_ERROR_LEN;
	}
};

/*
================================================
idNetworkLoadException
================================================
*/
class idNetworkLoadException : public idException
{
public:
	idNetworkLoadException( const char* text = "" ) : idException( text ) { }
};

/*
===============================================================================

	idLib headers.

===============================================================================
*/

// System
#include "sys/sys_assert.h"
#include "sys/sys_threading.h"

// memory management and arrays
#include "Heap.h"
#include "containers/Sort.h"
#include "containers/List.h"

// math
#include "math/Simd.h"
#include "math/Math.h"
#include "math/Random.h"
#include "math/Complex.h"
#include "math/Vector.h"
#include "math/VecX.h"
#include "math/VectorI.h"
#include "math/Matrix.h"
#include "math/MatX.h"
#include "math/Angles.h"
#include "math/Quat.h"
#include "math/Rotation.h"
#include "math/Plane.h"
#include "math/Pluecker.h"
#include "math/Polynomial.h"
#include "math/Extrapolate.h"
#include "math/Interpolate.h"
#include "math/Curve.h"
#include "math/Ode.h"
#include "math/Lcp.h"
#include "math/SphericalHarmonics.h"

// bounding volumes
#include "bv/Sphere.h"
#include "bv/Bounds.h"
#include "bv/Box.h"

// geometry
#include "geometry/RenderMatrix.h"
#include "geometry/JointTransform.h"
#include "geometry/DrawVert.h"
#include "geometry/Winding.h"
#include "geometry/Winding2D.h"
#include "geometry/Surface.h"
#include "geometry/Surface_Patch.h"
#include "geometry/Surface_Polytope.h"
#include "geometry/Surface_SweptSpline.h"
#include "geometry/TraceModel.h"

// text manipulation
#include "Str.h"
#include "StrStatic.h"
#include "Token.h"
#include "Lexer.h"
#include "Parser.h"
#include "Base64.h"
#include "CmdArgs.h"

// containers
#include "containers/Array.h"
#include "containers/BTree.h"
#include "containers/BinSearch.h"
#include "containers/HashIndex.h"
#include "containers/HashTable.h"
#include "containers/StaticList.h"
#include "containers/LinkList.h"
#include "containers/Hierarchy.h"
#include "containers/Queue.h"
#include "containers/Stack.h"
#include "containers/StrList.h"
#include "containers/StrPool.h"
#include "containers/VectorSet.h"
#include "containers/PlaneSet.h"

// hashing
#include "hashing/CRC32.h"
#include "hashing/MD4.h"
#include "hashing/MD5.h"

// misc
#include "Dict.h"
#include "LangDict.h"
#include "DataQueue.h"
#include "BitMsg.h"
#include "MapFile.h"
#include "Timer.h"
#include "Thread.h"
#include "Swap.h"
#include "Callback.h"
#include "ParallelJobList.h"
#include "SoftwareCache.h"
#include "TileMap.h" // RB

#endif	/* !__LIB_H__ */
