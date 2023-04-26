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
#include "gltfProperties.h"

#pragma region GLTF Types parsing

#pragma region Parser interfaces
struct parsable
{
public:
	virtual ~parsable() {}

	virtual void parse( idToken& token ) = 0;
	virtual void parse( idToken& token , idLexer* parser ) {};
	virtual idStr& Name() = 0;
};

template<class T>
class parseType
{
public:
	void Set( T* type )
	{
		item = type;
	}
	T* item;
};

class gltfItem : public parsable, public parseType<idStr>
{
public:
	gltfItem( idStr Name ) : name( Name )
	{
		item = nullptr;
	}
	virtual void parse( idToken& token )
	{
		*item = token;
	};
	virtual idStr& Name()
	{
		return name;
	}
	~gltfItem() {}
private:
	idStr name;
};

class gltfObject : public parsable, public parseType<idStr>
{
public:
	gltfObject( idStr Name ) : name( Name ), object( "null" ) {}
	virtual void parse( idToken& token ) {}
	virtual void parse( idToken& token , idLexer* parser )
	{
		parser->UnreadToken( &token );
		parser->ParseBracedSection( object );
	}
	virtual idStr& Name()
	{
		return name;
	}
private:
	idStr name;
	idStr object;
};

class gltfItem_Extra : public parsable, public parseType<gltfExtra>
{
public:
	gltfItem_Extra( idStr Name ) : name( Name ), data( nullptr ), parser( nullptr )
	{
		item = nullptr;
	}
	virtual void parse( idToken& token ) ;
	virtual idStr& Name()
	{
		return name;
	}
	void Set( gltfExtra* type, idLexer* lexer )
	{
		parseType::Set( type );
		parser = lexer;
	}
	static void Register( parsable* extra );
private:
	idStr name;
	gltfData* data;
	idLexer* parser;
};

class gltfItem_uri : public parsable, public parseType<idStr>
{
public:
	gltfItem_uri( idStr Name ) : name( Name )
	{
		item = nullptr;
	}
	virtual void parse( idToken& token )
	{
		*item = token;
		Convert();
	};
	virtual idStr& Name()
	{
		return name;
	}
	void Set( idStr* type, int* targetBufferview, gltfData* dataDestination )
	{
		parseType::Set( type );
		bufferView = targetBufferview;
		data = dataDestination;
	}
	// read data from uri file, and push it at end of current data buffer for this GLTF File
	// bufferView will be set accordingly to the generated buffer.
	bool Convert();
private:
	idStr name;
	int* bufferView;
	gltfData*   data;
};
#pragma endregion

#pragma region helper macro to define gltf data types with extra parsing context forced to be implemented externally
#define gltfItemClassParser(className,ptype)											\
class gltfItem_##className : public parsable, public parseType<ptype>					\
{public:																				\
	gltfItem_##className( idStr Name ) : name( Name ){ item = nullptr; }				\
	virtual void parse( idToken &token );												\
	virtual idStr &Name() { return name; }												\
	void Set( ptype *type, idLexer *lexer ) { parseType::Set( type ); parser = lexer; }	\
private:																				\
	idStr name;																			\
	idLexer *parser;}
#pragma endregion

gltfItemClassParser( animation_sampler,				idList<gltfAnimation_Sampler*> );
gltfItemClassParser( animation_channel_target,		gltfAnimation_Channel_Target );
gltfItemClassParser( animation_channel,				idList<gltfAnimation_Channel*> );
gltfItemClassParser( mesh_primitive,				idList<gltfMesh_Primitive*> );
gltfItemClassParser( mesh_primitive_attribute,		idList<gltfMesh_Primitive_Attribute*> );
gltfItemClassParser( integer_array,					idList<int> );
gltfItemClassParser( number_array,					idList<double> ); //does float suffice?
gltfItemClassParser( mat4,							idMat4 );
gltfItemClassParser( vec4,							idVec4 );
gltfItemClassParser( vec3,							idVec3 );
gltfItemClassParser( vec2,							idVec2 );
gltfItemClassParser( quat,							idQuat );
gltfItemClassParser( accessor_sparse,				gltfAccessor_Sparse );
gltfItemClassParser( accessor_sparse_indices,		gltfAccessor_Sparse_Indices );
gltfItemClassParser( accessor_sparse_values,		gltfAccessor_Sparse_Values );
gltfItemClassParser( camera_perspective,			gltfCamera_Perspective );
gltfItemClassParser( camera_orthographic,			gltfCamera_Orthographic );
gltfItemClassParser( pbrMetallicRoughness,			gltfMaterial_pbrMetallicRoughness );
gltfItemClassParser( texture_info,					gltfTexture_Info );
gltfItemClassParser( normal_texture,				gltfNormalTexture_Info );
gltfItemClassParser( occlusion_texture,				gltfOcclusionTexture_Info );
gltfItemClassParser( node_extensions,				gltfNode_Extensions );
gltfItemClassParser( material_extensions,			gltfMaterial_Extensions );
gltfItemClassParser( texture_info_extensions,		gltfTexture_Info_Extensions );

