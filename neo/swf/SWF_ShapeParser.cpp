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
#include "precompiled.h"
#pragma hdrstop
#include "float.h"

#pragma warning( disable: 4189 ) // local variable is initialized but not referenced

/*
========================
idSWFShapeParser::ParseShape
========================
*/
void idSWFShapeParser::Parse( idSWFBitStream& bitstream, idSWFShape& shape, int recordType )
{
	extendedCount = ( recordType > 1 );
	lineStyle2 = ( recordType == 4 );
	rgba = ( recordType >= 3 );
	morph = false;

	bitstream.ReadRect( shape.startBounds );
	shape.endBounds = shape.startBounds;

	if( recordType == 4 )
	{
		swfRect_t edgeBounds;
		bitstream.ReadRect( edgeBounds );
		bitstream.ReadU8();	// flags (that we ignore)
	}

	ReadFillStyle( bitstream );
	ParseShapes( bitstream, NULL, false );
	TriangulateSoup( shape );

	shape.lineDraws.SetNum( lineDraws.Num() );
	for( int i = 0; i < lineDraws.Num(); i++ )
	{
		idSWFShapeDrawLine& ld = shape.lineDraws[i];
		swfSPDrawLine_t& spld = lineDraws[i];
		ld.style = spld.style;
		ld.indices.SetNum( spld.edges.Num() * 3 );
		ld.indices.SetNum( 0 );
		for( int e = 0; e < spld.edges.Num(); e++ )
		{
			int v0 = ld.startVerts.AddUnique( verts[ spld.edges[e].start.v0 ] );
			ld.indices.Append( v0 );
			ld.indices.Append( v0 );

			// Rather then tesselating curves at run time, we do them once here by inserting a vert every 10 units
			// It may not wind up being 10 actual pixels when rendered because the shape may have scaling applied to it
			if( spld.edges[e].start.cp != 0xFFFF )
			{
				assert( spld.edges[e].end.cp != 0xFFFF );
				float length1 = ( verts[ spld.edges[e].start.v0 ] - verts[ spld.edges[e].start.v1 ] ).Length();
				float length2 = ( verts[ spld.edges[e].end.v0 ] - verts[ spld.edges[e].end.v1 ] ).Length();
				int numPoints = 1 + idMath::Ftoi( Max( length1, length2 ) / 10.0f );
				for( int ti = 0; ti < numPoints; ti++ )
				{
					float t0 = ( ti + 1 ) / ( ( float ) numPoints + 1.0f );
					float t1 = ( 1.0f - t0 );
					float c1 = t1 * t1;
					float c2 = t0 * t1 * 2.0f;
					float c3 = t0 * t0;

					idVec2	p1  = c1 * verts[ spld.edges[e].start.v0 ];
					p1 += c2 * verts[ spld.edges[e].start.cp ];
					p1 += c3 * verts[ spld.edges[e].start.v1 ];

					int v1 = ld.startVerts.AddUnique( p1 );
					ld.indices.Append( v1 );
					ld.indices.Append( v1 );
					ld.indices.Append( v1 );
				}
			}
			ld.indices.Append( ld.startVerts.AddUnique( verts[ spld.edges[e].start.v1 ] ) );
		}
	}
}

/*
========================
idSWFShapeParser::ParseMorph
========================
*/
void idSWFShapeParser::ParseMorph( idSWFBitStream& bitstream, idSWFShape& shape )
{
	extendedCount = true;
	lineStyle2 = false;
	rgba = true;
	morph = true;

	bitstream.ReadRect( shape.startBounds );
	bitstream.ReadRect( shape.endBounds );

	uint32 offset = bitstream.ReadU32();

	// offset is the byte offset from the current read position to the 'endShape' record
	// we read the entire block into 'bitstream1' which moves the read pointer of 'bitstream'
	// to the start of the 'endShape' record

	idSWFBitStream bitstream1;
	bitstream1.Load( ( byte* )bitstream.ReadData( offset ), offset, false );

	ReadFillStyle( bitstream1 );
	ParseShapes( bitstream1, &bitstream, true );
	TriangulateSoup( shape );
}

/*
========================
idSWFShapeParser::ParseFont
========================
*/
void idSWFShapeParser::ParseFont( idSWFBitStream& bitstream, idSWFFontGlyph& shape )
{
	extendedCount = false;
	lineStyle2 = false;
	rgba = false;
	morph = false;

	fillDraws.SetNum( 1 );

	ParseShapes( bitstream, NULL, true );
	TriangulateSoup( shape );
}

