/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2022 Harrie van Ginneken

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

#pragma once
#include "containers/StrList.h"
#include <functional>
#include "math/Quat.h"
#include "Lib.h"
#include "containers/List.h"

enum gltfProperty
{
	INVALID,
	ASSET,
	ACCESSOR,
	CAMERAS,
	SCENE,
	SCENES,
	NODES,
	MATERIALS,
	MESHES,
	TEXTURES,
	IMAGES,
	ACCESSORS,
	BUFFERVIEWS,
	SAMPLERS,
	BUFFERS,
	ANIMATIONS,
	SKINS,
	EXTENSIONS,
	EXTENSIONS_USED,
	EXTENSIONS_REQUIRED
};


class gltfData;

struct gltf_accessor_component
{
	enum Type
	{
		_byte,
		_uByte,
		_short,
		_uShort,
		_uInt,
		_float,
		_double,
		Count
	};
};

template< class T >
struct gltf_accessor_component_type_map
{
	idStr stringID;
	int id;
	T type;
	uint sizeInBytes;//single element
};

class gltfExtra
{
public:
	gltfExtra() { }
	//entire extra json scope
	idStr json;
	//str:str pairs of each item
	idDict strPairs;
	//specialized parsers
	idList<gltfExtra*> extras;
};

class gltfExt_KHR_lights_punctual;
class gltfExtensions
{
public:
	gltfExtensions() { }
	idList<gltfExt_KHR_lights_punctual*>	KHR_lights_punctual;
};

class gltfNode_KHR_lights_punctual
{
public:
	int light;
};

class gltfNode_Extensions
{
public:
	gltfNode_Extensions() :
		KHR_lights_punctual( nullptr ) { }
	gltfNode_KHR_lights_punctual* KHR_lights_punctual;
};

class gltfExt_KHR_materials_pbrSpecularGlossiness;
class gltfMaterial_Extensions
{
public:
	gltfMaterial_Extensions() :
		KHR_materials_pbrSpecularGlossiness( nullptr ) { }
	gltfExt_KHR_materials_pbrSpecularGlossiness* KHR_materials_pbrSpecularGlossiness;
};


class gltfNode
{
public:
	gltfNode() : camera( -1 ), skin( -1 ), matrix( mat4_zero ),
		mesh( -1 ), rotation( 0.f, 0.f, 0.f, 1.f ), scale( 1.f, 1.f, 1.f ),
		translation( vec3_zero ), parent( nullptr ), dirty( true ) { }
	//Only checks name!
	bool operator == ( const gltfNode& rhs )
	{
		return name == rhs.name;
	}
	int						camera;
	idList<int>				children;
	int						skin;
	idMat4					matrix;
	int						mesh;
	idQuat					rotation;
	idVec3					scale;
	idVec3					translation;
	idList<double>			weights;
	idStr					name;
	gltfNode_Extensions		extensions;
	gltfExtra				extras;

	//
	gltfNode* 		parent;
	bool			dirty;
};

struct gltfCameraNodePtrs
{
	gltfNode* translationNode = nullptr;
	gltfNode* orientationNode = nullptr;
};

class gltfScene
{
public:
	gltfScene() { }
	idList<int> nodes;
	idStr		name;
	idStr		extensions;
	gltfExtra	extras;
};

class gltfMesh_Primitive_Attribute
{
public:
	enum Type
	{
		Position,
		Normal,
		Tangent,
		TexCoord0,
		TexCoord1,
		TexCoord2,
		TexCoord3,
		TexCoord4,
		TexCoord5,
		TexCoord6,
		TexCoord7,
		Color0,
		Color1,
		Color2,
		Color3,
		Weight,
		Joints, // joint indices
		Count
	};

	gltfMesh_Primitive_Attribute() : accessorIndex( -1 ), elementSize( 0 ), type( gltfMesh_Primitive_Attribute::Type::Count ) { }
	idStr attributeSemantic;
	int accessorIndex;
	uint elementSize;

	Type type;
};

struct gltf_mesh_attribute_map
{
	idStr stringID;
	gltfMesh_Primitive_Attribute::Type attib;
	uint elementSize;
};

class gltfMesh_Primitive
{
public:
	gltfMesh_Primitive() : indices( -1 ), material( -1 ), mode( -1 ) { }
	idList<gltfMesh_Primitive_Attribute*> attributes;
	int			indices;
	int			material;
	int			mode;
	idStr		target;
	idStr		extensions;
	gltfExtra	extras;
};

class gltfMesh
{
public:
	gltfMesh() { };

