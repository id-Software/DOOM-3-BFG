/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans
Copyright (C) 2016-2017 Dustin Land

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
#include "Model_local.h"

#if defined(USE_INTRINSICS_SSE)
static const __m128 vector_float_posInfinity		= { idMath::INFINITUM, idMath::INFINITUM, idMath::INFINITUM, idMath::INFINITUM };
static const __m128 vector_float_negInfinity		= { -idMath::INFINITUM, -idMath::INFINITUM, -idMath::INFINITUM, -idMath::INFINITUM };
#endif

static const char* MD5_SnapshotName = "_MD5_Snapshot_";

static const byte MD5B_VERSION = 106;
static const unsigned int MD5B_MAGIC = ( '5' << 24 ) | ( 'D' << 16 ) | ( 'M' << 8 ) | MD5B_VERSION;

idCVar r_useGPUSkinning( "r_useGPUSkinning", "1", CVAR_INTEGER, "animate normals and tangents instead of deriving" );

/***********************************************************************

	idMD5Mesh

***********************************************************************/

static int c_numVerts = 0;
static int c_numWeights = 0;
static int c_numWeightJoints = 0;

struct vertexWeight_t
{
	int							joint;
	idVec3						offset;
	float						jointWeight;
};

/*
====================
idMD5Mesh::idMD5Mesh
====================
*/
idMD5Mesh::idMD5Mesh()
{
	shader				= NULL;
	numVerts			= 0;
	numTris				= 0;
	meshJoints			= NULL;
	numMeshJoints		= 0;
	maxJointVertDist	= 0.0f;
	deformInfo			= NULL;
	surfaceNum			= 0;
}

/*
====================
idMD5Mesh::~idMD5Mesh
====================
*/
idMD5Mesh::~idMD5Mesh()
{
	if( meshJoints != NULL )
	{
		Mem_Free( meshJoints );
		meshJoints = NULL;
	}
	if( deformInfo != NULL )
	{
		R_FreeDeformInfo( deformInfo );
		deformInfo = NULL;
	}
}