/*
========================
idSWFShapeParser::ParseShapes
========================
*/
void idSWFShapeParser::ParseShapes( idSWFBitStream& bitstream1, idSWFBitStream* bitstream2, bool swap )
{
	int32 pen1X = 0;
	int32 pen1Y = 0;
	int32 pen2X = 0;
	int32 pen2Y = 0;

	uint8 fillStyle0 = 0;
	uint8 fillStyle1 = 0;
	uint8 lineStyle = 0;

	uint16 baseFillStyle = 0;
	uint16 baseLineStyle = 0;

	uint8 numBits = bitstream1.ReadU8();
	uint8 numFillBits1 = numBits >> 4;
	uint8 numLineBits1 = numBits & 0xF;

	uint8 numFillBits2 = 0;
	uint8 numLineBits2 = 0;

	if( bitstream2 )
	{
		numBits = bitstream2->ReadU8();
		numFillBits2 = numBits >> 4;
		numLineBits2 = numBits & 0xF;
	}

	while( true )
	{
		if( !bitstream1.ReadBool() )
		{
			bool stateNewStyles = bitstream1.ReadBool();
			bool stateLineStyle = bitstream1.ReadBool();
			bool stateFillStyle1 = bitstream1.ReadBool();
			bool stateFillStyle0 = bitstream1.ReadBool();
			bool stateMoveTo = bitstream1.ReadBool();
			if( ( stateNewStyles || stateLineStyle || stateFillStyle1 || stateFillStyle0 || stateMoveTo ) == false )
			{
				// end record
				if( bitstream2 )
				{
					uint8 flags = bitstream2->ReadU( 6 );
					if( flags != 0 )
					{
						idLib::Warning( "idSWFShapeParser: morph stream 1 ends before 2" );
						break;
					}
				}
				break;
			}
			if( stateMoveTo )
			{
				uint8 moveBits = bitstream1.ReadU( 5 );
				pen1X = bitstream1.ReadS( moveBits );
				pen1Y = bitstream1.ReadS( moveBits );
			}
			if( stateFillStyle0 )
			{
				fillStyle0 = bitstream1.ReadU( numFillBits1 );
			}
			if( stateFillStyle1 )
			{
				fillStyle1 = bitstream1.ReadU( numFillBits1 );
			}
			if( stateLineStyle )
			{
				lineStyle = bitstream1.ReadU( numLineBits1 );
			}
			if( stateNewStyles )
			{
				baseFillStyle = fillDraws.Num();
				baseLineStyle = lineDraws.Num();
				ReadFillStyle( bitstream1 );
				numBits = bitstream1.ReadU8();
				numFillBits1 = numBits >> 4;
				numLineBits1 = numBits & 0xF;
			}
			if( bitstream2 )
			{
				bool isEdge = bitstream2->ReadBool();
				if( isEdge )
				{
					idLib::Warning( "idSWFShapeParser: morph stream 1 defines style change, but stream 2 does not" );
					break;
				}
				bool stateNewStyles = bitstream2->ReadBool();
				bool stateLineStyle = bitstream2->ReadBool();
				bool stateFillStyle1 = bitstream2->ReadBool();
				bool stateFillStyle0 = bitstream2->ReadBool();
				bool stateMoveTo = bitstream2->ReadBool();
				if( stateMoveTo )
				{
					uint8 moveBits = bitstream2->ReadU( 5 );
					pen2X = bitstream2->ReadS( moveBits );
					pen2Y = bitstream2->ReadS( moveBits );
				}
				if( stateFillStyle0 )
				{
					if( bitstream2->ReadU( numFillBits2 ) != fillStyle0 )
					{
						idLib::Warning( "idSWFShapeParser: morph stream 2 defined a different fillStyle0 from stream 1" );
						break;
					}
				}
				if( stateFillStyle1 )
				{
					if( bitstream2->ReadU( numFillBits2 ) != fillStyle1 )
					{
						idLib::Warning( "idSWFShapeParser: morph stream 2 defined a different fillStyle1 from stream 1" );
						break;
					}
				}
				if( stateLineStyle )
				{
					if( bitstream2->ReadU( numLineBits2 ) != lineStyle )
					{
						idLib::Warning( "idSWFShapeParser: morph stream 2 defined a different lineStyle from stream 1" );
						break;
					}
				}
				if( stateNewStyles )
				{
					idLib::Warning( "idSWFShapeParser: morph stream 2 defines new styles" );
					break;
				}
			}
		}
		else
		{
			swfSPMorphEdge_t morphEdge;

			ParseEdge( bitstream1, pen1X, pen1Y, morphEdge.start );
			if( bitstream2 )
			{
				bool isEdge = bitstream2->ReadBool();
				if( !isEdge )
				{
					idLib::Warning( "idSWFShapeParser: morph stream 1 defines an edge, but stream 2 does not" );
					break;
				}
				ParseEdge( *bitstream2, pen2X, pen2Y, morphEdge.end );
			}
			else
			{
				morphEdge.end = morphEdge.start;
			}

			// one edge may be a straight edge, and the other may be a curve
			// in this case, we turn the straight edge into a curve by adding
			// a control point in the middle of the line
			if( morphEdge.start.cp != 0xFFFF )
			{
				if( morphEdge.end.cp == 0xFFFF )
				{
					const idVec2& v0 = verts[ morphEdge.end.v0 ];
					const idVec2& v1 = verts[ morphEdge.end.v1 ];
					morphEdge.end.cp = verts.AddUnique( ( v0 + v1 ) * 0.5f );
				}
			}
			else
			{
				if( morphEdge.end.cp != 0xFFFF )
				{
					const idVec2& v0 = verts[ morphEdge.start.v0 ];
					const idVec2& v1 = verts[ morphEdge.start.v1 ];
					morphEdge.start.cp = verts.AddUnique( ( v0 + v1 ) * 0.5f );
				}
			}

			if( lineStyle != 0 )
			{
				lineDraws[ baseLineStyle + lineStyle - 1 ].edges.Append( morphEdge );
			}
			if( swap )
			{
				SwapValues( morphEdge.start.v0, morphEdge.start.v1 );
				SwapValues( morphEdge.end.v0, morphEdge.end.v1 );
			}
			if( fillStyle1 != 0 )
			{
				fillDraws[ baseFillStyle + fillStyle1 - 1 ].edges.Append( morphEdge );
			}
			if( fillStyle0 != 0 )
			{
				// for fill style 0, we need to reverse the winding
				swfSPMorphEdge_t swapped = morphEdge;
				SwapValues( swapped.start.v0, swapped.start.v1 );
				SwapValues( swapped.end.v0, swapped.end.v1 );
				fillDraws[ baseFillStyle + fillStyle0 - 1 ].edges.Append( swapped );
			}
		}
	}
}

