/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 2016 Johannes Ohlemacher (http://github.com/eXistence/fhDOOM)

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma once

class idDrawVert;

enum GFXenum
{
	GFX_INVALID_ENUM = 0x0500,
	GFX_LINES = 0x0001,
	GFX_LINE_LOOP = 0x0002,
	GFX_TRIANGLES = 0x0004,
	GFX_QUADS = 0x0007,
	GFX_QUAD_STRIP = 0x0008,
	GFX_POLYGON = 0x0009,
};


class fhImmediateMode
{
public:
	explicit fhImmediateMode( nvrhi::ICommandList* _commandList, bool geometryOnly = false );
	~fhImmediateMode();

	void SetTexture( idImage* texture );

	void Begin( GFXenum mode );
	void TexCoord2f( float s, float t );
	void TexCoord2fv( const float* v );
	void Color3fv( const float* c );
	void Color3f( float r, float g, float b );
	void Color4f( float r, float g, float b, float a );
	void Color4fv( const float* c );
	void Color4ubv( const byte* bytes );
	void Vertex3fv( const float* c );
	void Vertex3f( float x, float y, float z );
	void Vertex2f( float x, float y );
	void End();

	void Sphere( float radius, int rings, int sectors, bool inverse = false );

	GFXenum getCurrentMode() const
	{
		return currentMode;
	}

	static void AddTrianglesFromPolygon( fhImmediateMode& im, const idVec3* xyz, int num );

	static void Init( nvrhi::ICommandList* commandList );
	static void Shutdown();
	static void ResetStats();
	static int DrawCallCount();
	static int DrawCallVertexSize();
private:

	static void InitBuffers( nvrhi::ICommandList* commandList );

	nvrhi::CommandListHandle		commandList;
	static idVertexBuffer			vertexBuffer;
	static idIndexBuffer			indexBuffer;

	bool		geometryOnly;
	float		currentTexCoord[2];
	GFXenum		currentMode;
	byte		currentColor[4];
	idImage*	currentTexture;
	int			drawVertsUsed;

	//idDrawVert*                     vertexPointer;
	//triIndex_t*                     indexPointer;
	//int							    numVerts;
	//int							    numIndexes;

	static int drawCallCount;
	static int drawCallVertexSize;
};

class fhLineBuffer
{
public:
	fhLineBuffer();
	~fhLineBuffer();

	void Add( idVec3 from, idVec3 to, idVec4 color );
	void Add( idVec3 from, idVec3 to, idVec3 color );
	void Clear();
	void Commit();

private:
	int verticesAllocated;
	int verticesUsed;
	idDrawVert* vertices;
};