	idList<gltfMesh_Primitive*> primitives;	// gltfMesh_Primitive[1,*]
	idList<double>	weights;						// number[1,*]
	idStr			name;
	idStr			extensions;
	gltfExtra		extras;
};

class gltfCamera_Orthographic
{
public:
	gltfCamera_Orthographic() : xmag( 0.0f ), ymag( 0.0f ), zfar( 0.0f ), znear( 0.0f ) { };
	float		xmag;
	float		ymag;
	float		zfar;
	float		znear;
	idStr		extensions;
	gltfExtra	extras;
};

class gltfCamera_Perspective
{
public:
	gltfCamera_Perspective() : aspectRatio( 0.0f ), yfov( 0.0f ), zfar( 0.0f ), znear( 0.0f ) { };
	float		aspectRatio;
	float		yfov;
	float		zfar;
	float		znear;
	idStr		extensions;
	gltfExtra	extras;
};

class gltfCamera
{
public:
	gltfCamera() { };
	gltfCamera_Orthographic orthographic;
	gltfCamera_Perspective	perspective;
	idStr					type;
	idStr					name;
	idStr					extensions;
	gltfExtra				extras;
};

class gltfAnimation_Channel_Target
{
public:
	gltfAnimation_Channel_Target() : node( -1 ), TRS( gltfTRS::count ) { };
	int			node;
	idStr		path;
	idStr		extensions;
	gltfExtra	extras;

	enum gltfTRS
	{
		none,
		rotation,
		translation,
		scale,
		weights,
		count
	};

	gltfTRS TRS;

	static gltfTRS resolveType( idStr type )
	{
		if( type == "translation" )
		{
			return gltfTRS::translation;
		}
		else if( type == "rotation" )
		{
			return  gltfTRS::rotation;
		}
		else if( type == "scale" )
		{
			return  gltfTRS::scale;
		}
		else if( type == "weights" )
		{
			return  gltfTRS::weights;
		}
		return gltfTRS::count;
	}
};

class gltfAnimation_Channel
{
public:
	gltfAnimation_Channel() : sampler( -1 ) { };
	int								sampler;
	gltfAnimation_Channel_Target	target;
	idStr							extensions;
	gltfExtra						extras;
};

class gltfAnimation_Sampler
{
public:
	gltfAnimation_Sampler() : input( -1 ), interpolation( "LINEAR" ), output( -1 ), intType( gltfInterpType::count ) { };
	int			input;
	idStr		interpolation;
	int			output;
	idStr		extensions;
	gltfExtra	extras;

	enum gltfInterpType
	{
		linear,
		step,
		cubicSpline,
		count
	};

	gltfInterpType intType;

	static gltfInterpType resolveType( idStr type )
	{
		if( type == "LINEAR" )
		{
			return gltfInterpType::linear;
		}
		else if( type == "STEP" )
		{
			return gltfInterpType::step;
		}
		else if( type == "CUBICSPLINE" )
		{
			return gltfInterpType::cubicSpline;
		}
		return gltfInterpType::count;
	}

};

class gltfAnimation
{
public:
	gltfAnimation() : maxTime( 0.0f ), numFrames( 0 ) { };
	idList<gltfAnimation_Channel*>	channels;
	idList<gltfAnimation_Sampler*>	samplers;
	idStr							name;
	idStr							extensions;
	gltfExtra						extras;

	float maxTime;

	//id specific
	mutable int	ref_count;
	int numFrames;
	void DecreaseRefs() const
	{
		ref_count--;
	};
	void IncreaseRefs() const
	{
		ref_count++;
	};
	bool GetBounds( idBounds& bnds, int time, int cyclecount ) const
	{
		return false;
	}
	bool GetOriginRotation( idQuat& rotation, int time, int cyclecount ) const
	{
		return false;
	}
	bool GetOrigin( idVec3& offset, int time, int cyclecount ) const
	{
		return false;
	}
	int NumFrames() const
	{
		return numFrames;
	}
};

class gltfAccessor_Sparse_Values
{
public:
	gltfAccessor_Sparse_Values() : bufferView( -1 ), byteOffset( -1 ) { };
	int			bufferView;
	int			byteOffset;
	idStr		extensions;
	gltfExtra	extras;
};

class gltfAccessor_Sparse_Indices
{
public:
	gltfAccessor_Sparse_Indices() : bufferView( -1 ), byteOffset( -1 ), componentType( -1 ) { };
	int			bufferView;
	int			byteOffset;
	int			componentType;
	idStr		extensions;
	gltfExtra	extras;
};