/*
========================
idSWFShapeParser::ParseEdge
========================
*/
void idSWFShapeParser::ParseEdge( idSWFBitStream& bitstream, int32& penX, int32& penY, swfSPEdge_t& edge )
{
	bool straight = bitstream.ReadBool();
	uint8 numBits = bitstream.ReadU( 4 ) + 2;

	edge.v0 = verts.AddUnique( idVec2( SWFTWIP( penX ), SWFTWIP( penY ) ) );

	if( straight )
	{
		edge.cp = 0xFFFF;
		if( bitstream.ReadBool() )
		{
			penX += bitstream.ReadS( numBits );
			penY += bitstream.ReadS( numBits );
		}
		else
		{
			if( bitstream.ReadBool() )
			{
				penY += bitstream.ReadS( numBits );
			}
			else
			{
				penX += bitstream.ReadS( numBits );
			}
		}
	}
	else
	{
		penX += bitstream.ReadS( numBits );
		penY += bitstream.ReadS( numBits );
		edge.cp = verts.AddUnique( idVec2( SWFTWIP( penX ), SWFTWIP( penY ) ) );
		penX += bitstream.ReadS( numBits );
		penY += bitstream.ReadS( numBits );
	}

	edge.v1 = verts.AddUnique( idVec2( SWFTWIP( penX ), SWFTWIP( penY ) ) );
}