/*
====================
idMD5Mesh::ParseMesh
====================
*/
void idMD5Mesh::ParseMesh( idLexer& parser, int numJoints, const idJointMat* joints )
{
	idToken		token;
	idToken		name;

	parser.ExpectTokenString( "{" );

	//
	// parse name
	//
	if( parser.CheckTokenString( "name" ) )
	{
		parser.ReadToken( &name );
	}

	//
	// parse shader
	//
	parser.ExpectTokenString( "shader" );

	parser.ReadToken( &token );
	idStr shaderName = token;

	shader = declManager->FindMaterial( shaderName );

	//
	// parse texture coordinates
	//
	parser.ExpectTokenString( "numverts" );
	int count = parser.ParseInt();
	if( count < 0 )
	{
		parser.Error( "Invalid size: %s", token.c_str() );
	}

	this->numVerts = count;

	idList<idVec2> texCoords;
	idList<int> firstWeightForVertex;
	idList<int> numWeightsForVertex;

	texCoords.SetNum( count );
	firstWeightForVertex.SetNum( count );
	numWeightsForVertex.SetNum( count );

	int numWeights = 0;
	int maxweight = 0;
	for( int i = 0; i < texCoords.Num(); i++ )
	{
		parser.ExpectTokenString( "vert" );
		parser.ParseInt();

		parser.Parse1DMatrix( 2, texCoords[ i ].ToFloatPtr() );

		firstWeightForVertex[ i ]	= parser.ParseInt();
		numWeightsForVertex[ i ]	= parser.ParseInt();

		if( !numWeightsForVertex[ i ] )
		{
			parser.Error( "Vertex without any joint weights." );
		}

		numWeights += numWeightsForVertex[ i ];
		if( numWeightsForVertex[ i ] + firstWeightForVertex[ i ] > maxweight )
		{
			maxweight = numWeightsForVertex[ i ] + firstWeightForVertex[ i ];
		}
	}

	//
	// parse tris
	//
	parser.ExpectTokenString( "numtris" );
	count = parser.ParseInt();
	if( count < 0 )
	{
		parser.Error( "Invalid size: %d", count );
	}

	idList<int> tris;
	tris.SetNum( count * 3 );
	numTris = count;
	for( int i = 0; i < count; i++ )
	{
		parser.ExpectTokenString( "tri" );
		parser.ParseInt();

		tris[ i * 3 + 0 ] = parser.ParseInt();
		tris[ i * 3 + 1 ] = parser.ParseInt();
		tris[ i * 3 + 2 ] = parser.ParseInt();
	}

	//
	// parse weights
	//
	parser.ExpectTokenString( "numweights" );
	count = parser.ParseInt();
	if( count < 0 )
	{
		parser.Error( "Invalid size: %d", count );
	}

	if( maxweight > count )
	{
		parser.Warning( "Vertices reference out of range weights in model (%d of %d weights).", maxweight, count );
	}

	idList<vertexWeight_t> tempWeights;
	tempWeights.SetNum( count );
	assert( numJoints < 256 );		// so we can pack into bytes

	for( int i = 0; i < count; i++ )
	{
		parser.ExpectTokenString( "weight" );
		parser.ParseInt();

		int jointnum = parser.ParseInt();
		if( ( jointnum < 0 ) || ( jointnum >= numJoints ) )
		{
			parser.Error( "Joint Index out of range(%d): %d", numJoints, jointnum );
		}

		tempWeights[ i ].joint			= jointnum;
		tempWeights[ i ].jointWeight	= parser.ParseFloat();

		parser.Parse1DMatrix( 3, tempWeights[ i ].offset.ToFloatPtr() );
	}

	// create pre-scaled weights and an index for the vertex/joint lookup
	idVec4* scaledWeights = ( idVec4* ) Mem_Alloc16( numWeights * sizeof( scaledWeights[0] ), TAG_MD5_WEIGHT );
	int* weightIndex = ( int* ) Mem_Alloc16( numWeights * 2 * sizeof( weightIndex[0] ), TAG_MD5_INDEX );
	memset( weightIndex, 0, numWeights * 2 * sizeof( weightIndex[0] ) );

	count = 0;
	for( int i = 0; i < texCoords.Num(); i++ )
	{
		int num = firstWeightForVertex[i];
		for( int j = 0; j < numWeightsForVertex[i]; j++, num++, count++ )
		{
			scaledWeights[count].ToVec3() = tempWeights[num].offset * tempWeights[num].jointWeight;
			scaledWeights[count].w = tempWeights[num].jointWeight;
			weightIndex[count * 2 + 0] = tempWeights[num].joint * sizeof( idJointMat );
		}
		weightIndex[count * 2 - 1] = 1;
	}

	parser.ExpectTokenString( "}" );

	// update counters
	c_numVerts += texCoords.Num();
	c_numWeights += numWeights;
	c_numWeightJoints++;
	for( int i = 0; i < numWeights; i++ )
	{
		c_numWeightJoints += weightIndex[i * 2 + 1];
	}

	//
	// build a base pose that can be used for skinning
	//
	idDrawVert* basePose = ( idDrawVert* )Mem_ClearedAlloc( texCoords.Num() * sizeof( *basePose ), TAG_MD5_BASE );
	for( int j = 0, i = 0; i < texCoords.Num(); i++ )
	{
		idVec3 v = ( *( idJointMat* )( ( byte* )joints + weightIndex[j * 2 + 0] ) ) * scaledWeights[j];
		while( weightIndex[j * 2 + 1] == 0 )
		{
			j++;
			v += ( *( idJointMat* )( ( byte* )joints + weightIndex[j * 2 + 0] ) ) * scaledWeights[j];
		}
		j++;

		basePose[i].Clear();
		basePose[i].xyz = v;
		basePose[i].SetTexCoord( texCoords[i] );
	}

	// build the weights and bone indexes into the verts, so they will be duplicated
	// as necessary at mirror seems

	static int maxWeightsPerVert;
	static float maxResidualWeight;

	const int MAX_VERTEX_WEIGHTS = 4;

	idList< bool > jointIsUsed;
	jointIsUsed.SetNum( numJoints );
	for( int i = 0; i < jointIsUsed.Num(); i++ )
	{
		jointIsUsed[i] = false;
	}

	numMeshJoints = 0;
	maxJointVertDist = 0.0f;

	//-----------------------------------------
	// new-style setup for fixed four weights and normal / tangent deformation
	//
	// Several important models have >25% residual weight in joints after the
	// first four, which is worrisome for using a fixed four joint deformation.
	//-----------------------------------------
	for( int i = 0; i < texCoords.Num(); i++ )
	{
		idDrawVert& dv = basePose[i];

		// some models do have >4 joint weights, so it is necessary to sort and renormalize

		// sort the weights and take the four largest
		int	weights[256];
		const int numWeights = numWeightsForVertex[ i ];
		for( int j = 0; j < numWeights; j++ )
		{
			weights[j] = firstWeightForVertex[i] + j;
		}
		// bubble sort
		for( int j = 0; j < numWeights; j++ )
		{
			for( int k = 0; k < numWeights - 1 - j; k++ )
			{
				if( tempWeights[weights[k]].jointWeight < tempWeights[weights[k + 1]].jointWeight )
				{
					SwapValues( weights[k], weights[k + 1] );
				}
			}
		}

		if( numWeights > maxWeightsPerVert )
		{
			maxWeightsPerVert = numWeights;
		}

		const int usedWeights = Min( MAX_VERTEX_WEIGHTS, numWeights );

		float totalWeight = 0;
		for( int j = 0; j < numWeights; j++ )
		{
			totalWeight += tempWeights[weights[j]].jointWeight;
		}
		assert( totalWeight > 0.999f && totalWeight < 1.001f );

		float usedWeight = 0;
		for( int j = 0; j < usedWeights; j++ )
		{
			usedWeight += tempWeights[weights[j]].jointWeight;
		}

		const float residualWeight = totalWeight - usedWeight;
		if( residualWeight > maxResidualWeight )
		{
			maxResidualWeight = residualWeight;
		}

		byte finalWeights[MAX_VERTEX_WEIGHTS] = { 0 };
		byte finalJointIndecies[MAX_VERTEX_WEIGHTS] = { 0 };
		for( int j = 0; j < usedWeights; j++ )
		{
			const vertexWeight_t& weight = tempWeights[weights[j]];
			const int jointIndex = weight.joint;
			const float fw = weight.jointWeight;
			assert( fw >= 0.0f && fw <= 1.0f );
			const float normalizedWeight = fw / usedWeight;
			finalWeights[j] = idMath::Ftob( normalizedWeight * 255.0f );
			finalJointIndecies[j] = jointIndex;
		}

		// Sort the weights and indices for hardware skinning
		for( int k = 0; k < 3; ++k )
		{
			for( int l = k + 1; l < 4; ++l )
			{
				if( finalWeights[l] > finalWeights[k] )
				{
					SwapValues( finalWeights[k], finalWeights[l] );
					SwapValues( finalJointIndecies[k], finalJointIndecies[l] );
				}
			}
		}

		// Give any left over to the biggest weight
		finalWeights[0] += Max( 255 - finalWeights[0] - finalWeights[1] - finalWeights[2] - finalWeights[3], 0 );

		dv.color[0] = finalJointIndecies[0];
		dv.color[1] = finalJointIndecies[1];
		dv.color[2] = finalJointIndecies[2];
		dv.color[3] = finalJointIndecies[3];

		dv.color2[0] = finalWeights[0];
		dv.color2[1] = finalWeights[1];
		dv.color2[2] = finalWeights[2];
		dv.color2[3] = finalWeights[3];

		for( int j = usedWeights; j < 4; j++ )
		{
			assert( dv.color2[j] == 0 );
		}

		for( int j = 0; j < usedWeights; j++ )
		{
			if( !jointIsUsed[finalJointIndecies[j]] )
			{
				jointIsUsed[finalJointIndecies[j]] = true;
				numMeshJoints++;
			}
			const idJointMat& joint = joints[finalJointIndecies[j]];
			float dist = ( dv.xyz - joint.GetTranslation() ).Length();
			if( dist > maxJointVertDist )
			{
				maxJointVertDist = dist;
			}
		}
	}

	meshJoints = ( byte* ) Mem_Alloc( numMeshJoints * sizeof( meshJoints[0] ), TAG_MODEL );
	numMeshJoints = 0;
	for( int i = 0; i < numJoints; i++ )
	{
		if( jointIsUsed[i] )
		{
			meshJoints[numMeshJoints++] = i;
		}
	}

	// build the deformInfo and collect a final base pose with the mirror
	// seam verts properly including the bone weights
	deformInfo = R_BuildDeformInfo( texCoords.Num(), basePose, tris.Num(), tris.Ptr(),
									shader->UseUnsmoothedTangents() );

	for( int i = 0; i < deformInfo->numOutputVerts; i++ )
	{
		for( int j = 0; j < 4; j++ )
		{
			if( deformInfo->verts[i].color[j] >= numJoints )
			{
				idLib::FatalError( "Bad joint index" );
			}
		}
	}

	Mem_Free( basePose );
}