class gltfAccessor_Sparse
{
public:
	gltfAccessor_Sparse() : count( -1 ) { };
	int count;
	gltfAccessor_Sparse_Indices indices;
	gltfAccessor_Sparse_Values values;
	idStr extensions;
	gltfExtra	extras;
};

class gltfAccessor
{
public:
	gltfAccessor() : bufferView( -1 ), byteOffset( 0 ), componentType( -1 ), normalized( false ), count( -1 ) ,
		floatView( nullptr ), vecView( nullptr ), quatView( nullptr ), matView( nullptr ) { }
	int					bufferView;
	int					byteOffset;
	int					componentType;
	bool				normalized;
	int					count;
	idStr				type;
	idList<double>		max;
	idList<double>		min;
	gltfAccessor_Sparse sparse;
	idStr				name;
	idStr				extensions;
	gltfExtra			extras;

	uint typeSize;

	idList<float>*   	floatView;
	idList<idVec3*>* 	vecView;
	idList<idQuat*>* 	quatView;
	idList<idMat4>*  	matView;
};

class gltfBufferView
{
public:
	gltfBufferView() : buffer( -1 ), byteLength( -1 ), byteStride( 0 ), byteOffset( 0 ), target( -1 ) { };
	int			buffer;
	int			byteLength;
	int			byteStride;
	int			byteOffset;
	int			target;
	idStr		name;
	idStr		extensions;
	gltfExtra	extras;
	//
	gltfData* parent;
};

class gltfBuffer
{
public:
	gltfBuffer() : byteLength( -1 ), parent( nullptr ) { };
	idStr		uri;
	int			byteLength;
	idStr		name;
	idStr		extensions;
	gltfExtra	extras;
	//
	gltfData* 	parent;
};

class gltfSampler
{
public:
	gltfSampler() : magFilter( 0 ), minFilter( 0 ), wrapS( 10497 ), wrapT( 10497 ) { };
	int			magFilter;
	int			minFilter;
	int			wrapS;
	int			wrapT;
	idStr		name;
	idStr		extensions;
	gltfExtra	extras;
	//
	uint bgfxSamplerFlags;
};

class gltfImage
{
public:
	gltfImage() : bufferView( -1 ) { }
	idStr		uri;
	idStr		mimeType;
	int			bufferView;
	idStr		name;
	idStr		extensions;
	gltfExtra	extras;
};

class gltfSkin
{
public:
	gltfSkin() : inverseBindMatrices( -1 ), skeleton( -1 ), name( "unnamedSkin" ) { };
	int			inverseBindMatrices;
	int			skeleton;
	idList<int>	joints; // integer[1,*]
	idStr		name;
	idStr		extensions;
	gltfExtra	extras;
};

class gltfExt_KHR_texture_transform;
class gltfTexture_Info_Extensions
{
public:
	gltfTexture_Info_Extensions() :
		KHR_texture_transform( nullptr ) { }
	gltfExt_KHR_texture_transform* KHR_texture_transform;
};

class gltfOcclusionTexture_Info
{
public:
	gltfOcclusionTexture_Info() : index( -1 ), texCoord( 0 ), strength( 1.0f ) { }
	int							index;
	int							texCoord;
	float						strength;
	gltfTexture_Info_Extensions	extensions;
	gltfExtra					extras;
};

class gltfNormalTexture_Info
{
public:
	gltfNormalTexture_Info() : index( -1 ), texCoord( 0 ), scale( 1.0f ) { }
	int							index;
	int							texCoord;
	float						scale;
	gltfTexture_Info_Extensions	extensions;
	gltfExtra					extras;
};

class gltfTexture_Info
{
public:
	gltfTexture_Info() : index( -1 ), texCoord( 0 ) { }
	int							index;
	int							texCoord;
	gltfTexture_Info_Extensions	extensions;
	gltfExtra					extras;
};


class gltfTexture
{
public:
	gltfTexture() : sampler( -1 ), source( -1 ) { }
	int							sampler;
	int							source;
	idStr						name;
	gltfTexture_Info_Extensions	extensions;
	gltfExtra					extras;
};

class gltfMaterial_pbrMetallicRoughness
{
public:
	gltfMaterial_pbrMetallicRoughness() : baseColorFactor( vec4_one ), metallicFactor( 1.0f ), roughnessFactor( 1.0f ) { }
	idVec4				baseColorFactor;
	gltfTexture_Info	baseColorTexture;
	float				metallicFactor;
	float				roughnessFactor;
	gltfTexture_Info	metallicRoughnessTexture;
	idStr				extensions;
	gltfExtra			extras;
};