/*
========================
idSWFShapeParser::MakeLoops
========================
*/
void idSWFShapeParser::MakeLoops()
{

	// At this point, each fill style has an edge soup associated with it
	// We want to turn this soup into loops of connected verts

	for( int i = 0; i < fillDraws.Num(); i++ )
	{
		swfSPDrawFill_t& fill = fillDraws[i];

		// first remove degenerate edges
		for( int e = 0; e < fill.edges.Num(); e++ )
		{
			if( ( fill.edges[e].start.v0 == fill.edges[e].start.v1 ) || ( fill.edges[e].end.v0 == fill.edges[e].end.v1 ) )
			{
				fill.edges.RemoveIndexFast( e );
				e--;
			}
		}

		idList< int > unusedEdges;
		unusedEdges.SetNum( fill.edges.Num() );
		for( int e = 0; e < fill.edges.Num(); e++ )
		{
			unusedEdges[e] = e;
		}
		while( unusedEdges.Num() > 0 )
		{
			swfSPLineLoop_t& loop = fill.loops.Alloc();
			loop.hole = false;

			int e1 = unusedEdges[ unusedEdges.Num() - 1 ];
			unusedEdges.SetNum( unusedEdges.Num() - 1 );

			while( true )
			{
				loop.vindex1.Append( fill.edges[e1].start.v0 );
				loop.vindex2.Append( fill.edges[e1].end.v0 );

				// Rather then tesselating curves at run time, we do them once here by inserting a vert every 10 units
				// It may not wind up being 10 actual pixels when rendered because the shape may have scaling applied to it
				if( fill.edges[e1].start.cp != 0xFFFF )
				{
					assert( fill.edges[e1].end.cp != 0xFFFF );
					float length1 = ( verts[ fill.edges[e1].start.v0 ] - verts[ fill.edges[e1].start.v1 ] ).Length();
					float length2 = ( verts[ fill.edges[e1].end.v0 ] - verts[ fill.edges[e1].end.v1 ] ).Length();
					int numPoints = 1 + idMath::Ftoi( Max( length1, length2 ) / 10.0f );
					for( int ti = 0; ti < numPoints; ti++ )
					{
						float t0 = ( ti + 1 ) / ( ( float ) numPoints + 1.0f );
						float t1 = ( 1.0f - t0 );
						float c1 = t1 * t1;
						float c2 = t0 * t1 * 2.0f;
						float c3 = t0 * t0;

						idVec2	p1  = c1 * verts[ fill.edges[e1].start.v0 ];
						p1 += c2 * verts[ fill.edges[e1].start.cp ];
						p1 += c3 * verts[ fill.edges[e1].start.v1 ];

						idVec2	p2  = c1 * verts[ fill.edges[e1].end.v0 ];
						p2 += c2 * verts[ fill.edges[e1].end.cp ];
						p2 += c3 * verts[ fill.edges[e1].end.v1 ];

						loop.vindex1.Append( verts.AddUnique( p1 ) );
						loop.vindex2.Append( verts.AddUnique( p2 ) );
					}
				}

				const swfSPEdge_t& edge1 = fill.edges[e1].start;

				float bestNormal = FLT_MAX;
				int beste = -1;
				for( int e = 0; e < unusedEdges.Num(); e++ )
				{
					int e2 = unusedEdges[e];
					const swfSPEdge_t& edge2 = fill.edges[e2].start;
					if( edge1.v1 != edge2.v0 )
					{
						continue;
					}

					assert( edge1.v0 != edge2.v0 );
					assert( edge1.v1 != edge2.v1 );

					const idVec2& v1 = verts[ edge1.v0 ];
					const idVec2& v2 = verts[ edge1.v1 ];  // == edge2.v0
					const idVec2& v3 = verts[ edge2.v1 ];
					idVec2 a = v1 - v2;
					idVec2 b = v3 - v2;

					float normal = ( a.x * b.y - a.y * b.x );
					if( normal < bestNormal )
					{
						bestNormal = normal;
						beste = e;
					}
					else
					{
						assert( beste != -1 );
					}
				}
				if( beste < 0 )
				{
					// no more edges connect to this one
					break;
				}
				e1 = unusedEdges[beste];
				unusedEdges.RemoveIndexFast( beste );
			}
			if( loop.vindex1.Num() < 3 )
			{
				idLib::Warning( "idSWFShapeParser: loop with < 3 verts" );
				fill.loops.SetNum( fill.loops.Num() - 1 );
				continue;
			}
			// Use the left most vert to determine if it's a hole or not
			float leftMostX = FLT_MAX;
			int leftMostIndex = 0;
			for( int j = 0; j < loop.vindex1.Num(); j++ )
			{
				idVec2& v = verts[ loop.vindex1[j] ];
				if( v.x < leftMostX )
				{
					leftMostIndex = j;
					leftMostX = v.x;
				}
			}
			const idVec2& v1 = verts[ loop.vindex1[( loop.vindex1.Num() + leftMostIndex - 1 ) % loop.vindex1.Num()] ];
			const idVec2& v2 = verts[ loop.vindex1[leftMostIndex] ];
			const idVec2& v3 = verts[ loop.vindex1[( leftMostIndex + 1 ) % loop.vindex1.Num()] ];
			idVec2 a = v1 - v2;
			idVec2 b = v3 - v2;
			float normal = ( a.x * b.y - a.y * b.x );
			loop.hole = ( normal > 0.0f );
		}

		// now we have a series of loops, which define either shapes or holes
		// we want to merge the holes into the shapes by inserting edges
		// this assumes shapes are either completely contained or not
		// we start merging holes starting on the right so nested holes work
		while( true )
		{
			int hole = -1;
			int holeVert = -1;
			float rightMostX = -1e10f;
			for( int j = 0; j < fill.loops.Num(); j++ )
			{
				swfSPLineLoop_t& loop = fill.loops[j];
				if( !loop.hole )
				{
					continue;
				}
				for( int v = 0; v < loop.vindex1.Num(); v++ )
				{
					if( verts[ loop.vindex1[v] ].x > rightMostX )
					{
						hole = j;
						holeVert = v;
						rightMostX = verts[ loop.vindex1[v] ].x;
					}
				}
			}
			if( hole == -1 )
			{
				break;
			}
			swfSPLineLoop_t& loopHole = fill.loops[ hole ];
			const idVec2& holePoint = verts[ loopHole.vindex1[ holeVert ] ];

			int shape = -1;
			for( int j = 0; j < fill.loops.Num(); j++ )
			{
				swfSPLineLoop_t& loop = fill.loops[j];
				if( loop.hole )
				{
					continue;
				}
				bool inside = false;
				for( int k = 0; k < loop.vindex1.Num(); k++ )
				{
					const idVec2& v1 = verts[ loop.vindex1[k] ];
					const idVec2& v2 = verts[ loop.vindex1[( k + 1 ) % loop.vindex1.Num()] ];
					if( v1.x < holePoint.x && v2.x < holePoint.x )
					{
						continue; // both on the left of the holePoint
					}
					if( ( v1.y < holePoint.y ) == ( v2.y < holePoint.y ) )
					{
						continue; // both on the same side of the horizon
					}
					assert( v1 != holePoint );
					assert( v2 != holePoint );
					inside = !inside;
				}
				if( inside )
				{
					shape = j;
					break;
				}
			}
			if( shape == -1 )
			{
				idLib::Warning( "idSWFShapeParser: Hole not in a shape" );
				fill.loops.RemoveIndexFast( hole );
				continue;
			}
			swfSPLineLoop_t& loopShape = fill.loops[ shape ];

			// now that we have a hole and the shape it's inside, merge the two together

			// find the nearest vert that's on the right side of holePoint
			float bestDist = 1e10f;
			int shapeVert = -1;
			for( int j = 0; j < loopShape.vindex1.Num(); j++ )
			{
				const idVec2& v1 = verts[ loopShape.vindex1[j] ];
				if( v1.x < holePoint.x )
				{
					continue; // on the left of the holePoint
				}
				float dist = ( v1 - holePoint ).Length();
				if( dist < bestDist )
				{
					shapeVert = j;
					bestDist = dist;
				}
			}

			// connect holeVert to shapeVert
			idList< uint16 > vindex;
			vindex.SetNum( loopShape.vindex1.Num() + loopHole.vindex1.Num() + 1 );
			vindex.SetNum( 0 );
			for( int j = 0; j <= shapeVert; j++ )
			{
				vindex.Append( loopShape.vindex1[j] );
			}
			for( int j = holeVert; j < loopHole.vindex1.Num(); j++ )
			{
				vindex.Append( loopHole.vindex1[j] );
			}
			for( int j = 0; j <= holeVert; j++ )
			{
				vindex.Append( loopHole.vindex1[j] );
			}
			for( int j = shapeVert; j < loopShape.vindex1.Num(); j++ )
			{
				vindex.Append( loopShape.vindex1[j] );
			}
			loopShape.vindex1 = vindex;

			vindex.Clear();
			for( int j = 0; j <= shapeVert; j++ )
			{
				vindex.Append( loopShape.vindex2[j] );
			}
			for( int j = holeVert; j < loopHole.vindex2.Num(); j++ )
			{
				vindex.Append( loopHole.vindex2[j] );
			}
			for( int j = 0; j <= holeVert; j++ )
			{
				vindex.Append( loopHole.vindex2[j] );
			}
			for( int j = shapeVert; j < loopShape.vindex2.Num(); j++ )
			{
				vindex.Append( loopShape.vindex2[j] );
			}
			loopShape.vindex2 = vindex;

			fill.loops.RemoveIndexFast( hole );
		}
	}
}