/*
============
TransformVertsAndTangents
============
*/
void TransformVertsAndTangents( idDrawVert* targetVerts, const int numVerts, const idDrawVert* baseVerts, const idJointMat* joints )
{
	for( int i = 0; i < numVerts; i++ )
	{
		const idDrawVert& base = baseVerts[i];

		const idJointMat& j0 = joints[base.color[0]];
		const idJointMat& j1 = joints[base.color[1]];
		const idJointMat& j2 = joints[base.color[2]];
		const idJointMat& j3 = joints[base.color[3]];

		const float w0 = base.color2[0] * ( 1.0f / 255.0f );
		const float w1 = base.color2[1] * ( 1.0f / 255.0f );
		const float w2 = base.color2[2] * ( 1.0f / 255.0f );
		const float w3 = base.color2[3] * ( 1.0f / 255.0f );

		idJointMat accum;
		idJointMat::Mul( accum, j0, w0 );
		idJointMat::Mad( accum, j1, w1 );
		idJointMat::Mad( accum, j2, w2 );
		idJointMat::Mad( accum, j3, w3 );

		targetVerts[i].xyz = accum * idVec4( base.xyz.x, base.xyz.y, base.xyz.z, 1.0f );
		targetVerts[i].SetNormal( accum * base.GetNormal() );
		targetVerts[i].SetTangent( accum * base.GetTangent() );
		targetVerts[i].tangent[3] = base.tangent[3];
	}
}

/*
====================
idMD5Mesh::UpdateSurface
====================
*/
void idMD5Mesh::UpdateSurface( const struct renderEntity_s* ent, const idJointMat* entJoints,
							   const idJointMat* entJointsInverted, modelSurface_t* surf )
{

	tr.pc.c_deformedSurfaces++;
	tr.pc.c_deformedVerts += deformInfo->numOutputVerts;
	tr.pc.c_deformedIndexes += deformInfo->numIndexes;

	surf->shader = shader;

	if( surf->geometry != NULL )
	{
		// if the number of verts and indexes are the same we can re-use the triangle surface
		if( surf->geometry->numVerts == deformInfo->numOutputVerts && surf->geometry->numIndexes == deformInfo->numIndexes )
		{
			R_FreeStaticTriSurfVertexCaches( surf->geometry );
		}
		else
		{
			R_FreeStaticTriSurf( surf->geometry );
			surf->geometry = R_AllocStaticTriSurf();
		}
	}
	else
	{
		surf->geometry = R_AllocStaticTriSurf();
	}

	srfTriangles_t* tri = surf->geometry;

	// note that some of the data is referenced, and should not be freed
	tri->referencedIndexes = true;
	tri->numIndexes = deformInfo->numIndexes;
	tri->indexes = deformInfo->indexes;
	tri->silIndexes = deformInfo->silIndexes;
	tri->numMirroredVerts = deformInfo->numMirroredVerts;
	tri->mirroredVerts = deformInfo->mirroredVerts;
	tri->numDupVerts = deformInfo->numDupVerts;
	tri->dupVerts = deformInfo->dupVerts;
	//tri->numSilEdges = deformInfo->numSilEdges;
	//tri->silEdges = deformInfo->silEdges;

	tri->indexCache = deformInfo->staticIndexCache;

	tri->numVerts = deformInfo->numOutputVerts;

	// RB: added check wether GPU skinning is available at all
	if( r_useGPUSkinning.GetBool() )
	{
		if( tri->verts != NULL && tri->verts != deformInfo->verts )
		{
			R_FreeStaticTriSurfVerts( tri );
		}
		tri->verts = deformInfo->verts;
		tri->ambientCache = deformInfo->staticAmbientCache;
		tri->referencedVerts = true;
	}
	else
	{
		if( tri->verts == NULL || tri->verts == deformInfo->verts )
		{
			tri->verts = NULL;
			R_AllocStaticTriSurfVerts( tri, deformInfo->numOutputVerts );
			assert( tri->verts != NULL );	// quiet analyze warning
			memcpy( tri->verts, deformInfo->verts, deformInfo->numOutputVerts * sizeof( deformInfo->verts[0] ) );	// copy over the texture coordinates
		}
		TransformVertsAndTangents( tri->verts, deformInfo->numOutputVerts, deformInfo->verts, entJointsInverted );
		tri->referencedVerts = false;
	}
	tri->tangentsCalculated = true;

	CalculateBounds( entJoints, tri->bounds );
}

/*
====================
idMD5Mesh::CalculateBounds
====================
*/
void idMD5Mesh::CalculateBounds( const idJointMat* entJoints, idBounds& bounds ) const
{
#if defined(USE_INTRINSICS_SSE)

	__m128 minX = vector_float_posInfinity;
	__m128 minY = vector_float_posInfinity;
	__m128 minZ = vector_float_posInfinity;
	__m128 maxX = vector_float_negInfinity;
	__m128 maxY = vector_float_negInfinity;
	__m128 maxZ = vector_float_negInfinity;
	for( int i = 0; i < numMeshJoints; i++ )
	{
		const idJointMat& joint = entJoints[meshJoints[i]];
		__m128 x = _mm_load_ps( joint.ToFloatPtr() + 0 * 4 );
		__m128 y = _mm_load_ps( joint.ToFloatPtr() + 1 * 4 );
		__m128 z = _mm_load_ps( joint.ToFloatPtr() + 2 * 4 );
		minX = _mm_min_ps( minX, x );
		minY = _mm_min_ps( minY, y );
		minZ = _mm_min_ps( minZ, z );
		maxX = _mm_max_ps( maxX, x );
		maxY = _mm_max_ps( maxY, y );
		maxZ = _mm_max_ps( maxZ, z );
	}
	__m128 expand = _mm_splat_ps( _mm_load_ss( & maxJointVertDist ), 0 );
	minX = _mm_sub_ps( minX, expand );
	minY = _mm_sub_ps( minY, expand );
	minZ = _mm_sub_ps( minZ, expand );
	maxX = _mm_add_ps( maxX, expand );
	maxY = _mm_add_ps( maxY, expand );
	maxZ = _mm_add_ps( maxZ, expand );
	_mm_store_ss( bounds.ToFloatPtr() + 0, _mm_splat_ps( minX, 3 ) );
	_mm_store_ss( bounds.ToFloatPtr() + 1, _mm_splat_ps( minY, 3 ) );
	_mm_store_ss( bounds.ToFloatPtr() + 2, _mm_splat_ps( minZ, 3 ) );
	_mm_store_ss( bounds.ToFloatPtr() + 3, _mm_splat_ps( maxX, 3 ) );
	_mm_store_ss( bounds.ToFloatPtr() + 4, _mm_splat_ps( maxY, 3 ) );
	_mm_store_ss( bounds.ToFloatPtr() + 5, _mm_splat_ps( maxZ, 3 ) );

#else

	bounds.Clear();
	for( int i = 0; i < numMeshJoints; i++ )
	{
		const idJointMat& joint = entJoints[meshJoints[i]];
		bounds.AddPoint( joint.GetTranslation() );
	}
	bounds.ExpandSelf( maxJointVertDist );

#endif
}