class gltfMaterial
{
public:
	enum gltfAlphaMode
	{
		gltfOPAQUE,
		gltfMASK,
		gltfBLEND,
		count
	};

	gltfMaterial() : emissiveFactor( vec3_zero ), alphaMode( "OPAQUE" ), alphaCutoff( 0.5f ), doubleSided( false ) { }
	gltfMaterial_pbrMetallicRoughness	pbrMetallicRoughness;
	gltfNormalTexture_Info				normalTexture;
	gltfOcclusionTexture_Info			occlusionTexture;
	gltfTexture_Info					emissiveTexture;
	idVec3								emissiveFactor;
	idStr								alphaMode;
	float								alphaCutoff;
	bool								doubleSided;
	idStr								name;
	gltfMaterial_Extensions				extensions;
	gltfExtra							extras;

	gltfAlphaMode intType;

	static gltfAlphaMode resolveAlphaMode( idStr type )
	{
		if( type == "OPAQUE" )
		{
			return gltfAlphaMode::gltfOPAQUE;
		}
		else if( type == "MASK" )
		{
			return gltfAlphaMode::gltfMASK;
		}
		else if( type == "BLEND" )
		{
			return gltfAlphaMode::gltfBLEND;
		}
		return gltfAlphaMode::count;
	}
};

class gltfAsset
{
public:
	gltfAsset() { }
	idStr		copyright;
	idStr		generator;
	idStr		version;
	idStr		minVersion;
	idStr		extensions;
	gltfExtra	extras;
};

//this is not used.
//if an extension is found, it _will_ be used. (if implemented)
class gltfExtensionsUsed
{
public:
	gltfExtensionsUsed() { }
	idStr	extension;
};

//ARCHIVED?
//https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Archived/KHR_materials_pbrSpecularGlossiness
class gltfExt_KHR_materials_pbrSpecularGlossiness
{
public:
	gltfExt_KHR_materials_pbrSpecularGlossiness() { }
	idVec4				diffuseFactor;
	gltfTexture_Info	diffuseTexture;
	idVec3				specularFactor;
	float				glossinessFactor;
	gltfTexture_Info	specularGlossinessTexture;
	idStr				extensions;
	gltfExtra			extras;
};

//KHR_lights_punctual_spot
//https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/schema/light.spot.schema.json
class gltfExt_KHR_lights_punctual_spot
{
public:
	gltfExt_KHR_lights_punctual_spot() : innerConeAngle( 0.0f ), outerConeAngle( idMath::ONEFOURTH_PI ) { }
	float		innerConeAngle;
	float		outerConeAngle;
	idStr		extensions;
	gltfExtra	extras;
};

//KHR_lights_punctual
//https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/schema/light.schema.json
class gltfExt_KHR_lights_punctual
{
public:
	enum Type
	{
		Directional,
		Point,
		Spot,
		count
	};
	gltfExt_KHR_lights_punctual() : color( vec3_one ), intensity( 1.0f ), range( -1.0f ), intType( -1 ) { }
	idVec3		color;
	float		intensity;
	gltfExt_KHR_lights_punctual_spot		spot;
	idStr		type; //directional=0,point=1,spot=2
	float		range;
	idStr		name;
	idStr		extensions;
	gltfExtra	extras;

	int intType;


	static Type resolveType( idStr type )
	{
		if( type == "directional" )
		{
			return Type::Directional;
		}
		else if( type == "point" )
		{
			return Type::Point;
		}
		else if( type == "spot" )
		{
			return  Type::Spot;
		}
		return  Type::count;
	}
};

//KHR_texture_transform
//https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_texture_transform/schema/KHR_texture_transform.textureInfo.schema.json
class gltfExt_KHR_texture_transform
{
public:
	gltfExt_KHR_texture_transform() : offset( vec2_zero ), rotation( 0.0f ), scale( vec2_one ), texCoord( -1 ), index( 0 ), resolved( false ) { }
	idVec2	offset;
	float	rotation;
	idVec2	scale;
	int		texCoord;
	idStr	extensions;
	gltfExtra	extras;

	//for shader
	uint	index;
	bool	resolved;
};

/////////////////////////////////////////////////////////////////////////////
//// For these to function you need to add an private idList<gltf{name}*> {target}
//#define GLTFCACHEITEM(name,target) \
//gltf##name * name () { target.AssureSizeAlloc( target.Num()+1,idListNewElement<gltf##name>); return target[target.Num()-1];} \
//const inline idList<gltf##name*> & ##name##List() { return target; }


