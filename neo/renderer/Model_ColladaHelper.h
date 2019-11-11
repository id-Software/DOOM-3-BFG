/** Helper structures for the Collada loader */

/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
Copyright (C) 2012 Robert Beckebans (id Tech 4 integration)
All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

#ifndef __MODEL_COLLADAHELPER_H__
#define __MODEL_COLLADAHELPER_H__

namespace Collada
{

/** Collada file versions which evolved during the years ... */
enum FormatVersion
{
	FV_1_5_n,
	FV_1_4_n,
	FV_1_3_n
};


/** Transformation types that can be applied to a node */
enum TransformType
{
	TF_LOOKAT,
	TF_ROTATE,
	TF_TRANSLATE,
	TF_SCALE,
	TF_SKEW,
	TF_MATRIX
};

/** Different types of input data to a vertex or face */
enum InputType
{
	IT_Invalid,
	IT_Vertex,  // special type for per-index data referring to the <vertices> element carrying the per-vertex data.
	IT_Position,
	IT_Normal,
	IT_Texcoord,
	IT_Color,
	IT_Tangent,
	IT_Bitangent
};

/** Contains all data for one of the different transformation types */
struct Transform
{
	idStr mID;  ///< SID of the transform step, by which anim channels address their target node
	TransformType mType;
	float f[16]; ///< Interpretation of data depends on the type of the transformation
};

/** A collada camera. */
struct Camera
{
	Camera()
		:	mOrtho( false )
		,	mHorFov( 10e10f )
		,	mVerFov( 10e10f )
		,	mAspect( 10e10f )
		,	mZNear( 0.1f )
		,	mZFar( 1000.f )
	{}

	// Name of camera
	idStr mName;

	// True if it is an orthografic camera
	bool mOrtho;

	//! Horizontal field of view in degrees
	float mHorFov;

	//! Vertical field of view in degrees
	float mVerFov;

	//! Screen aspect
	float mAspect;

	//! Near& far z
	float mZNear, mZFar;
};

#define aiLightSource_AMBIENT 0xdeaddead

/** A collada light source. */
struct Light
{
	Light()
		:	mAttConstant( 1.f )
		,	mAttLinear( 0.f )
		,	mAttQuadratic( 0.f )
		,	mFalloffAngle( 180.f )
		,	mFalloffExponent( 0.f )
		,	mPenumbraAngle( 10e10f )
		,	mOuterAngle( 10e10f )
		,	mIntensity( 1.f )
	{}

	//! Type of the light source aiLightSourceType + ambient
	unsigned int mType;

	//! Color of the light
	idVec3 mColor;

	//! Light attenuation
	float mAttConstant, mAttLinear, mAttQuadratic;

	//! Spot light falloff
	float mFalloffAngle;
	float mFalloffExponent;

	// -----------------------------------------------------
	// FCOLLADA extension from here

	//! ... related stuff from maja and max extensions
	float mPenumbraAngle;
	float mOuterAngle;

	//! Common light intensity
	float mIntensity;
};

/** Short vertex index description */
struct InputSemanticMapEntry
{
	InputSemanticMapEntry()
		:	mSet( 0 )
	{}

	//! Index of set, optional
	unsigned int mSet;

	//! Name of referenced vertex input
	InputType mType;
};

/** Table to map from effect to vertex input semantics */
struct SemanticMappingTable
{
	//! Name of material
	idStr mMatName;

	//! List of semantic map commands, grouped by effect semantic name
	idHashTable<InputSemanticMapEntry> mMap;

	//! For std::find
	bool operator == ( const idStr& s ) const
	{
		return s == mMatName;
	}
};

/** A reference to a mesh inside a node, including materials assigned to the various subgroups.
 * The ID refers to either a mesh or a controller which specifies the mesh
 */
struct MeshInstance
{
	///< ID of the mesh or controller to be instanced
	idStr mMeshOrController;

	///< Map of materials by the subgroup ID they're applied to
	//idHashTable<SemanticMappingTable> mMaterials;
	idStrList mMaterials;
};

/** A reference to a camera inside a node*/
struct CameraInstance
{
	///< ID of the camera
	idStr mCamera;
};

/** A reference to a light inside a node*/
struct LightInstance
{
	///< ID of the camera
	idStr mLight;
};

/** A reference to a node inside a node*/
struct NodeInstance
{
	///< ID of the node
	idStr mNode;
};

/** A node in a scene hierarchy */
struct Node
{
	idStr mName;
	idStr mID;
	idStr mSID;
	Node* mParent;
	idList<Node*> mChildren;

	/** Operations in order to calculate the resulting transformation to parent. */
	idList<Transform> mTransforms;

	/** Meshes at this node */
	idList<MeshInstance> mMeshes;

	/** Lights at this node */
	idList<LightInstance> mLights;

	/** Cameras at this node */
	idList<CameraInstance> mCameras;

	/** Node instances at this node */
	idList<NodeInstance> mNodeInstances;

	/** Rootnodes: Name of primary camera, if any */
	idStr mPrimaryCamera;