/*
====================
idMD5Mesh::NearestJoint
====================
*/
int idMD5Mesh::NearestJoint( int a, int b, int c ) const
{
	// duplicated vertices might not have weights
	int vertNum;
	if( a >= 0 && a < numVerts )
	{
		vertNum = a;
	}
	else if( b >= 0 && b < numVerts )
	{
		vertNum = b;
	}
	else if( c >= 0 && c < numVerts )
	{
		vertNum = c;
	}
	else
	{
		// all vertices are duplicates which shouldn't happen
		return 0;
	}

	const idDrawVert& v = deformInfo->verts[vertNum];

	int bestWeight = 0;
	int bestJoint = 0;
	for( int i = 0; i < 4; i++ )
	{
		if( v.color2[i] > bestWeight )
		{
			bestWeight = v.color2[i];
			bestJoint = v.color[i];
		}
	}

	return bestJoint;
}

/***********************************************************************

	idRenderModelMD5

***********************************************************************/

/*
====================
idRenderModelMD5::ParseJoint
====================
*/
void idRenderModelMD5::ParseJoint( idLexer& parser, idMD5Joint* joint, idJointQuat* defaultPose )
{
	//
	// parse name
	//
	idToken	token;
	parser.ReadToken( &token );
	joint->name = token;

	//
	// parse parent
	//
	int num = parser.ParseInt();
	if( num < 0 )
	{
		joint->parent = NULL;
	}
	else
	{
		if( num >= joints.Num() - 1 )
		{
			parser.Error( "Invalid parent for joint '%s'", joint->name.c_str() );
		}
		joint->parent = &joints[ num ];
	}

	//
	// parse default pose
	//
	parser.Parse1DMatrix( 3, defaultPose->t.ToFloatPtr() );
	parser.Parse1DMatrix( 3, defaultPose->q.ToFloatPtr() );
	defaultPose->q.w = defaultPose->q.CalcW();
}

/*
====================
idRenderModelMD5::InitFromFile
====================
*/
void idRenderModelMD5::InitFromFile( const char* fileName, const idImportOptions* options )
{
	name = fileName;
	LoadModel();
}

/*
========================
idRenderModelMD5::LoadBinaryModel
========================
*/
bool idRenderModelMD5::LoadBinaryModel( idFile* file, const ID_TIME_T sourceTimeStamp )
{
	if( !idRenderModelStatic::LoadBinaryModel( file, sourceTimeStamp ) )
	{
		return false;
	}

	unsigned int magic = 0;
	file->ReadBig( magic );
	if( magic != MD5B_MAGIC )
	{
		return false;
	}

	int tempNum;
	file->ReadBig( tempNum );
	joints.SetNum( tempNum );
	for( int i = 0; i < joints.Num(); i++ )
	{
		file->ReadString( joints[i].name );
		int offset;
		file->ReadBig( offset );
		if( offset >= 0 )
		{
			joints[i].parent = joints.Ptr() + offset;
		}
		else
		{
			joints[i].parent = NULL;
		}
	}

	file->ReadBig( tempNum );
	defaultPose.SetNum( tempNum );
	for( int i = 0; i < defaultPose.Num(); i++ )
	{
		file->ReadBig( defaultPose[i].q.x );
		file->ReadBig( defaultPose[i].q.y );
		file->ReadBig( defaultPose[i].q.z );
		file->ReadBig( defaultPose[i].q.w );
		file->ReadVec3( defaultPose[i].t );
	}

	file->ReadBig( tempNum );
	invertedDefaultPose.SetNum( tempNum );
	for( int i = 0; i < invertedDefaultPose.Num(); i++ )
	{
		file->ReadBigArray( invertedDefaultPose[ i ].ToFloatPtr(), JOINTMAT_TYPESIZE );
	}
	SIMD_INIT_LAST_JOINT( invertedDefaultPose.Ptr(), joints.Num() );

	file->ReadBig( tempNum );
	meshes.SetNum( tempNum );
	for( int i = 0; i < meshes.Num(); i++ )
	{

		idStr materialName;
		file->ReadString( materialName );
		if( materialName.IsEmpty() )
		{
			meshes[i].shader = NULL;
		}
		else
		{
			meshes[i].shader = declManager->FindMaterial( materialName );
		}

		file->ReadBig( meshes[i].numVerts );
		file->ReadBig( meshes[i].numTris );

		file->ReadBig( meshes[i].numMeshJoints );
		meshes[i].meshJoints = ( byte* ) Mem_Alloc( meshes[i].numMeshJoints * sizeof( meshes[i].meshJoints[0] ), TAG_MODEL );
		file->ReadBigArray( meshes[i].meshJoints, meshes[i].numMeshJoints );
		file->ReadBig( meshes[i].maxJointVertDist );

		meshes[i].deformInfo = ( deformInfo_t* )R_ClearedStaticAlloc( sizeof( deformInfo_t ) );
		deformInfo_t& deform = *meshes[i].deformInfo;

		file->ReadBig( deform.numSourceVerts );
		file->ReadBig( deform.numOutputVerts );
		file->ReadBig( deform.numIndexes );
		file->ReadBig( deform.numMirroredVerts );
		file->ReadBig( deform.numDupVerts );

		int numSilEdges;
		file->ReadBig( numSilEdges );

		srfTriangles_t	tri;
		memset( &tri, 0, sizeof( srfTriangles_t ) );

		if( deform.numOutputVerts > 0 )
		{
			R_AllocStaticTriSurfVerts( &tri, deform.numOutputVerts );
			deform.verts = tri.verts;
			file->ReadBigArray( deform.verts, deform.numOutputVerts );
		}

		if( deform.numIndexes > 0 )
		{
			R_AllocStaticTriSurfIndexes( &tri, deform.numIndexes );
			R_AllocStaticTriSurfSilIndexes( &tri, deform.numIndexes );
			deform.indexes = tri.indexes;
			deform.silIndexes = tri.silIndexes;
			file->ReadBigArray( deform.indexes, deform.numIndexes );
			file->ReadBigArray( deform.silIndexes, deform.numIndexes );
		}

		if( deform.numMirroredVerts > 0 )
		{
			R_AllocStaticTriSurfMirroredVerts( &tri, deform.numMirroredVerts );
			deform.mirroredVerts = tri.mirroredVerts;
			file->ReadBigArray( deform.mirroredVerts, deform.numMirroredVerts );
		}

		if( deform.numDupVerts > 0 )
		{
			R_AllocStaticTriSurfDupVerts( &tri, deform.numDupVerts );
			deform.dupVerts = tri.dupVerts;
			file->ReadBigArray( deform.dupVerts, deform.numDupVerts * 2 );
		}
// jmarshall - compatibility
		if( numSilEdges > 0 )
		{
			for( int j = 0; j < numSilEdges; j++ )
			{
				triIndex_t stub;
				file->ReadBig( stub );
				file->ReadBig( stub );
				file->ReadBig( stub );
				file->ReadBig( stub );
			}
		}
// jmarshall end

		file->ReadBig( meshes[i].surfaceNum );
	}

	return true;
}