// URI's are resolved during parsing so that
// all data should be layed out like an GLB with multiple bin chunks
// EACH URI will have an unique chunk
// JSON chunk MUST be the first one to be allocated/added

class gltfData
{
public:
	gltfData() : fileName( "" ), fileNameHash( 0 ), json( nullptr ), data( nullptr ), totalChunks( -1 ) { };
	~gltfData();
	byte* AddData( int size, int* bufferID = nullptr );
	byte* GetJsonData( int& size )
	{
		size = jsonDataLength;
		return json;
	}
	byte* GetData( int index )
	{
		return data[index];
	}
	void FileName( const idStr& file, int hash )
	{
		fileName = file;
		fileNameHash = hash;
	}
	int FileNameHash()
	{
		return fileNameHash;
	}
	idStr& FileName()
	{
		return fileName;
	}

	static idHashIndex			fileDataHash;
	static idList<gltfData*>	dataList;
	//add data for filename
	static gltfData* Data( idStr& fileName, bool create = false )
	{
		static bool intialized = false;
		if( ! intialized )
		{
			dataList.SetGranularity( 1 );
			intialized = true;
		}
		int key = fileDataHash.GenerateKey( fileName );
		int index = fileDataHash.GetFirst( key );

		if( create && index == -1 )
		{
			index = dataList.Num();
			dataList.AssureSizeAlloc( index + 1, idListNewElement<gltfData> );
			dataList[index]->FileName( fileName, key );
			fileDataHash.Add( key , index );
		}

		if( !create && index < 0 )
		{
			return nullptr;
		}

		return dataList[index];
	}
	static const idList<gltfData*>& DataList()
	{
		return dataList;
	}

	static void ClearData( idStr& fileName );

	//return the GLTF nodes that control the given camera
	//return TRUE if the camera uses 2 nodes (like when blender exports gltfs with +Y..)
	//This is determined by checking for an "_Orientation" suffix to the camera name of the node that has the target camera assigned.
	// if so, translate node will be set to the parent node of the orientation node.
	//Note: does not take overides into account!
	gltfNode* GetCameraNodes( gltfCamera* camera )
	{
		gltfCameraNodePtrs result;

		assert( camera );
		int camId = -1;
		for( auto* cam : cameras )
		{
			camId++;
			if( cam == camera )
			{
				break;
			}
		}

		for( int i = 0; i < nodes.Num(); i++ )
		{
			if( nodes[i]->camera != -1 && nodes[i]->camera == camId )
			{
				return nodes[i];
			}
		}

		return nullptr;
	}

	gltfNode* GetNode( gltfScene* scene , gltfMesh* mesh , int* id = nullptr )
	{
		assert( scene );
		assert( mesh );

		auto& nodeList = scene->nodes;
		int nodeCnt = 0;
		for( auto& nodeId : nodeList )
		{

			if( nodes[nodeId]->mesh != -1 &&*& meshes[nodes[nodeId]->mesh] == mesh )
			{
				if( id != nullptr )
				{
					*id = nodeCnt;
				}

				return nodes[nodeId];
			}
			nodeCnt++;
		}

		return nullptr;
	}

	gltfNode* GetNode( idStr sceneName,  int id, idStr* name = nullptr )
	{
		int sceneId = GetSceneId( sceneName );
		if( sceneId < 0 || sceneId > scenes.Num() )
		{
			return nullptr;
		}

		gltfScene* scene = scenes[sceneId];

		assert( scene );
		assert( id >= 0 );

		auto& nodeList = scene->nodes;
		for( auto& nodeId : nodeList )
		{
			if( nodeId == id )
			{
				if( name != nullptr )
				{
					*name = nodes[nodeId]->name;
				}

				return nodes[nodeId];
			}
		}

		return nullptr;
	}

	gltfNode* GetNode( idStr name, int* id = nullptr, bool caseSensitive = false )
	{
		assert( name[0] );

		auto& nodeList = NodeList();
		for( auto* node : nodes )
		{
			int nodeId = GetNodeIndex( node );
			if( caseSensitive ? nodes[nodeId]->name.Cmp( name ) : nodes[nodeId]->name.Icmp( name ) == 0 )
			{
				if( id != nullptr )
				{
					*id = nodeId;
				}

				return nodes[nodeId];
			}
		}

		return nullptr;
	}