/*
========================
idSWFShapeParser::TriangulateSoup
========================
*/
void idSWFShapeParser::TriangulateSoup( idSWFShape& shape )
{

	MakeLoops();

	// Now turn the (potentially) concave line loops into triangles by using ear clipping

	shape.fillDraws.SetNum( fillDraws.Num() );
	for( int i = 0; i < fillDraws.Num(); i++ )
	{
		swfSPDrawFill_t& spDrawFill = fillDraws[i];
		idSWFShapeDrawFill& drawFill = shape.fillDraws[i];

		swfFillStyle_t& style = spDrawFill.style;
		drawFill.style = spDrawFill.style;

		for( int j = 0; j < spDrawFill.loops.Num(); j++ )
		{
			swfSPLineLoop_t& loop = spDrawFill.loops[j];
			int numVerts = loop.vindex1.Num();
			for( int k = 0; k < numVerts - 2; k++ )
			{
				int v1 = FindEarVert( loop );
				if( v1 == -1 )
				{
					idLib::Warning( "idSWFShapeParser: could not find an ear vert" );
					break;
				}
				int num = loop.vindex1.Num();
				int v2 = ( v1 + 1 ) % num;
				int v3 = ( v1 + 2 ) % num;

				AddUniqueVert( drawFill, verts[ loop.vindex1[ v1 ] ], verts[ loop.vindex2[ v1 ] ] );
				AddUniqueVert( drawFill, verts[ loop.vindex1[ v2 ] ], verts[ loop.vindex2[ v2 ] ] );
				AddUniqueVert( drawFill, verts[ loop.vindex1[ v3 ] ], verts[ loop.vindex2[ v3 ] ] );

				loop.vindex1.RemoveIndex( v2 );
				loop.vindex2.RemoveIndex( v2 );
			}
		}
	}
}