/*
========================
idRenderModelMD5::WriteBinaryModel
========================
*/
void idRenderModelMD5::WriteBinaryModel( idFile* file, ID_TIME_T* _timeStamp ) const
{

	idRenderModelStatic::WriteBinaryModel( file );

	if( file == NULL )
	{
		return;
	}

	file->WriteBig( MD5B_MAGIC );

	file->WriteBig( joints.Num() );
	for( int i = 0; i < joints.Num(); i++ )
	{
		file->WriteString( joints[i].name );
		int offset = -1;
		if( joints[i].parent != NULL )
		{
			offset = joints[i].parent - joints.Ptr();
		}
		file->WriteBig( offset );
	}

	file->WriteBig( defaultPose.Num() );
	for( int i = 0; i < defaultPose.Num(); i++ )
	{
		file->WriteBig( defaultPose[i].q.x );
		file->WriteBig( defaultPose[i].q.y );
		file->WriteBig( defaultPose[i].q.z );
		file->WriteBig( defaultPose[i].q.w );
		file->WriteVec3( defaultPose[i].t );
	}

	file->WriteBig( invertedDefaultPose.Num() );
	for( int i = 0; i < invertedDefaultPose.Num(); i++ )
	{
		file->WriteBigArray( invertedDefaultPose[ i ].ToFloatPtr(), JOINTMAT_TYPESIZE );
	}

	file->WriteBig( meshes.Num() );
	for( int i = 0; i < meshes.Num(); i++ )
	{

		if( meshes[i].shader != NULL && meshes[i].shader->GetName() != NULL )
		{
			file->WriteString( meshes[i].shader->GetName() );
		}
		else
		{
			file->WriteString( "" );
		}

		file->WriteBig( meshes[i].numVerts );
		file->WriteBig( meshes[i].numTris );

		file->WriteBig( meshes[i].numMeshJoints );
		file->WriteBigArray( meshes[i].meshJoints, meshes[i].numMeshJoints );
		file->WriteBig( meshes[i].maxJointVertDist );

		deformInfo_t& deform = *meshes[i].deformInfo;

		file->WriteBig( deform.numSourceVerts );
		file->WriteBig( deform.numOutputVerts );
		file->WriteBig( deform.numIndexes );
		file->WriteBig( deform.numMirroredVerts );
		file->WriteBig( deform.numDupVerts );
		int silDummy = 0;
		file->WriteBig( silDummy ); // deform.numSilEdges

		if( deform.numOutputVerts > 0 )
		{
			file->WriteBigArray( deform.verts, deform.numOutputVerts );
		}

		if( deform.numIndexes > 0 )
		{
			file->WriteBigArray( deform.indexes, deform.numIndexes );
			file->WriteBigArray( deform.silIndexes, deform.numIndexes );
		}

		if( deform.numMirroredVerts > 0 )
		{
			file->WriteBigArray( deform.mirroredVerts, deform.numMirroredVerts );
		}

		if( deform.numDupVerts > 0 )
		{
			file->WriteBigArray( deform.dupVerts, deform.numDupVerts * 2 );
		}

		file->WriteBig( meshes[i].surfaceNum );
	}
}

/*
====================
idRenderModelMD5::LoadModel

used for initial loads, reloadModel, and reloading the data of purged models
Upon exit, the model will absolutely be valid, but possibly as a default model
====================
*/
void idRenderModelMD5::LoadModel()
{

	int			version;
	int			num;
	int			parentNum;
	idToken		token;
	idLexer		parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS );

	if( !purged )
	{
		PurgeModel();
	}
	purged = false;

	if( !parser.LoadFile( name ) )
	{
		MakeDefaultModel();
		return;
	}

	parser.ExpectTokenString( MD5_VERSION_STRING );
	version = parser.ParseInt();

	if( version != MD5_VERSION )
	{
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD5_VERSION );
	}

	//
	// skip commandline
	//
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	// parse num joints
	parser.ExpectTokenString( "numJoints" );
	num  = parser.ParseInt();
	joints.SetGranularity( 1 );
	joints.SetNum( num );
	defaultPose.SetGranularity( 1 );
	defaultPose.SetNum( num );

	// parse num meshes
	parser.ExpectTokenString( "numMeshes" );
	num = parser.ParseInt();
	if( num < 0 )
	{
		parser.Error( "Invalid size: %d", num );
	}
	meshes.SetGranularity( 1 );
	meshes.SetNum( num );

	//
	// parse joints
	//
	parser.ExpectTokenString( "joints" );
	parser.ExpectTokenString( "{" );
	idJointMat* poseMat = ( idJointMat* )_alloca16( joints.Num() * sizeof( poseMat[0] ) );
	for( int i = 0; i < joints.Num(); i++ )
	{
		idMD5Joint* joint = &joints[i];
		idJointQuat*	 pose = &defaultPose[i];

		ParseJoint( parser, joint, pose );
		poseMat[ i ].SetRotation( pose->q.ToMat3() );
		poseMat[ i ].SetTranslation( pose->t );
		if( joint->parent )
		{
			parentNum = joint->parent - joints.Ptr();
			pose->q = ( poseMat[ i ].ToMat3() * poseMat[ parentNum ].ToMat3().Transpose() ).ToQuat();
			pose->t = ( poseMat[ i ].ToVec3() - poseMat[ parentNum ].ToVec3() ) * poseMat[ parentNum ].ToMat3().Transpose();
		}
	}
	parser.ExpectTokenString( "}" );

	//-----------------------------------------
	// create the inverse of the base pose joints to support tech6 style deformation
	// of base pose vertexes, normals, and tangents.
	//
	// vertex * joints * inverseJoints == vertex when joints is the base pose
	// When the joints are in another pose, it gives the animated vertex position
	//-----------------------------------------
	invertedDefaultPose.SetNum( SIMD_ROUND_JOINTS( joints.Num() ) );
	for( int i = 0; i < joints.Num(); i++ )
	{
		invertedDefaultPose[i] = poseMat[i];
		invertedDefaultPose[i].Invert();
	}
	SIMD_INIT_LAST_JOINT( invertedDefaultPose.Ptr(), joints.Num() );

	for( int i = 0; i < meshes.Num(); i++ )
	{
		parser.ExpectTokenString( "mesh" );
		meshes[i].ParseMesh( parser, defaultPose.Num(), poseMat );
	}

	// calculate the bounds of the model
	bounds.Clear();
	for( int i = 0; i < meshes.Num(); i++ )
	{
		idBounds meshBounds;
		meshes[i].CalculateBounds( poseMat, meshBounds );
		bounds.AddBounds( meshBounds );
	}

	// set the timestamp for reloadmodels
	fileSystem->ReadFile( name, NULL, &timeStamp );

	common->UpdateLevelLoadPacifier();
}