	gltfNode* GetNode( idStr sceneName, idStr name , int* id = nullptr , bool caseSensitive = false )
	{
		int sceneId =  GetSceneId( sceneName );
		if( sceneId < 0 || sceneId > scenes.Num() )
		{
			return nullptr;
		}

		gltfScene* scene = scenes[sceneId];

		assert( scene );
		assert( name[0] );

		auto& nodeList = scene->nodes;
		for( auto nodeId : nodeList )
		{
			if( caseSensitive ? nodes[nodeId]->name.Cmp( name ) : nodes[nodeId]->name.Icmp( name ) == 0 )
			{
				if( id != nullptr )
				{
					*id = nodeId;
				}

				return nodes[nodeId];
			}
		}

		return nullptr;
	}

	int GetNodeIndex( gltfNode* node )
	{
		int index = -1;
		for( auto& it : nodes )
		{
			index++;
			if( it == node )
			{
				return index;
			}
		}
		return -1;
	}

	bool HasAnimation( int nodeID )
	{
		for( auto anim : animations )
		{
			for( auto channel : anim->channels )
			{
				if( channel->target.node == nodeID )
				{
					return true;
				}
			}
		}
		return false;
	}

	gltfAnimation* GetAnimation( idStr animName, int target )
	{
		for( auto* anim : animations )
		{
			if( anim->name == animName )
			{
				bool hasTarget = false;
				for( auto* channel : anim->channels )
				{
					if( channel->target.node == target )
					{
						hasTarget = true;
						break;
					}
				}
				if( hasTarget )
				{
					return anim;
				}
			}
		}
		return nullptr;
	}

	int GetSceneId( idStr sceneName , gltfScene* result = nullptr ) const
	{
		for( int i = 0; i < scenes.Num(); i++ )
		{
			if( scenes[i]->name == sceneName )
			{
				if( result != nullptr )
				{
					result = scenes[i];
				}

				return i;
			}
		}
		return -1;
	}

	void GetAllMeshes( gltfNode* node, idList<int>& meshIds )
	{
		if( node->mesh != -1 )
		{
			meshIds.Append( GetNodeIndex( node ) );
		}

		for( auto child : node->children )
		{
			GetAllMeshes( nodes[child], meshIds );
		}
	}

	gltfSkin* GetSkin( int boneNodeId )
	{
		for( auto skin : skins )
		{
			if( skin->joints.Find( boneNodeId ) )
			{
				return skin;
			}
		}

		return nullptr;
	}

	gltfSkin* GetSkin( gltfAnimation* anim )
	{
		auto animTargets = GetAnimTargets( anim );

		if( !animTargets.Num() )
		{
			return nullptr;
		}

		for( int nodeID : animTargets )
		{
			gltfSkin* foundSkin = GetSkin( nodeID );
			if( foundSkin != nullptr )
			{
				return foundSkin;
			}
		}

		return nullptr;
	}

	idList<int> GetAnimTargets( gltfAnimation* anim ) const
	{
		idList<int> result;
		for( auto channel : anim->channels )
		{
			result.AddUnique( channel->target.node );
		}
		return result;
	}

	idList<int> GetChannelIds( gltfAnimation* anim , gltfNode* node ) const
	{
		idList<int> result;
		int channelIdx = 0;
		for( auto channel : anim->channels )
		{
			if( channel->target.node >= 0 && nodes[channel->target.node] == node )
			{
				result.Append( channelIdx );
				break;
			}
			channelIdx++;
		}
		return result;
	}

	int GetAnimationIds( gltfNode* node , idList<int>& result )
	{

		int animIdx = 0;
		for( auto anim : animations )
		{
			for( auto channel : anim->channels )
			{
				if( channel->target.node >= 0 && nodes[channel->target.node] == node )
				{
					result.AddUnique( animIdx );
				}
			}
			animIdx++;
		}
		for( int nodeId : node->children )
		{
			GetAnimationIds( nodes[nodeId], result );
		}
		return result.Num();
	}

	idMat4 GetViewMatrix( int camId ) const
	{
		//if (cameraManager->HasOverideID(camId) )
		//{
		//	auto overrideCam = cameraManager->GetOverride( camId );
		//	camId = overrideCam.newCameraID;
		//}

		idMat4 result = mat4_identity;

		idList<gltfNode*> hierachy( 2 );
		gltfNode* parent = nullptr;

		for( int i = 0; i < nodes.Num(); i++ )
		{
			if( nodes[i]->camera != -1 && nodes[i]->camera == camId )
			{
				parent = nodes[i];
				while( parent )
				{
					hierachy.Append( parent );
					parent = parent->parent;
				}
				break;
			}
		}

		for( int i = hierachy.Num() - 1; i >= 0; i-- )
		{
			ResolveNodeMatrix( hierachy[i] );
			result *= hierachy[i]->matrix;
		}

		return result;
	}

