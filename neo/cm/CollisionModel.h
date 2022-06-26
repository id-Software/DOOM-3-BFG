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

#ifndef __COLLISIONMODELMANAGER_H__
#define __COLLISIONMODELMANAGER_H__

/*
===============================================================================

	Trace model vs. polygonal model collision detection.

	Short translations are the least expensive. Retrieving contact points is
	about as cheap as a short translation. Position tests are more expensive
	and rotations are most expensive.

	There is no position test at the start of a translation or rotation. In other
	words if a translation with start != end or a rotation with angle != 0 starts
	in solid, this goes unnoticed and the collision result is undefined.

	A translation with start == end or a rotation with angle == 0 performs
	a position test and fills in the trace_t structure accordingly.

===============================================================================
*/

// contact type
typedef enum
{
	CONTACT_NONE,							// no contact
	CONTACT_EDGE,							// trace model edge hits model edge
	CONTACT_MODELVERTEX,					// model vertex hits trace model polygon
	CONTACT_TRMVERTEX						// trace model vertex hits model polygon
} contactType_t;

// contact info
typedef struct
{
	contactType_t			type;			// contact type
	idVec3					point;			// point of contact
	idVec3					normal;			// contact plane normal
	float					dist;			// contact plane distance
	int						contents;		// contents at other side of surface
	const idMaterial* 		material;		// surface material
	int						modelFeature;	// contact feature on model
	int						trmFeature;		// contact feature on trace model
	int						entityNum;		// entity the contact surface is a part of
	int						id;				// id of clip model the contact surface is part of
} contactInfo_t;

// trace result
typedef struct trace_s
{
	float					fraction;		// fraction of movement completed, 1.0 = didn't hit anything
	idVec3					endpos;			// final position of trace model
	idMat3					endAxis;		// final axis of trace model
	contactInfo_t			c;				// contact information, only valid if fraction < 1.0
} trace_t;

typedef int cmHandle_t;

#define CM_CLIP_EPSILON		0.25f			// always stay this distance away from any model
#define CM_BOX_EPSILON		1.0f			// should always be larger than clip epsilon
#define CM_MAX_TRACE_DIST	4096.0f			// maximum distance a trace model may be traced, point traces are unlimited

class idCollisionModelManager
{
public:
	virtual					~idCollisionModelManager() {}

	// Loads collision models from a map file.
	virtual void			LoadMap( const idMapFile* mapFile, bool ignoreOldCollisionFile ) = 0;
	// Frees all the collision models.
	virtual void			FreeMap() = 0;

	virtual void			Preload( const char* mapName ) = 0;

	// Gets the clip handle for a model.
	virtual cmHandle_t		LoadModel( const char* modelName, const bool precache ) = 0;

	// Sets up a trace model for collision with other trace models.
	virtual cmHandle_t		SetupTrmModel( const idTraceModel& trm, const idMaterial* material ) = 0;

	// Creates a trace model from a collision model, returns true if succesfull.
	virtual bool			TrmFromModel( const char* modelName, idTraceModel& trm ) = 0;

	// Gets the name of a model.
	virtual const char* 	GetModelName( cmHandle_t model ) const = 0;

	// Gets the bounds of a model.
	virtual bool			GetModelBounds( cmHandle_t model, idBounds& bounds ) const = 0;

	// Gets all contents flags of brushes and polygons of a model ored together.
	virtual bool			GetModelContents( cmHandle_t model, int& contents ) const = 0;

	// Gets a vertex of a model.
	virtual bool			GetModelVertex( cmHandle_t model, int vertexNum, idVec3& vertex ) const = 0;

	// Gets an edge of a model.
	virtual bool			GetModelEdge( cmHandle_t model, int edgeNum, idVec3& start, idVec3& end ) const = 0;

	// Gets a polygon of a model.
	virtual bool			GetModelPolygon( cmHandle_t model, int polygonNum, idFixedWinding& winding ) const = 0;

	// Translates a trace model and reports the first collision if any.
	virtual void			Translation( trace_t* results, const idVec3& start, const idVec3& end,
										 const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
										 cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis ) = 0;

	// Rotates a trace model and reports the first collision if any.
	virtual void			Rotation( trace_t* results, const idVec3& start, const idRotation& rotation,
									  const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
									  cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis ) = 0;

	// Returns the contents touched by the trace model or 0 if the trace model is in free space.
	virtual int				Contents( const idVec3& start,
									  const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
									  cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis ) = 0;

	// Stores all contact points of the trace model with the model, returns the number of contacts.
	virtual int				Contacts( contactInfo_t* contacts, const int maxContacts, const idVec3& start, const idVec6& dir, const float depth,
									  const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
									  cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis ) = 0;

	// Tests collision detection.
	virtual void			DebugOutput( const idVec3& origin ) = 0;

	// Draws a model.
	virtual void			DrawModel( cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis,
									   const idVec3& viewOrigin, const float radius ) = 0;

	// Prints model information, use -1 handle for accumulated model info.
	virtual void			ModelInfo( cmHandle_t model ) = 0;

	// Lists all loaded models.
	virtual void			ListModels() = 0;

	// Writes a collision model file for the given map entity.
	virtual bool			WriteCollisionModelForMapEntity( const idMapEntity* mapEnt, const char* filename, const bool testTraceModel = true ) = 0;
};

extern idCollisionModelManager* 		collisionModelManager;

#endif /* !__COLLISIONMODELMANAGER_H__ */