void idRenderModelMD5::CreateBuffers( nvrhi::ICommandList* commandList )
{
	for( int i = 0; i < meshes.Num(); i++ )
	{
		auto& deform = *meshes[i].deformInfo;
		R_CreateDeformStaticVertices( meshes[i].deformInfo, commandList );
	}
}

/*
==============
idRenderModelMD5::Print
==============
*/
void idRenderModelMD5::Print() const
{
	common->Printf( "%s\n", name.c_str() );
	common->Printf( "Dynamic model.\n" );
	common->Printf( "Generated smooth normals.\n" );
	common->Printf( "    verts  tris weights material\n" );
	int	totalVerts = 0;
	int	totalTris = 0;
	const idMD5Mesh* mesh = meshes.Ptr();
	for( int i = 0; i < meshes.Num(); i++, mesh++ )
	{
		totalVerts += mesh->NumVerts();
		totalTris += mesh->NumTris();
		common->Printf( "%2i: %5i %5i %s\n", i, mesh->NumVerts(), mesh->NumTris(), mesh->shader->GetName() );
	}
	common->Printf( "-----\n" );
	common->Printf( "%4i verts.\n", totalVerts );
	common->Printf( "%4i tris.\n", totalTris );
	common->Printf( "%4i joints.\n", joints.Num() );
}

/*
==============
idRenderModelMD5::List
==============
*/
void idRenderModelMD5::List() const
{
	int			i;
	const idMD5Mesh*	mesh;
	int			totalTris = 0;
	int			totalVerts = 0;

	for( mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++ )
	{
		totalTris += mesh->numTris;
		totalVerts += mesh->NumVerts();
	}
	common->Printf( " %4ik %3i %4i %4i %s(MD5)", Memory() / 1024, meshes.Num(), totalVerts, totalTris, Name() );

	if( defaulted )
	{
		common->Printf( " (DEFAULTED)" );
	}

	common->Printf( "\n" );
}

/*
====================
idRenderModelMD5::Bounds

This calculates a rough bounds by using the joint radii without
transforming all the points
====================
*/
idBounds idRenderModelMD5::Bounds( const renderEntity_t* ent ) const
{
	if( ent == NULL )
	{
		// this is the bounds for the reference pose
		return bounds;
	}

	return ent->bounds;
}

/*
====================
idRenderModelMD5::DrawJoints
====================
*/
void idRenderModelMD5::DrawJoints( const renderEntity_t* ent, const viewDef_t* view ) const
{
	int					i;
	int					num;
	idVec3				pos;
	const idJointMat*	joint;
	const idMD5Joint*	md5Joint;
	int					parentNum;

	num = ent->numJoints;
	joint = ent->joints;
	md5Joint = joints.Ptr();
	for( i = 0; i < num; i++, joint++, md5Joint++ )
	{
		pos = ent->origin + joint->ToVec3() * ent->axis;
		if( md5Joint->parent )
		{
			parentNum = md5Joint->parent - joints.Ptr();
			common->RW()->DebugLine( colorWhite, ent->origin + ent->joints[ parentNum ].ToVec3() * ent->axis, pos );
		}

		common->RW()->DebugLine( colorRed,	pos, pos + joint->ToMat3()[ 0 ] * 2.0f * ent->axis );
		common->RW()->DebugLine( colorGreen, pos, pos + joint->ToMat3()[ 1 ] * 2.0f * ent->axis );
		common->RW()->DebugLine( colorBlue,	pos, pos + joint->ToMat3()[ 2 ] * 2.0f * ent->axis );
	}

	idBounds bounds;

	bounds.FromTransformedBounds( ent->bounds, vec3_zero, ent->axis );
	common->RW()->DebugBounds( colorMagenta, bounds, ent->origin );

	if( ( r_jointNameScale.GetFloat() != 0.0f ) && ( bounds.Expand( 128.0f ).ContainsPoint( view->renderView.vieworg - ent->origin ) ) )
	{
		idVec3	offset( 0, 0, r_jointNameOffset.GetFloat() );
		float	scale;

		scale = r_jointNameScale.GetFloat();
		joint = ent->joints;
		num = ent->numJoints;
		for( i = 0; i < num; i++, joint++ )
		{
			pos = ent->origin + joint->ToVec3() * ent->axis;
			common->RW()->DrawText( joints[ i ].name, pos + offset, scale, colorWhite, view->renderView.viewaxis, 1 );
		}
	}
}

