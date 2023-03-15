/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2016 Johannes Ohlemacher (http://github.com/eXistence/fhDOOM)
Copyright (C) 2022 Robert Beckebans (BFG integration)

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

#include "precompiled.h"
#pragma hdrstop


#include "RenderCommon.h"
#include "ImmediateMode.h"

#include <sys/DeviceManager.h>
extern DeviceManager* deviceManager;


namespace
{
const int c_drawVertsCapacity = ( 40000 * 4 );
const int c_drawIndexesCapacity = ( c_drawVertsCapacity * 2 );

idDrawVert drawVerts[c_drawVertsCapacity];
triIndex_t lineIndices[c_drawIndexesCapacity];
triIndex_t sphereIndices[c_drawIndexesCapacity];

bool active = false;
}

int fhImmediateMode::drawCallCount = 0;
int fhImmediateMode::drawCallVertexSize = 0;

idVertexBuffer	fhImmediateMode::vertexBuffer;
idIndexBuffer	fhImmediateMode::indexBuffer;

void fhImmediateMode::Init( nvrhi::ICommandList* commandList )
{
	for( int i = 0; i < c_drawVertsCapacity * 2; ++i )
	{
		lineIndices[i] = i;
	}

	ResetStats();
	InitBuffers( commandList );
}

void fhImmediateMode::Shutdown()
{
	vertexBuffer.FreeBufferObject();
	indexBuffer.FreeBufferObject();
}

void fhImmediateMode::ResetStats()
{
	drawCallCount = 0;
	drawCallVertexSize = 0;
}

int fhImmediateMode::DrawCallCount()
{
	return drawCallCount;
}

int fhImmediateMode::DrawCallVertexSize()
{
	return drawCallVertexSize;
}

void fhImmediateMode::InitBuffers( nvrhi::ICommandList* commandList )
{
	commandList->open();
	vertexBuffer.AllocBufferObject( nullptr, c_drawVertsCapacity * sizeof( idDrawVert ), bufferUsageType_t::BU_STATIC, commandList );
	indexBuffer.AllocBufferObject( nullptr, c_drawIndexesCapacity * sizeof( triIndex_t ), bufferUsageType_t::BU_STATIC, commandList );
	commandList->close();
	deviceManager->GetDevice()->executeCommandList( commandList );
}

fhImmediateMode::fhImmediateMode( nvrhi::ICommandList* _commandList, bool geometryOnly ) :
	commandList( _commandList ),
	drawVertsUsed( 0 ),
	currentTexture( nullptr ),
	geometryOnly( geometryOnly ),
	currentMode( GFX_INVALID_ENUM )
{
}

fhImmediateMode::~fhImmediateMode()
{
	End();
}

void fhImmediateMode::SetTexture( idImage* texture )
{
	currentTexture = texture;
}

void fhImmediateMode::Begin( GFXenum mode )
{
	End();
	assert( !active );
	active = true;

	currentMode = mode;
	drawVertsUsed = 0;
}

void fhImmediateMode::End()
{
	active = false;
	if( !drawVertsUsed )
	{
		return;
	}

	vertexBuffer.Update( drawVerts, drawVertsUsed * sizeof( idDrawVert ), 0, false, commandList );
	indexBuffer.Update( lineIndices, drawVertsUsed * sizeof( triIndex_t ), 0, false, commandList );

	renderProgManager.CommitConstantBuffer( commandList, true );

	int bindingLayoutType = renderProgManager.BindingLayoutType();

	idStaticList<nvrhi::BindingLayoutHandle, nvrhi::c_MaxBindingLayouts>* layouts
		= renderProgManager.GetBindingLayout( bindingLayoutType );

	for( int i = 0; i < layouts->Num(); i++ )
	{
		if( !tr.backend.currentBindingSets[i] || *tr.backend.currentBindingSets[i]->getDesc() != tr.backend.pendingBindingSetDescs[bindingLayoutType][i] )
		{
			tr.backend.currentBindingSets[i] = tr.backend.bindingCache.GetOrCreateBindingSet( tr.backend.pendingBindingSetDescs[bindingLayoutType][i], ( *layouts )[i] );
		}
	}

	uint64_t stateBits = tr.backend.glStateBits;

	int program = renderProgManager.CurrentProgram();
	PipelineKey key{ stateBits, program, static_cast<int>( tr.backend.depthBias ), tr.backend.slopeScaleBias, tr.backend.currentFrameBuffer };
	auto pipeline = tr.backend.pipelineCache.GetOrCreatePipeline( key );

	{
		nvrhi::GraphicsState state;

		for( int i = 0; i < layouts->Num(); i++ )
		{
			state.bindings.push_back( tr.backend.currentBindingSets[i] );
		}

		state.indexBuffer = { indexBuffer.GetAPIObject(), nvrhi::Format::R16_UINT, 0};
		state.vertexBuffers = { { vertexBuffer.GetAPIObject(), 0, 0 } };
		state.pipeline = pipeline;
		state.framebuffer = tr.backend.currentFrameBuffer->GetApiObject();

		nvrhi::Viewport viewport{ ( float )tr.backend.currentViewport.x1,
								  ( float )tr.backend.currentViewport.x2,
								  ( float )tr.backend.currentViewport.y1,
								  ( float )tr.backend.currentViewport.y2,
								  0.0f,
								  1.0f };
		state.viewport.addViewport( viewport );
		state.viewport.addScissorRect( nvrhi::Rect( viewport ) );

		commandList->setGraphicsState( state );
	}

	nvrhi::DrawArguments args;
	args.vertexCount = drawVertsUsed;
	commandList->drawIndexed( args );

	// RB: added stats
	tr.backend.pc.c_drawElements++;
	tr.backend.pc.c_drawIndexes += drawVertsUsed;

	// reset
	drawVertsUsed = 0;
	currentMode = GFX_INVALID_ENUM;
}

void fhImmediateMode::TexCoord2f( float s, float t )
{
	currentTexCoord[0] = s;
	currentTexCoord[1] = t;
}

void fhImmediateMode::TexCoord2fv( const float* v )
{
	TexCoord2f( v[0], v[1] );
}

void fhImmediateMode::Color4f( float r, float g, float b, float a )
{
	currentColor[0] = static_cast<byte>( r * 255.0f );
	currentColor[1] = static_cast<byte>( g * 255.0f );
	currentColor[2] = static_cast<byte>( b * 255.0f );
	currentColor[3] = static_cast<byte>( a * 255.0f );
}

void fhImmediateMode::Color3f( float r, float g, float b )
{
	Color4f( r, g, b, 1.0f );
}

void fhImmediateMode::Color3fv( const float* c )
{
	Color4f( c[0], c[1], c[2], 1.0f );
}

void fhImmediateMode::Color4fv( const float* c )
{
	Color4f( c[0], c[1], c[2], c[3] );
}

void fhImmediateMode::Color4ubv( const byte* bytes )
{
	currentColor[0] = bytes[0];
	currentColor[1] = bytes[1];
	currentColor[2] = bytes[2];
	currentColor[3] = bytes[3];
}

void fhImmediateMode::Vertex3fv( const float* c )
{
	Vertex3f( c[0], c[1], c[2] );
}

void fhImmediateMode::Vertex3f( float x, float y, float z )
{
	if( drawVertsUsed + 1 >= c_drawVertsCapacity )
	{
		return;
	}

	//we don't want to draw deprecated quads/polygons... correct them by re-adding
	// previous vertices, so we render triangles instead of quads/polygons
	// NOTE: this only works for convex polygons (just as GL_POLYGON)
	if( ( currentMode == GFX_POLYGON || currentMode == GFX_QUADS ) &&
			drawVertsUsed >= 3 &&
			drawVertsUsed + 3 < c_drawVertsCapacity )
	{
		drawVerts[drawVertsUsed] = drawVerts[0];
		drawVerts[drawVertsUsed + 1] = drawVerts[drawVertsUsed - 1];
		drawVertsUsed += 2;
	}

	if( currentMode == GFX_QUAD_STRIP && drawVertsUsed >= 3 && drawVertsUsed + 3 < c_drawVertsCapacity )
	{
		if( drawVertsUsed % 6 == 0 )
		{
			drawVerts[drawVertsUsed] = drawVerts[drawVertsUsed - 3];
			drawVerts[drawVertsUsed + 1] = drawVerts[drawVertsUsed - 1];
		}
		else if( drawVertsUsed % 3 == 0 )
		{
			drawVerts[drawVertsUsed] = drawVerts[drawVertsUsed - 1];
			drawVerts[drawVertsUsed + 1] = drawVerts[drawVertsUsed - 2];
		}
		drawVertsUsed += 2;
	}

	/*
	if( ( currentMode == GFX_LINES ) &&
			drawVertsUsed >= 2 &&
			drawVertsUsed + 1 < c_drawVertsCapacity )
	{
		// duplicate the last one if new line starts
		if( drawVertsUsed % 2 == 0 )
		{
			drawVerts[drawVertsUsed] = drawVerts[drawVertsUsed -1];
			drawVertsUsed += 1;
		}
	}
	*/

	idDrawVert& vertex = drawVerts[drawVertsUsed++];
	vertex.xyz.Set( x, y, z );
	vertex.SetTexCoord( currentTexCoord[0], currentTexCoord[1] );
	vertex.color[0] = currentColor[0];
	vertex.color[1] = currentColor[1];
	vertex.color[2] = currentColor[2];
	vertex.color[3] = currentColor[3];
}

void fhImmediateMode::Vertex2f( float x, float y )
{
	Vertex3f( x, y, 0.0f );
}


/*
void fhImmediateMode::Sphere( float radius, int rings, int sectors, bool inverse )
{
	assert( !active );
	assert( radius > 0.0f );
	assert( rings > 1 );
	assert( sectors > 1 );

	float const R = 1. / ( float )( rings - 1 );
	float const S = 1. / ( float )( sectors - 1 );

	int vertexNum = 0;
	for( int r = 0; r < rings; r++ )
	{
		for( int s = 0; s < sectors; s++ )
		{
			float const y = sin( -( idMath::PI / 2.0f ) + idMath::PI * r * R );
			float const x = cos( 2 * idMath::PI * s * S ) * sin( idMath::PI * r * R );
			float const z = sin( 2 * idMath::PI * s * S ) * sin( idMath::PI * r * R );

			drawVerts[vertexNum].xyz.x = x * radius;
			drawVerts[vertexNum].xyz.y = y * radius;
			drawVerts[vertexNum].xyz.z = z * radius;
			drawVerts[vertexNum].st.x = s * S;
			drawVerts[vertexNum].st.y = r * R;
			drawVerts[vertexNum].color[0] = currentColor[0];
			drawVerts[vertexNum].color[1] = currentColor[1];
			drawVerts[vertexNum].color[2] = currentColor[2];
			drawVerts[vertexNum].color[3] = currentColor[3];

			vertexNum += 1;
		}
	}

	int indexNum = 0;
	for( int r = 0; r < rings - 1; r++ )
	{
		for( int s = 0; s < sectors - 1; s++ )
		{
			if( r == 0 )
			{
				//faces of first ring are single triangles
				sphereIndices[indexNum + 2] = r * sectors + s;
				sphereIndices[indexNum + 1] = ( r + 1 ) * sectors + s;
				sphereIndices[indexNum + 0] = ( r + 1 ) * sectors + ( s + 1 );

				indexNum += 3;
			}
			else if( r == rings - 2 )
			{
				//faces of last ring are single triangles
				sphereIndices[indexNum + 0] = r * sectors + s;
				sphereIndices[indexNum + 1] = r * sectors + ( s + 1 );
				sphereIndices[indexNum + 2] = ( r + 1 ) * sectors + ( s + 1 );

				indexNum += 3;
			}
			else
			{
				//faces of remaining rings are quads (two triangles)
				sphereIndices[indexNum + 0] = r * sectors + s;
				sphereIndices[indexNum + 1] = r * sectors + ( s + 1 );
				sphereIndices[indexNum + 2] = ( r + 1 ) * sectors + ( s + 1 );

				sphereIndices[indexNum + 3] = sphereIndices[indexNum + 2];
				sphereIndices[indexNum + 4] = ( r + 1 ) * sectors + s;
				sphereIndices[indexNum + 5] = sphereIndices[indexNum + 0];

				indexNum += 6;
			}
		}
	}

	if( inverse )
	{
		for( int i = 0; i + 2 < indexNum; i += 3 )
		{
			unsigned short tmp = sphereIndices[i];
			sphereIndices[i] = sphereIndices[i + 2];
			sphereIndices[i + 2] = tmp;
		}
	}

	GL_UseProgram( vertexColorProgram );

	auto vert = vertexCache.AllocFrameTemp( drawVerts, vertexNum * sizeof( fhSimpleVert ) );
	int offset = vertexCache.Bind( vert );

	fhRenderProgram::SetModelViewMatrix( GL_ModelViewMatrix.Top() );
	fhRenderProgram::SetProjectionMatrix( GL_ProjectionMatrix.Top() );
	fhRenderProgram::SetDiffuseColor( idVec4::one );
	fhRenderProgram::SetBumpMatrix( idVec4( 1, 0, 0, 0 ), idVec4( 0, 1, 0, 0 ) );

	GL_SetupVertexAttributes( fhVertexLayout::Simple, offset );

	glDrawElements( GL_TRIANGLES,
					indexNum,
					GL_UNSIGNED_SHORT,
					sphereIndices );

	GL_SetVertexLayout( fhVertexLayout::None );
	GL_UseProgram( nullptr );
}
*/

void fhImmediateMode::AddTrianglesFromPolygon( fhImmediateMode& im, const idVec3* xyz, int num )
{
	assert( im.getCurrentMode() == GFX_TRIANGLES );

	if( num < 3 )
	{
		return;
	}

	for( int i = 0; i < num; ++i )
	{
		if( i > 0 && i % 3 == 0 )
		{
			im.Vertex3fv( xyz[0].ToFloatPtr() );
			im.Vertex3fv( xyz[i - 1].ToFloatPtr() );
		}
		im.Vertex3fv( xyz[i].ToFloatPtr() );
	}
}


/*
fhLineBuffer::fhLineBuffer()
	: verticesUsed( 0 )
	, verticesAllocated( 4096 )
{
	vertices = new fhSimpleVert[verticesAllocated];
}

fhLineBuffer::~fhLineBuffer()
{
	delete[] vertices;
}

void fhLineBuffer::Add( idVec3 from, idVec3 to, idVec3 color )
{
	Add( from, to, idVec4( color.x, color.y, color.z, 1.0f ) );
}

void fhLineBuffer::Add( idVec3 from, idVec3 to, idVec4 color )
{
	if( verticesAllocated - verticesUsed < 2 )
	{
		fhSimpleVert* n = new fhSimpleVert[verticesUsed * 2];
		memcpy( n, vertices, sizeof( vertices[0] ) * verticesUsed );
		delete[] vertices;
		vertices = n;
		verticesAllocated = verticesUsed * 2;
	}

	byte colorBytes[4];
	colorBytes[0] = static_cast<byte>( color.x * 255.0f );
	colorBytes[1] = static_cast<byte>( color.y * 255.0f );
	colorBytes[2] = static_cast<byte>( color.z * 255.0f );
	colorBytes[3] = static_cast<byte>( color.w * 255.0f );

	fhSimpleVert& fromVert = vertices[verticesUsed + 0];
	fhSimpleVert& toVert = vertices[verticesUsed + 1];

	memcpy( fromVert.color, colorBytes, sizeof( colorBytes ) );
	memcpy( toVert.color, colorBytes, sizeof( colorBytes ) );
	fromVert.xyz = from;
	toVert.xyz = to;

	verticesUsed += 2;
}

void fhLineBuffer::Commit()
{
	if( verticesUsed > 0 )
	{
		GL_UseProgram( vertexColorProgram );

		fhRenderProgram::SetModelViewMatrix( GL_ModelViewMatrix.Top() );
		fhRenderProgram::SetProjectionMatrix( GL_ProjectionMatrix.Top() );
		fhRenderProgram::SetDiffuseColor( idVec4::one );
		fhRenderProgram::SetColorAdd( idVec4::zero );
		fhRenderProgram::SetColorModulate( idVec4::one );
		fhRenderProgram::SetBumpMatrix( idVec4( 1, 0, 0, 0 ), idVec4( 0, 1, 0, 0 ) );

		int verticesCommitted = 0;
		while( verticesCommitted < verticesUsed )
		{
			static const int maxVerticesPerCommit = ( sizeof( lineIndices ) / sizeof( lineIndices[0] ) ) / 2;

			int verticesToCommit = Min( maxVerticesPerCommit, verticesUsed - verticesCommitted );

			auto vert = vertexCache.AllocFrameTemp( &vertices[verticesCommitted], verticesToCommit * sizeof( fhSimpleVert ) );
			int offset = vertexCache.Bind( vert );

			GL_SetupVertexAttributes( fhVertexLayout::Simple, offset );

			glDrawElements( GL_LINES,
							verticesToCommit,
							GL_UNSIGNED_SHORT,
							lineIndices );

			verticesCommitted += verticesToCommit;
		}
	}

	verticesUsed = 0;
}

void fhLineBuffer::Clear()
{
	verticesUsed = 0;
}
*/