	//Please note : assumes all nodes are _not_ dirty!
	idMat4 GetLightMatrix( int lightId ) const
	{
		idMat4 result = mat4_identity;

		idList<gltfNode*> hierachy;
		gltfNode* parent = nullptr;
		hierachy.SetGranularity( 2 );

		for( int i = 0; i < nodes.Num(); i++ )
		{
			if( nodes[i]->extensions.KHR_lights_punctual && nodes[i]->extensions.KHR_lights_punctual->light == lightId )
			{
				parent = nodes[i];
				while( parent )
				{
					hierachy.Append( parent );
					parent = parent->parent;
				}
				break;
			}
		}

		for( int i = hierachy.Num() - 1; i >= 0; i-- )
		{
			result *= hierachy[i]->matrix;
		}

		return result;
	}

	// v * T * R * S. ->row major
	// v' = S * R * T * v -> column major;
	//bgfx = column-major
	//idmath = row major, except mat3
	//gltf matrices : column-major.
	//if mat* is valid , it will be multplied by this node's matrix that is resolved in its full hiararchy and stops at root.
	static void ResolveNodeMatrix( gltfNode* node, idMat4* mat = nullptr , gltfNode* root = nullptr )
	{
		if( node->dirty )
		{
			idMat4 scaleMat = idMat4(
								  node->scale.x, 0, 0, 0,
								  0, node->scale.y, 0, 0,
								  0, 0, node->scale.z, 0,
								  0, 0, 0, 1
							  );

			node->matrix = idMat4( mat3_identity, node->translation ) * node->rotation.ToMat4().Transpose() * scaleMat;

			node->dirty = false;
		}

		//resolve full hierarchy
		if( mat != nullptr )
		{
			idList<gltfNode*> hierachy( 2 );
			gltfNode* parent = node;
			while( parent )
			{
				ResolveNodeMatrix( parent );
				hierachy.Append( parent );
				if( parent == root )
				{
					break;
				}
				parent = parent->parent;
			}
			for( int i = hierachy.Num() - 1; i >= 0; i-- )
			{
				*mat *= hierachy[i]->matrix;
			}
		}
	}

	void Advance( gltfAnimation* anim = nullptr );

	//this copies the data and view cached on the accessor
	template <class T>
	idList<T*>& GetAccessorView( gltfAccessor* accessor );
	idList<float>& GetAccessorView( gltfAccessor* accessor );
	idList<idMat4>& GetAccessorViewMat( gltfAccessor* accessor );

	int& DefaultScene()
	{
		return scene;
	}

//#define GLTFCACHEITEM(name,target) \
//gltf##name * name () { target.AssureSizeAlloc( target.Num()+1,idListNewElement<gltf##name>); return target[target.Num()-1];} \
//const inline idList<gltf##name*> & ##name##List() { return target; }

	gltfBuffer* Buffer()
	{
		buffers.AssureSizeAlloc( buffers.Num() + 1 , idListNewElement<gltfBuffer> );
		return buffers[buffers.Num() - 1];
	}
	const inline idList<gltfBuffer*>& BufferList()
	{
		return buffers;
	}

	gltfSampler* Sampler()
	{
		samplers.AssureSizeAlloc( samplers.Num() + 1 , idListNewElement<gltfSampler> );
		return samplers[samplers.Num() - 1];
	}
	const inline idList<gltfSampler*>& SamplerList()
	{
		return samplers;
	}

	gltfBufferView* BufferView()
	{
		bufferViews.AssureSizeAlloc( bufferViews.Num() + 1 , idListNewElement<gltfBufferView> );
		return bufferViews[bufferViews.Num() - 1];
	}
	const inline idList<gltfBufferView*>& BufferViewList()
	{
		return bufferViews;
	}

	gltfImage* Image()
	{
		images.AssureSizeAlloc( images.Num() + 1 , idListNewElement<gltfImage> );
		return images[images.Num() - 1];
	}
	const inline idList<gltfImage*>& ImageList()
	{
		return images;
	}

	gltfTexture* Texture()
	{
		textures.AssureSizeAlloc( textures.Num() + 1 , idListNewElement<gltfTexture> );
		return textures[textures.Num() - 1];
	}
	const inline idList<gltfTexture*>& TextureList()
	{
		return textures;
	}

	gltfAccessor* Accessor()
	{
		accessors.AssureSizeAlloc( accessors.Num() + 1 , idListNewElement<gltfAccessor> );
		return accessors[accessors.Num() - 1];
	}
	const inline idList<gltfAccessor*>& AccessorList()
	{
		return accessors;
	}