/*
====================
TransformJoints
====================
*/
static void TransformJoints( idJointMat* __restrict outJoints, const int numJoints, const idJointMat* __restrict inJoints1, const idJointMat* __restrict inJoints2 )
{

	float* outFloats = outJoints->ToFloatPtr();
	const float* inFloats1 = inJoints1->ToFloatPtr();
	const float* inFloats2 = inJoints2->ToFloatPtr();

	assert_16_byte_aligned( outFloats );
	assert_16_byte_aligned( inFloats1 );
	assert_16_byte_aligned( inFloats2 );

#if defined(USE_INTRINSICS_SSE)

	const __m128 mask_keep_last = __m128c( _mm_set_epi32( 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 ) );

	for( int i = 0; i < numJoints; i += 2, inFloats1 += 2 * 12, inFloats2 += 2 * 12, outFloats += 2 * 12 )
	{
		__m128 m1a0 = _mm_load_ps( inFloats1 + 0 * 12 + 0 );
		__m128 m1b0 = _mm_load_ps( inFloats1 + 0 * 12 + 4 );
		__m128 m1c0 = _mm_load_ps( inFloats1 + 0 * 12 + 8 );
		__m128 m1a1 = _mm_load_ps( inFloats1 + 1 * 12 + 0 );
		__m128 m1b1 = _mm_load_ps( inFloats1 + 1 * 12 + 4 );
		__m128 m1c1 = _mm_load_ps( inFloats1 + 1 * 12 + 8 );

		__m128 m2a0 = _mm_load_ps( inFloats2 + 0 * 12 + 0 );
		__m128 m2b0 = _mm_load_ps( inFloats2 + 0 * 12 + 4 );
		__m128 m2c0 = _mm_load_ps( inFloats2 + 0 * 12 + 8 );
		__m128 m2a1 = _mm_load_ps( inFloats2 + 1 * 12 + 0 );
		__m128 m2b1 = _mm_load_ps( inFloats2 + 1 * 12 + 4 );
		__m128 m2c1 = _mm_load_ps( inFloats2 + 1 * 12 + 8 );

		__m128 tj0 = _mm_and_ps( m1a0, mask_keep_last );
		__m128 tk0 = _mm_and_ps( m1b0, mask_keep_last );
		__m128 tl0 = _mm_and_ps( m1c0, mask_keep_last );
		__m128 tj1 = _mm_and_ps( m1a1, mask_keep_last );
		__m128 tk1 = _mm_and_ps( m1b1, mask_keep_last );
		__m128 tl1 = _mm_and_ps( m1c1, mask_keep_last );

		__m128 ta0 = _mm_splat_ps( m1a0, 0 );
		__m128 td0 = _mm_splat_ps( m1b0, 0 );
		__m128 tg0 = _mm_splat_ps( m1c0, 0 );
		__m128 ta1 = _mm_splat_ps( m1a1, 0 );
		__m128 td1 = _mm_splat_ps( m1b1, 0 );
		__m128 tg1 = _mm_splat_ps( m1c1, 0 );

		__m128 ra0 = _mm_add_ps( tj0, _mm_mul_ps( ta0, m2a0 ) );
		__m128 rd0 = _mm_add_ps( tk0, _mm_mul_ps( td0, m2a0 ) );
		__m128 rg0 = _mm_add_ps( tl0, _mm_mul_ps( tg0, m2a0 ) );
		__m128 ra1 = _mm_add_ps( tj1, _mm_mul_ps( ta1, m2a1 ) );
		__m128 rd1 = _mm_add_ps( tk1, _mm_mul_ps( td1, m2a1 ) );
		__m128 rg1 = _mm_add_ps( tl1, _mm_mul_ps( tg1, m2a1 ) );

		__m128 tb0 = _mm_splat_ps( m1a0, 1 );
		__m128 te0 = _mm_splat_ps( m1b0, 1 );
		__m128 th0 = _mm_splat_ps( m1c0, 1 );
		__m128 tb1 = _mm_splat_ps( m1a1, 1 );
		__m128 te1 = _mm_splat_ps( m1b1, 1 );
		__m128 th1 = _mm_splat_ps( m1c1, 1 );

		__m128 rb0 = _mm_add_ps( ra0, _mm_mul_ps( tb0, m2b0 ) );
		__m128 re0 = _mm_add_ps( rd0, _mm_mul_ps( te0, m2b0 ) );
		__m128 rh0 = _mm_add_ps( rg0, _mm_mul_ps( th0, m2b0 ) );
		__m128 rb1 = _mm_add_ps( ra1, _mm_mul_ps( tb1, m2b1 ) );
		__m128 re1 = _mm_add_ps( rd1, _mm_mul_ps( te1, m2b1 ) );
		__m128 rh1 = _mm_add_ps( rg1, _mm_mul_ps( th1, m2b1 ) );

		__m128 tc0 = _mm_splat_ps( m1a0, 2 );
		__m128 tf0 = _mm_splat_ps( m1b0, 2 );
		__m128 ti0 = _mm_splat_ps( m1c0, 2 );
		__m128 tf1 = _mm_splat_ps( m1b1, 2 );
		__m128 ti1 = _mm_splat_ps( m1c1, 2 );
		__m128 tc1 = _mm_splat_ps( m1a1, 2 );

		__m128 rc0 = _mm_add_ps( rb0, _mm_mul_ps( tc0, m2c0 ) );
		__m128 rf0 = _mm_add_ps( re0, _mm_mul_ps( tf0, m2c0 ) );
		__m128 ri0 = _mm_add_ps( rh0, _mm_mul_ps( ti0, m2c0 ) );
		__m128 rc1 = _mm_add_ps( rb1, _mm_mul_ps( tc1, m2c1 ) );
		__m128 rf1 = _mm_add_ps( re1, _mm_mul_ps( tf1, m2c1 ) );
		__m128 ri1 = _mm_add_ps( rh1, _mm_mul_ps( ti1, m2c1 ) );

		_mm_store_ps( outFloats + 0 * 12 + 0, rc0 );
		_mm_store_ps( outFloats + 0 * 12 + 4, rf0 );
		_mm_store_ps( outFloats + 0 * 12 + 8, ri0 );
		_mm_store_ps( outFloats + 1 * 12 + 0, rc1 );
		_mm_store_ps( outFloats + 1 * 12 + 4, rf1 );
		_mm_store_ps( outFloats + 1 * 12 + 8, ri1 );
	}

#else

	for( int i = 0; i < numJoints; i++ )
	{
		idJointMat::Multiply( outJoints[i], inJoints1[i], inJoints2[i] );
	}

#endif
}

/*
====================
idRenderModelMD5::InstantiateDynamicModel
====================
*/
idRenderModel* idRenderModelMD5::InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel )
{
	if( cachedModel != NULL && !r_useCachedDynamicModels.GetBool() )
	{
		delete cachedModel;
		cachedModel = NULL;
	}

	if( purged )
	{
		common->DWarning( "model %s instantiated while purged", Name() );
		LoadModel();
	}

	if( !ent->joints )
	{
		common->Printf( "idRenderModelMD5::InstantiateDynamicModel: NULL joints on renderEntity for '%s'\n", Name() );
		delete cachedModel;
		return NULL;
	}
	else if( ent->numJoints != joints.Num() )
	{
		common->Printf( "idRenderModelMD5::InstantiateDynamicModel: renderEntity has different number of joints than model for '%s'\n", Name() );
		delete cachedModel;
		return NULL;
	}

	tr.pc.c_generateMd5++;

	idRenderModelStatic* staticModel;
	if( cachedModel != NULL )
	{
		assert( dynamic_cast<idRenderModelStatic*>( cachedModel ) != NULL );
		assert( idStr::Icmp( cachedModel->Name(), MD5_SnapshotName ) == 0 );
		staticModel = static_cast<idRenderModelStatic*>( cachedModel );
	}
	else
	{
		staticModel = new( TAG_MODEL ) idRenderModelStatic;
		staticModel->InitEmpty( MD5_SnapshotName );
	}

	staticModel->bounds.Clear();

	if( r_showSkel.GetInteger() )
	{
		if( ( view != NULL ) && ( !r_skipSuppress.GetBool() || !ent->suppressSurfaceInViewID || ( ent->suppressSurfaceInViewID != view->renderView.viewID ) ) )
		{
			// only draw the skeleton
			DrawJoints( ent, view );
		}

		if( r_showSkel.GetInteger() > 1 )
		{
			// turn off the model when showing the skeleton
			staticModel->InitEmpty( MD5_SnapshotName );
			return staticModel;
		}
	}

	// update the GPU joints array
	const int numInvertedJoints = SIMD_ROUND_JOINTS( joints.Num() );
	if( staticModel->jointsInverted == NULL )
	{
		staticModel->numInvertedJoints = numInvertedJoints;
		staticModel->jointsInverted = ( idJointMat* )Mem_ClearedAlloc( numInvertedJoints * sizeof( idJointMat ), TAG_JOINTMAT );
		staticModel->jointsInvertedBuffer = 0;
	}
	else
	{
		assert( staticModel->numInvertedJoints == numInvertedJoints );
	}

	TransformJoints( staticModel->jointsInverted, joints.Num(), ent->joints, invertedDefaultPose.Ptr() );

	// create all the surfaces
	idMD5Mesh* mesh = meshes.Ptr();
	for( int i = 0; i < meshes.Num(); i++, mesh++ )
	{
		// avoid deforming the surface if it will be a nodraw due to a skin remapping
		const idMaterial* shader = mesh->shader;

		shader = R_RemapShaderBySkin( shader, ent->customSkin, ent->customShader );

		if( !shader || ( !shader->IsDrawn() && !shader->SurfaceCastsShadow() ) )
		{
			staticModel->DeleteSurfaceWithId( i );
			mesh->surfaceNum = -1;
			continue;
		}

		modelSurface_t* surf;

		int surfaceNum = 0;
		if( staticModel->FindSurfaceWithId( i, surfaceNum ) )
		{
			mesh->surfaceNum = surfaceNum;
			surf = &staticModel->surfaces[surfaceNum];
		}
		else
		{
			mesh->surfaceNum = staticModel->NumSurfaces();
			surf = &staticModel->surfaces.Alloc();
			surf->geometry = NULL;
			surf->shader = NULL;
			surf->id = i;
		}

		mesh->UpdateSurface( ent, ent->joints, staticModel->jointsInverted, surf );
		assert( surf->geometry != NULL );	// to get around compiler warning

		// the deformation of the tangents can be deferred until each surface is added to the view
		surf->geometry->staticModelWithJoints = staticModel;

		staticModel->bounds.AddBounds( surf->geometry->bounds );
	}

	return staticModel;
}