//extensions
gltfItemClassParser( KHR_lights_punctual,							gltfExtensions );
gltfItemClassParser( Node_KHR_lights_punctual,						gltfNode_Extensions );
gltfItemClassParser( Material_KHR_materials_pbrSpecularGlossiness,	gltfMaterial_Extensions );
gltfItemClassParser( TextureInfo_KHR_texture_transform,				gltfTexture_Info_Extensions );

#undef gltfItemClassParser

#pragma region helper macro to define more gltf data types that only rely on token
#define gltfItemClass(className,type,function)								\
class gltfItem_##className : public parsable, public parseType<type>		\
{public:																	\
	gltfItem_##className( idStr Name ) : name( Name ){ item = nullptr; }	\
	virtual void parse( idToken &token ) { function }						\
	virtual idStr &Name() { return name; }									\
private:																	\
	idStr name;}
#pragma endregion

gltfItemClass( integer, int, *item = token.GetIntValue(); );
gltfItemClass( number, float, *item = token.GetFloatValue(); );
gltfItemClass( boolean, bool, if( token.Icmp( "true" ) == 0 ) *item = true; else
{
	if( token.Icmp( "false" ) == 0 )
		{
			*item = false;
		}
		else
		{
			idLib::FatalError( "parse error" );
		}
	} );
#undef gltfItemClass

class gltfItemArray
{
public:
	~gltfItemArray();
	gltfItemArray() { };
	int Num()
	{
		return items.Num();
	}
	void AddItemDef( parsable* item )
	{
		items.Alloc() = item;
	}
	int Fill( idLexer* lexer , idDict* strPairs );
	int Parse( idLexer* lexer , bool forwardLexer = false );
	template<class T>
	T* Get( idStr name )
	{
		for( auto* item : items ) if( item->Name() == name )
			{
				return static_cast<T*>( item );
			}
		return nullptr;
	}
private:
	idList<parsable*> items;
};
#pragma endregion

#pragma region GLTF Object parsing
class gltfPropertyArray;
class gltfPropertyItem
{
public:
	gltfPropertyItem() : array( nullptr ) { }
	gltfPropertyArray* array;
	idToken item;
};

class gltfPropertyArray
{
public:
	gltfPropertyArray( idLexer* Parser, bool AoS = true );
	~gltfPropertyArray();
	struct Iterator
	{
		gltfPropertyArray* array;
		gltfPropertyItem* p;
		gltfPropertyItem& operator*()
		{
			return *p;
		}
		bool operator != ( Iterator& rhs )
		{
			return p != rhs.p;
		}
		void operator ++();
	};
	gltfPropertyArray::Iterator begin();
	gltfPropertyArray::Iterator end();
private:
	bool iterating;
	bool dirty;
	int index;
	idLexer* parser;
	idList<gltfPropertyItem*> properties;
	gltfPropertyItem* endPtr;
	bool isArrayOfStructs;
};
#pragma endregion

class GLTF_Parser
{
public:
	~GLTF_Parser()
	{
		Shutdown();
	}
	GLTF_Parser();
	void Shutdown();
	bool Parse();
	bool Load( idStr filename );
	bool loadGLB( idStr filename );

	//current/last loaded gltf asset and index offsets
	gltfData* currentAsset;
	idStr currentFile;
private:
	void SetNodeParent( gltfNode* node, gltfNode* parent = nullptr );
	//void CreateBgfxData();

	void Parse_ASSET( idToken& token );
	void Parse_CAMERAS( idToken& token );
	void Parse_SCENE( idToken& token );
	void Parse_SCENES( idToken& token );
	void Parse_NODES( idToken& token );
	void Parse_MATERIALS( idToken& token );
	void Parse_MESHES( idToken& token );
	void Parse_TEXTURES( idToken& token );
	void Parse_IMAGES( idToken& token );
	void Parse_ACCESSORS( idToken& token );
	void Parse_BUFFERVIEWS( idToken& token );
	void Parse_SAMPLERS( idToken& token );
	void Parse_BUFFERS( idToken& token );
	void Parse_ANIMATIONS( idToken& token );
	void Parse_SKINS( idToken& token );
	void Parse_EXTENSIONS( idToken& token );
	void Parse_EXTENSIONS_USED( idToken& token );
	void Parse_EXTENSIONS_REQUIRED( idToken& token );


	gltfProperty ParseProp( idToken& token );
	gltfProperty ResolveProp( idToken& token );

	idLexer	parser;
	idToken	token;

	bool buffersDone;
	bool bufferViewsDone;
};

class gltfManager
{
public:
	static bool ExtractIdentifier( idStr& filename , int& id, idStr& name );
};