/*
========================
idSWFShapeParser::TriangulateSoup
========================
*/
void idSWFShapeParser::TriangulateSoup( idSWFFontGlyph& shape )
{

	MakeLoops();

	// Now turn the (potentially) concave line loops into triangles by using ear clipping

	assert( fillDraws.Num() == 1 );
	swfSPDrawFill_t& spDrawFill = fillDraws[0];
	for( int j = 0; j < spDrawFill.loops.Num(); j++ )
	{
		swfSPLineLoop_t& loop = spDrawFill.loops[j];
		int numVerts = loop.vindex1.Num();
		for( int k = 0; k < numVerts - 2; k++ )
		{
			int v1 = FindEarVert( loop );
			if( v1 == -1 )
			{
				idLib::Warning( "idSWFShapeParser: could not find an ear vert" );
				break;
			}
			int num = loop.vindex1.Num();
			int v2 = ( v1 + 1 ) % num;
			int v3 = ( v1 + 2 ) % num;

			shape.indices.Append( shape.verts.AddUnique( verts[ loop.vindex1[ v1 ] ] ) );
			shape.indices.Append( shape.verts.AddUnique( verts[ loop.vindex1[ v2 ] ] ) );
			shape.indices.Append( shape.verts.AddUnique( verts[ loop.vindex1[ v3 ] ] ) );

			loop.vindex1.RemoveIndex( v2 );
			loop.vindex2.RemoveIndex( v2 );
		}
	}
}

struct earVert_t
{
	int i1;
	int i2;
	int i3;
	float cross;
};
class idSort_Ears : public idSort_Quick< earVert_t, idSort_Ears >
{
public:
	int Compare( const earVert_t& a, const earVert_t& b ) const
	{
		if( a.cross < b.cross )
		{
			return -1;
		}
		else if( a.cross > b.cross )
		{
			return 1;
		}
		return 0;
	}
};