/*
====================
idRenderModelMD5::IsDynamicModel
====================
*/
dynamicModel_t idRenderModelMD5::IsDynamicModel() const
{
	return DM_CACHED;
}

/*
====================
idRenderModelMD5::NumJoints
====================
*/
int idRenderModelMD5::NumJoints() const
{
	return joints.Num();
}

/*
====================
idRenderModelMD5::GetJoints
====================
*/
const idMD5Joint* idRenderModelMD5::GetJoints() const
{
	return joints.Ptr();
}

/*
====================
idRenderModelMD5::GetDefaultPose
====================
*/
const idJointQuat* idRenderModelMD5::GetDefaultPose() const
{
	return defaultPose.Ptr();
}

/*
====================
idRenderModelMD5::GetJointHandle
====================
*/
jointHandle_t idRenderModelMD5::GetJointHandle( const char* name ) const
{
	const idMD5Joint* joint = joints.Ptr();
	for( int i = 0; i < joints.Num(); i++, joint++ )
	{
		if( idStr::Icmp( joint->name.c_str(), name ) == 0 )
		{
			return ( jointHandle_t )i;
		}
	}

	return INVALID_JOINT;
}

/*
=====================
idRenderModelMD5::GetJointName
=====================
*/
const char* idRenderModelMD5::GetJointName( jointHandle_t handle ) const
{
	if( ( handle < 0 ) || ( handle >= joints.Num() ) )
	{
		return "<invalid joint>";
	}

	return joints[ handle ].name;
}

/*
====================
idRenderModelMD5::NearestJoint
====================
*/
int idRenderModelMD5::NearestJoint( int surfaceNum, int a, int b, int c ) const
{
	if( surfaceNum > meshes.Num() )
	{
		common->Error( "idRenderModelMD5::NearestJoint: surfaceNum > meshes.Num()" );
	}

	const idMD5Mesh* mesh = meshes.Ptr();
	for( int i = 0; i < meshes.Num(); i++, mesh++ )
	{
		if( mesh->surfaceNum == surfaceNum )
		{
			return mesh->NearestJoint( a, b, c );
		}
	}
	return 0;
}

/*
====================
idRenderModelMD5::TouchData

models that are already loaded at level start time
will still touch their materials to make sure they
are kept loaded
====================
*/
void idRenderModelMD5::TouchData()
{
	for( int i = 0; i < meshes.Num(); i++ )
	{
		declManager->FindMaterial( meshes[i].shader->GetName() );
	}
}

/*
===================
idRenderModelMD5::PurgeModel

frees all the data, but leaves the class around for dangling references,
which can regenerate the data with LoadModel()
===================
*/
void idRenderModelMD5::PurgeModel()
{
	purged = true;
	joints.Clear();
	defaultPose.Clear();
	meshes.Clear();
}

/*
===================
idRenderModelMD5::Memory
===================
*/
int	idRenderModelMD5::Memory() const
{
	int total = sizeof( *this );
	total += joints.MemoryUsed() + defaultPose.MemoryUsed() + meshes.MemoryUsed();

	// count up strings
	for( int i = 0; i < joints.Num(); i++ )
	{
		total += joints[i].name.DynamicMemoryUsed();
	}

	// count up meshes
	for( int i = 0; i < meshes.Num(); i++ )
	{
		const idMD5Mesh* mesh = &meshes[i];

		total += mesh->numMeshJoints * sizeof( mesh->meshJoints[0] );

		// sum up deform info
		total += sizeof( mesh->deformInfo );
		total += R_DeformInfoMemoryUsed( mesh->deformInfo );
	}
	return total;
}


// RB begin
void idRenderModelMD5::ExportOBJ( idFile* objFile, idFile* mtlFile, ID_TIME_T* _timeStamp )
{
	if( objFile == NULL || mtlFile == NULL )
	{
		common->Printf( "Failed to ExportOBJ\n" );
		return;
	}

	renderEntity_t			ent;

	memset( &ent, 0, sizeof( ent ) );

	ent.bounds.Clear();
	ent.suppressSurfaceInViewID = 0;

	ent.numJoints = NumJoints();
	if( ent.numJoints > 0 )
	{
		ent.joints = ( idJointMat* )Mem_ClearedAlloc( SIMD_ROUND_JOINTS( ent.numJoints ) * sizeof( *ent.joints ), TAG_JOINTMAT );

		SIMD_INIT_LAST_JOINT( ent.joints, ent.numJoints );

		idRenderModel* newmodel = InstantiateDynamicModel( &ent, NULL, NULL );
		newmodel->ExportOBJ( objFile, mtlFile, _timeStamp );

		Mem_Free16( ent.joints );
		ent.joints = NULL;

		delete newmodel;
	}
}
// RB end