	//! Constructor. Begin with a zero parent
	Node()
	{
		mParent = NULL;
	}

	//! Destructor: delete all children subsequently
	~Node()
	{
		//mChildren.DeleteContents( false );
	}
};

/** Data source array: either floats or strings */
struct Data
{
	bool mIsStringArray;
	idList<float> mValues;
	idList<idStr> mStrings;
};

/** Accessor to a data array */
struct Accessor
{
	size_t mCount;   // in number of objects
	size_t mSize;    // size of an object, in elements (floats or strings, mostly 1)
	size_t mOffset;  // in number of values
	size_t mStride;  // Stride in number of values
	idList<idStr> mParams; // names of the data streams in the accessors. Empty string tells to ignore.
	size_t mSubOffset[4]; // Suboffset inside the object for the common 4 elements. For a vector, thats XYZ, for a color RGBA and so on.
	// For example, SubOffset[0] denotes which of the values inside the object is the vector X component.
	idStr mSource;   // URL of the source array
	mutable const Data* mData; // Pointer to the source array, if resolved. NULL else

	Accessor()
	{
		mCount = 0;
		mSize = 0;
		mOffset = 0;
		mStride = 0;
		mData = NULL;
		mSubOffset[0] = mSubOffset[1] = mSubOffset[2] = mSubOffset[3] = 0;
	}
};

/** A single face in a mesh */
struct Face
{
	idList<size_t> mIndices;
};

/** An input channel for mesh data, referring to a single accessor */
struct InputChannel
{
	InputType mType;      // Type of the data
	size_t mIndex;		  // Optional index, if multiple sets of the same data type are given
	size_t mOffset;       // Index offset in the indices array of per-face indices. Don't ask, can't explain that any better.
	idStr mAccessor; // ID of the accessor where to read the actual values from.
	mutable const Accessor* mResolved; // Pointer to the accessor, if resolved. NULL else

	InputChannel()
	{
		mType = IT_Invalid;
		mIndex = 0;
		mOffset = 0;
		mResolved = NULL;
	}
};

/** Subset of a mesh with a certain material */
struct SubMesh
{
	idStr mMaterial; ///< subgroup identifier
	size_t mNumFaces; ///< number of faces in this submesh
};

/** Contains data for a single mesh */
struct Mesh
{
	Mesh()
	{
		//for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
		//	mNumUVComponents[i] = 2;
	}

	// just to check if there's some sophisticated addressing involved...
	// which we don't support, and therefore should warn about.
	idStr mVertexID;

	// Vertex data addressed by vertex indices
	idList<InputChannel> mPerVertexData;

	// actual mesh data, assembled on encounter of a <p> element. Verbose format, not indexed
	idList<idVec3> mPositions;
	idList<idVec3> mNormals;
	idList<idVec3> mTangents;
	idList<idVec3> mBitangents;
	idList<idVec2> mTexCoords;
	idList<dword> mColors;

	//unsigned int mNumUVComponents[AI_MAX_NUMBER_OF_TEXTURECOORDS];

	// Faces. Stored are only the number of vertices for each face.
	// 1 == point, 2 == line, 3 == triangle, 4+ == poly
	idList<int> mFaceSize;

	// Position indices for all faces in the sequence given in mFaceSize -
	// necessary for bone weight assignment
	idList<int> mFacePosIndices;

	// Submeshes in this mesh, each with a given material
	idList<SubMesh> mSubMeshes;
};

/** Which type of primitives the ReadPrimitives() function is going to read */
enum PrimitiveType
{
	Prim_Invalid,
	Prim_Lines,
	Prim_LineStrip,
	Prim_Triangles,
	Prim_TriStrips,
	Prim_TriFans,
	Prim_Polylist,
	Prim_Polygon
};

struct WeightPair
{
	size_t jointIndex;
	size_t weightIndex;
};

/** A skeleton controller to deform a mesh with the use of joints */
struct Controller
{
	// the URL of the mesh deformed by the controller.
	idStr mMeshId;

	// accessor URL of the joint names
	idStr mJointNameSource;

	///< The bind shape matrix, as array of floats. I'm not sure what this matrix actually describes, but it can't be ignored in all cases
	float mBindShapeMatrix[16];

	// accessor URL of the joint inverse bind matrices
	idStr mJointOffsetMatrixSource;

	// input channel: joint names.
	InputChannel mWeightInputJoints;
	// input channel: joint weights
	InputChannel mWeightInputWeights;

	// Number of weights per vertex.
	idList<size_t> mWeightCounts;