/*
========================
idSWFShapeParser::FindEarVert
========================
*/
int idSWFShapeParser::FindEarVert( const swfSPLineLoop_t& loop )
{
	assert( loop.vindex1.Num() == loop.vindex2.Num() );
	int num = loop.vindex1.Num();

	idList<earVert_t> ears;
	ears.SetNum( num );

	for( int i1 = 0; i1 < num; i1++ )
	{
		int i2 = ( i1 + 1 ) % num;
		int i3 = ( i1 + 2 ) % num;
		const idVec2& v1s = verts[ loop.vindex1[ i1 ] ];
		const idVec2& v2s = verts[ loop.vindex1[ i2 ] ];
		const idVec2& v3s = verts[ loop.vindex1[ i3 ] ];

		idVec2 a = v1s - v2s;
		idVec2 b = v2s - v3s;

		ears[i1].cross = a.x * b.y - a.y * b.x;
		ears[i1].i1 = i1;
		ears[i1].i2 = i2;
		ears[i1].i3 = i3;
	}
	ears.SortWithTemplate( idSort_Ears() );

	for( int i = 0; i < ears.Num(); i++ )
	{
		if( ears[i].cross < 0.0f )
		{
			continue;
		}
		int i1 = ears[i].i1;
		int i2 = ears[i].i2;
		int i3 = ears[i].i3;

		const idVec2& v1s = verts[ loop.vindex1[ i1 ] ];
		const idVec2& v2s = verts[ loop.vindex1[ i2 ] ];
		const idVec2& v3s = verts[ loop.vindex1[ i3 ] ];

		const idVec2& v1e = verts[ loop.vindex2[ i1 ] ];
		const idVec2& v2e = verts[ loop.vindex2[ i2 ] ];
		const idVec2& v3e = verts[ loop.vindex2[ i3 ] ];

		idMat3 edgeEquations1;
		edgeEquations1[0].Set( v1s.x, v1s.y, 1.0f );
		edgeEquations1[1].Set( v2s.x, v2s.y, 1.0f );
		edgeEquations1[2].Set( v3s.x, v3s.y, 1.0f );

		idMat3 edgeEquations2;
		edgeEquations2[0].Set( v1e.x, v1e.y, 1.0f );
		edgeEquations2[1].Set( v2e.x, v2e.y, 1.0f );
		edgeEquations2[2].Set( v3e.x, v3e.y, 1.0f );

		edgeEquations1.InverseSelf();
		edgeEquations2.InverseSelf();

		bool isEar = true;
		for( int j = 0; j < num; j++ )
		{
			if( j == i1 || j == i2 || j == i3 )
			{
				continue;
			}

			idVec3 p1;
			p1.ToVec2() = verts[ loop.vindex1[j] ];
			p1.z = 1.0f;

			idVec3 signs1 = p1 * edgeEquations1;

			bool b1x = signs1.x > 0;
			bool b1y = signs1.y > 0;
			bool b1z = signs1.z > 0;
			if( b1x == b1y && b1x == b1z )
			{
				// point inside
				isEar = false;
				break;
			}

			idVec3 p2;
			p2.ToVec2() = verts[ loop.vindex2[j] ];
			p2.z = 1.0f;

			idVec3 signs2 = p2 * edgeEquations2;

			bool b2x = signs2.x > 0;
			bool b2y = signs2.y > 0;
			bool b2z = signs2.z > 0;
			if( b2x == b2y && b2x == b2z )
			{
				// point inside
				isEar = false;
				break;
			}
		}
		if( isEar )
		{
			return i1;
		}
	}
	return -1;
}

/*
========================
idSWFShapeParser::AddUniqueVert
========================
*/
void idSWFShapeParser::AddUniqueVert( idSWFShapeDrawFill& drawFill, const idVec2& start, const idVec2& end )
{
	if( morph )
	{
		for( int i = 0; i < drawFill.startVerts.Num(); i++ )
		{
			if( drawFill.startVerts[i] == start && drawFill.endVerts[i] == end )
			{
				drawFill.indices.Append( i );
				return;
			}
		}
		int index1 = drawFill.startVerts.Append( start );
		int index2 = drawFill.endVerts.Append( end );
		assert( index1 == index2 );

		drawFill.indices.Append( index1 );
	}
	else
	{
		drawFill.indices.Append( drawFill.startVerts.AddUnique( start ) );
	}
}