	gltfExtensionsUsed* ExtensionsUsed()
	{
		extensionsUsed.AssureSizeAlloc( accessors.Num() + 1 , idListNewElement<gltfExtensionsUsed> );
		return extensionsUsed[extensionsUsed.Num() - 1];
	}
	const inline idList<gltfExtensionsUsed*>& ExtensionsUsedList()
	{
		return extensionsUsed;
	}

	gltfMesh* Mesh()
	{
		meshes.AssureSizeAlloc( meshes.Num() + 1 , idListNewElement<gltfMesh> );
		return meshes[meshes.Num() - 1];
	}
	const inline idList<gltfMesh*>& MeshList()
	{
		return meshes;
	}

	gltfScene* Scene()
	{
		scenes.AssureSizeAlloc( scenes.Num() + 1 , idListNewElement<gltfScene> );
		return scenes[scenes.Num() - 1];
	}
	const inline idList<gltfScene*>& SceneList()
	{
		return scenes;
	}

	gltfNode* Node()
	{
		nodes.AssureSizeAlloc( nodes.Num() + 1 , idListNewElement<gltfNode> );
		return nodes[nodes.Num() - 1];
	}
	const inline idList<gltfNode*>& NodeList()
	{
		return nodes;
	}

	gltfCamera* Camera()
	{
		cameras.AssureSizeAlloc( cameras.Num() + 1 , idListNewElement<gltfCamera> );
		return cameras[cameras.Num() - 1];
	}
	const inline idList<gltfCamera*>& CameraList()
	{
		return cameras;
	}

	gltfMaterial* Material()
	{
		materials.AssureSizeAlloc( materials.Num() + 1 , idListNewElement<gltfMaterial> );
		return materials[materials.Num() - 1];
	}
	const inline idList<gltfMaterial*>& MaterialList()
	{
		return materials;
	}

	gltfExtensions* Extensions()
	{
		extensions.AssureSizeAlloc( extensions.Num() + 1 , idListNewElement<gltfExtensions> );
		return extensions[extensions.Num() - 1];
	}
	const inline idList<gltfExtensions*>& ExtensionsList()
	{
		return extensions;
	}

	gltfAnimation* Animation()
	{
		animations.AssureSizeAlloc( animations.Num() + 1 , idListNewElement<gltfAnimation> );
		return animations[animations.Num() - 1];
	}
	const inline idList<gltfAnimation*>& AnimationList()
	{
		return animations;
	}

	gltfSkin* Skin()
	{
		skins.AssureSizeAlloc( skins.Num() + 1 , idListNewElement<gltfSkin> );
		return skins[skins.Num() - 1];
	}
	const inline idList<gltfSkin*>& SkinList()
	{
		return skins;
	}

	/*
	GLTFCACHEITEM( Buffer, buffers )
	GLTFCACHEITEM( Sampler, samplers )
	GLTFCACHEITEM( BufferView, bufferViews )
	GLTFCACHEITEM( Image, images )
	GLTFCACHEITEM( Texture, textures )
	GLTFCACHEITEM( Accessor, accessors )
	GLTFCACHEITEM( ExtensionsUsed, extensionsUsed )
	GLTFCACHEITEM( Mesh, meshes )
	GLTFCACHEITEM( Scene, scenes )
	GLTFCACHEITEM( Node, nodes )
	GLTFCACHEITEM( Camera, cameras )
	GLTFCACHEITEM( Material, materials )
	GLTFCACHEITEM( Extensions, extensions )
	GLTFCACHEITEM( Animation, animations )
	GLTFCACHEITEM( Skin, skins )
	*/

	//gltfCameraManager * cameraManager;
private:
	idStr fileName;
	int	fileNameHash;

	byte* json;
	byte** data;
	int jsonDataLength;
	int totalChunks;

	idList<gltfBuffer*>			buffers;
	idList<gltfImage*>			images;
	idList<gltfData*>			assetData;
	idList<gltfSampler*>		samplers;
	idList<gltfBufferView*>		bufferViews;
	idList<gltfTexture*>		textures;
	idList<gltfAccessor*>		accessors;
	idList<gltfExtensionsUsed*>	extensionsUsed;
	idList<gltfMesh*>			meshes;
	int							scene;
	idList<gltfScene*>			scenes;
	idList<gltfNode*>			nodes;
	idList<gltfCamera*>			cameras;
	idList<gltfMaterial*>		materials;
	idList<gltfExtensions*>		extensions;
	idList<gltfAnimation*>		animations;
	idList<gltfSkin*>			skins;
};

#undef GLTFCACHEITEM