	// JointIndex-WeightIndex pairs for all vertices
	idList< WeightPair > mWeights;
};

/** A collada material. Pretty much the only member is a reference to an effect. */
struct Material
{
	idStr mEffect;
};

/** Type of the effect param */
enum ParamType
{
	Param_Sampler,
	Param_Surface
};

/** A param for an effect. Might be of several types, but they all just refer to each other, so I summarize them */
struct EffectParam
{
	ParamType mType;
	idStr mReference; // to which other thing the param is referring to.
};

/** Shading type supported by the standard effect spec of Collada */
enum ShadeType
{
	Shade_Invalid,
	Shade_Constant,
	Shade_Lambert,
	Shade_Phong,
	Shade_Blinn
};

/** Represents a texture sampler in collada */
struct Sampler
{
	Sampler()
		:	mWrapU( true )
		,	mWrapV( true )
		,	mMirrorU()
		,	mMirrorV()
		  //,	mOp( aiTextureOp_Multiply )
		  //,	mUVId( UINT_MAX )
		,	mWeighting( 1.f )
		,	mMixWithPrevious( 1.f )
	{}

	/** Name of image reference
	 */
	idStr mName;

	/** Wrap U?
	 */
	bool mWrapU;

	/** Wrap V?
	 */
	bool mWrapV;

	/** Mirror U?
	 */
	bool mMirrorU;

	/** Mirror V?
	 */
	bool mMirrorV;

	/** Blend mode
	 */
	//aiTextureOp mOp;

	/** UV transformation
	 */
	//aiUVTransform mTransform;

	/** Name of source UV channel
	 */
	//idStr mUVChannel;

	/** Resolved UV channel index or UINT_MAX if not known
	 */
	unsigned int mUVId;

	// OKINO/MAX3D extensions from here
	// -------------------------------------------------------

	/** Weighting factor
	 */
	float mWeighting;

	/** Mixing factor from OKINO
	 */
	float mMixWithPrevious;
};

/** A collada effect. Can contain about anything according to the Collada spec,
    but we limit our version to a reasonable subset. */
struct Effect
{
	// Shading mode
	ShadeType mShadeType;

	// Colors
	idVec4 mEmissive, mAmbient, mDiffuse, mSpecular,
		   mTransparent, mReflective;

	// Textures
	Sampler mTexEmissive, mTexAmbient, mTexDiffuse, mTexSpecular,
			mTexTransparent, mTexBump, mTexReflective;

	// Scalar factory
	float mShininess, mRefractIndex, mReflectivity;
	float mTransparency;

	// local params referring to each other by their SID
	typedef idHashTable<Collada::EffectParam> ParamLibrary;
	ParamLibrary mParams;

	// MAX3D extensions
	// ---------------------------------------------------------
	// Double-sided?
	bool mDoubleSided, mWireframe, mFaceted;

	Effect()
		: mShadeType( Shade_Phong )
		, mEmissive( 0, 0, 0, 1 )
		, mAmbient( 0.1f, 0.1f, 0.1f, 1 )
		, mDiffuse( 0.6f, 0.6f, 0.6f, 1 )
		, mSpecular( 0.4f, 0.4f, 0.4f, 1 )
		, mTransparent( 0, 0, 0, 1 )
		, mShininess( 10.0f )
		, mRefractIndex( 1.f )
		, mReflectivity( 1.f )
		, mTransparency( 0.f )
		, mDoubleSided( false )
		, mWireframe( false )
		, mFaceted( false )
	{
	}
};

/** An image, meaning texture */
struct Image
{
	idStr mFileName;

	/** If image file name is zero, embedded image data
	 */
	idList<uint8_t> mImageData;

	/** If image file name is zero, file format of
	 *  embedded image data.
	 */
	idStr mEmbeddedFormat;

};

/** An animation channel. */
struct AnimationChannel
{
	/** URL of the data to animate. Could be about anything, but we support only the
	 * "NodeID/TransformID.SubElement" notation
	 */
	idStr mTarget;

	/** Source URL of the time values. Collada calls them "input". Meh. */
	idStr mSourceTimes;
	/** Source URL of the value values. Collada calls them "output". */
	idStr mSourceValues;
};

/** An animation. Container for 0-x animation channels or 0-x animations */
struct Animation
{
	/** Anim name */
	idStr mName;

	/** the animation channels, if any */
	idList<AnimationChannel> mChannels;

	/** the sub-animations, if any */
	idList<Animation*> mSubAnims;

	/** Destructor */
	~Animation()
	{
		mSubAnims.DeleteContents( false );
	}
};

/** Description of a collada animation channel which has been determined to affect the current node */
struct ChannelEntry
{
	const Collada::AnimationChannel* mChannel; ///> the source channel
	idStr mTransformId;   // the ID of the transformation step of the node which is influenced
	size_t mTransformIndex; // Index into the node's transform chain to apply the channel to
	size_t mSubElement; // starting index inside the transform data

	// resolved data references
	const Collada::Accessor* mTimeAccessor; ///> Collada accessor to the time values
	const Collada::Data* mTimeData; ///> Source data array for the time values
	const Collada::Accessor* mValueAccessor; ///> Collada accessor to the key value values
	const Collada::Data* mValueData; ///> Source datat array for the key value values

	ChannelEntry()
	{
		mChannel = NULL;
		mSubElement = 0;
	}
};

} // end of namespace Collada

#endif // __MODEL_COLLADAHELPER_H__