/*
========================
idSWFShapeParser::ReadFillStyle
========================
*/
void idSWFShapeParser::ReadFillStyle( idSWFBitStream& bitstream )
{
	uint16 fillStyleCount = bitstream.ReadU8();
	if( extendedCount && fillStyleCount == 0xFF )
	{
		fillStyleCount = bitstream.ReadU16();
	}

	for( int i = 0; i < fillStyleCount; i++ )
	{
		uint8 fillStyleType = bitstream.ReadU8();

		swfFillStyle_t& fillStyle = fillDraws.Alloc().style;
		fillStyle.type = fillStyleType >> 4;
		fillStyle.subType = fillStyleType & 0xF;

		if( fillStyle.type == 0 )
		{
			if( morph )
			{
				bitstream.ReadColorRGBA( fillStyle.startColor );
				bitstream.ReadColorRGBA( fillStyle.endColor );
			}
			else
			{
				if( rgba )
				{
					bitstream.ReadColorRGBA( fillStyle.startColor );
				}
				else
				{
					bitstream.ReadColorRGB( fillStyle.startColor );
				}
				fillStyle.endColor = fillStyle.startColor;
			}
		}
		else if( fillStyle.type == 1 )
		{
			bitstream.ReadMatrix( fillStyle.startMatrix );
			if( morph )
			{
				bitstream.ReadMatrix( fillStyle.endMatrix );
				bitstream.ReadMorphGradient( fillStyle.gradient );
			}
			else
			{
				fillStyle.endMatrix = fillStyle.startMatrix;
				bitstream.ReadGradient( fillStyle.gradient, rgba );
			}
			if( fillStyle.subType == 3 )
			{
				assert( morph == false ); // focal gradients aren't allowed in morph shapes
				fillStyle.focalPoint = bitstream.ReadFixed8();
			}
		}
		else if( fillStyle.type == 4 )
		{
			fillStyle.bitmapID = bitstream.ReadU16();
			bitstream.ReadMatrix( fillStyle.startMatrix );
			if( morph )
			{
				bitstream.ReadMatrix( fillStyle.endMatrix );
			}
			else
			{
				fillStyle.endMatrix = fillStyle.startMatrix;
			}
		}
	}

	uint16 lineStyleCount = bitstream.ReadU8();
	if( extendedCount && lineStyleCount == 0xFF )
	{
		lineStyleCount = bitstream.ReadU16();
	}

	lineDraws.SetNum( lineDraws.Num() + lineStyleCount );
	lineDraws.SetNum( 0 );
	for( int i = 0; i < lineStyleCount; i++ )
	{
		swfLineStyle_t& lineStyle = lineDraws.Alloc().style;
		lineStyle.startWidth = bitstream.ReadU16();
		if( lineStyle2 )
		{
			lineStyle.endWidth = lineStyle.startWidth;

			uint8 startCapStyle = bitstream.ReadU( 2 );
			uint8 joinStyle = bitstream.ReadU( 2 );
			bool hasFillFlag = bitstream.ReadBool();
			bool noHScaleFlag = bitstream.ReadBool();
			bool noVScaleFlag = bitstream.ReadBool();
			bool pixelHintingFlag = bitstream.ReadBool();
			uint8 reserved = bitstream.ReadU( 5 );
			bool noClose = bitstream.ReadBool();
			uint8 endCapStyle = bitstream.ReadU( 2 );
			if( joinStyle == 2 )
			{
				uint16 miterLimitFactor = bitstream.ReadU16();
			}
			if( hasFillFlag )
			{
				// FIXME: read fill style
				idLib::Warning( "idSWFShapeParser: Ignoring hasFillFlag" );
			}
			else
			{
				bitstream.ReadColorRGBA( lineStyle.startColor );
				lineStyle.endColor = lineStyle.startColor;
			}
		}
		else
		{
			if( morph )
			{
				lineStyle.endWidth = bitstream.ReadU16();
			}
			else
			{
				lineStyle.endWidth = lineStyle.startWidth;
			}
			if( rgba )
			{
				bitstream.ReadColorRGBA( lineStyle.startColor );
			}
			else
			{
				bitstream.ReadColorRGB( lineStyle.startColor );
			}
			if( morph )
			{
				bitstream.ReadColorRGBA( lineStyle.endColor );
			}
			else
			{
				lineStyle.endColor = lineStyle.startColor;
			}
		}
	}
}
