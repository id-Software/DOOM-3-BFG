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

#ifndef __TRACEMODEL_H__
#define __TRACEMODEL_H__

/*
===============================================================================

	A trace model is an arbitrary polygonal model which is used by the
	collision detection system to find collisions, contacts or the contents
	of a volume. For collision detection speed reasons the number of vertices
	and edges are limited. The trace model can have any shape. However convex
	models are usually preferred.

===============================================================================
*/

class idVec3;
class idMat3;
class idBounds;

// trace model type
typedef enum
{
	TRM_INVALID,		// invalid trm
	TRM_BOX,			// box
	TRM_OCTAHEDRON,		// octahedron
	TRM_DODECAHEDRON,	// dodecahedron
	TRM_CYLINDER,		// cylinder approximation
	TRM_CONE,			// cone approximation
	TRM_BONE,			// two tetrahedrons attached to each other
	TRM_POLYGON,		// arbitrary convex polygon
	TRM_POLYGONVOLUME,	// volume for arbitrary convex polygon
	TRM_CUSTOM			// loaded from map model or ASE/LWO
} traceModel_t;

// these are bit cache limits
#define MAX_TRACEMODEL_VERTS		32
#define MAX_TRACEMODEL_EDGES		32
#define MAX_TRACEMODEL_POLYS		16
#define MAX_TRACEMODEL_POLYEDGES	16

typedef idVec3 traceModelVert_t;

typedef struct
{
	int					v[2];
	idVec3				normal;
} traceModelEdge_t;

typedef struct
{
	idVec3				normal;
	float				dist;
	idBounds			bounds;
	int					numEdges;
	int					edges[MAX_TRACEMODEL_POLYEDGES];
} traceModelPoly_t;

class idTraceModel
{

public:
	traceModel_t		type;
	int					numVerts;
	traceModelVert_t	verts[MAX_TRACEMODEL_VERTS];
	int					numEdges;
	traceModelEdge_t	edges[MAX_TRACEMODEL_EDGES + 1];
	int					numPolys;
	traceModelPoly_t	polys[MAX_TRACEMODEL_POLYS];
	idVec3				offset;			// offset to center of model
	idBounds			bounds;			// bounds of model
	bool				isConvex;		// true when model is convex

public:
	idTraceModel();
	// axial bounding box
	idTraceModel( const idBounds& boxBounds );
	// cylinder approximation
	idTraceModel( const idBounds& cylBounds, const int numSides );
	// bone
	idTraceModel( const float length, const float width );

	// axial box
	void				SetupBox( const idBounds& boxBounds );
	void				SetupBox( const float size );
	// octahedron
	void				SetupOctahedron( const idBounds& octBounds );
	void				SetupOctahedron( const float size );
	// dodecahedron
	void				SetupDodecahedron( const idBounds& dodBounds );
	void				SetupDodecahedron( const float size );
	// cylinder approximation
	void				SetupCylinder( const idBounds& cylBounds, const int numSides );
	void				SetupCylinder( const float height, const float width, const int numSides );
	// cone approximation
	void				SetupCone( const idBounds& coneBounds, const int numSides );
	void				SetupCone( const float height, const float width, const int numSides );
	// two tetrahedrons attached to each other
	void				SetupBone( const float length, const float width );
	// arbitrary convex polygon
	void				SetupPolygon( const idVec3* v, const int count );
	void				SetupPolygon( const idWinding& w );
	// generate edge normals
	int					GenerateEdgeNormals();
	// translate the trm
	void				Translate( const idVec3& translation );
	// rotate the trm
	void				Rotate( const idMat3& rotation );
	// shrink the model m units on all sides
	void				Shrink( const float m );
	// compare
	bool				Compare( const idTraceModel& trm ) const;
	bool				operator==(	const idTraceModel& trm ) const;
	bool				operator!=(	const idTraceModel& trm ) const;
	// get the area of one of the polygons
	float				GetPolygonArea( int polyNum ) const;
	// get the silhouette edges
	int					GetProjectionSilhouetteEdges( const idVec3& projectionOrigin, int silEdges[MAX_TRACEMODEL_EDGES] ) const;
	int					GetParallelProjectionSilhouetteEdges( const idVec3& projectionDir, int silEdges[MAX_TRACEMODEL_EDGES] ) const;
	// calculate mass properties assuming an uniform density
	void				GetMassProperties( const float density, float& mass, idVec3& centerOfMass, idMat3& inertiaTensor ) const;

private:
	void				InitBox();
	void				InitOctahedron();
	void				InitDodecahedron();
	void				InitBone();

	void				ProjectionIntegrals( int polyNum, int a, int b, struct projectionIntegrals_s& integrals ) const;
	void				PolygonIntegrals( int polyNum, int a, int b, int c, struct polygonIntegrals_s& integrals ) const;
	void				VolumeIntegrals( struct volumeIntegrals_s& integrals ) const;
	void				VolumeFromPolygon( idTraceModel& trm, float thickness ) const;
	int					GetOrderedSilhouetteEdges( const int edgeIsSilEdge[MAX_TRACEMODEL_EDGES + 1], int silEdges[MAX_TRACEMODEL_EDGES] ) const;
};


ID_INLINE idTraceModel::idTraceModel()
{
	type = TRM_INVALID;
	numVerts = numEdges = numPolys = 0;
	bounds.Zero();
}

ID_INLINE idTraceModel::idTraceModel( const idBounds& boxBounds )
{
	InitBox();
	SetupBox( boxBounds );
}

ID_INLINE idTraceModel::idTraceModel( const idBounds& cylBounds, const int numSides )
{
	SetupCylinder( cylBounds, numSides );
}

ID_INLINE idTraceModel::idTraceModel( const float length, const float width )
{
	InitBone();
	SetupBone( length, width );
}

ID_INLINE bool idTraceModel::operator==( const idTraceModel& trm ) const
{
	return Compare( trm );
}

ID_INLINE bool idTraceModel::operator!=( const idTraceModel& trm ) const
{
	return !Compare( trm );
}

#endif /* !__TRACEMODEL_H__ */

