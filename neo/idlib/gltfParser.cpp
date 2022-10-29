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

#include "precompiled.h"
#pragma hdrstop

/// <TODO>
///	Clean up registerd gltfItem_Extra's
/// Clean up loaded gltfData;
/// </TODO>

static const unsigned int gltfChunk_Type_JSON =  0x4E4F534A; //1313821514
static const unsigned int gltfChunk_Type_BIN =  0x004E4942; //5130562

idCVar gltf_parseVerbose( "gltf_parseVerbose", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "print gltf json data while parsing" );
idCVar gltfParser_PrefixNodeWithID( "gltfParser_PrefixNodeWithID", "0", CVAR_SYSTEM | CVAR_BOOL, "The node's id is prefixed to the node's name during load" );
//
//gltf_sampler_wrap_type_map s_samplerWrapTypeMap[] = {
//	//33071 CLAMP_TO_EDGE
//	33071, BGFX_SAMPLER_U_CLAMP, BGFX_SAMPLER_V_CLAMP,
//	//33648 MIRRORED_REPEAT
//	33648, BGFX_SAMPLER_U_MIRROR, BGFX_SAMPLER_V_MIRROR,
//	//10497 REPEAT
//	10497, BGFX_SAMPLER_NONE , BGFX_SAMPLER_NONE ,
//	0,0,0
//};
////todo
//gltf_sampler_mag_type_map s_samplerMagTypeMap[] = {
//	//9728 NEAREST //mag/min
//	9728 , BGFX_SAMPLER_MIN_ANISOTROPIC
//	//9729 LINEAR // mag/min
//	//
//	//9984 NEAREST_MIPMAP_NEAREST //min
//	//
//	//9985 LINEAR_MIPMAP_NEAREST //min
//	//
//	//9986 NEAREST_MIPMAP_LINEAR  //min
//	//
//	//9987 LINEAR_MIPMAP_LINEAR //min
//};

//uint64_t GetSamplerFlags( gltfSampler * sampler) {
//	// Ignore the sampling options for filter -- always use mag: LINEAR and min: LINEAR_MIPMAP_LINEAR
//	uint64_t flags ;//= BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_ANISOTROPIC ;
//	int i = -1;
//	while ( s_samplerWrapTypeMap[++i].id != 0 )
//	{
//		if ( s_samplerWrapTypeMap[i].id == sampler->wrapS)
//			flags |= s_samplerWrapTypeMap[i].bgfxFlagU;
//		if ( s_samplerWrapTypeMap[i].id == sampler->wrapT )
//			flags |= s_samplerWrapTypeMap[i].bgfxFlagV;
//	}
//	return flags;
//}


gltf_mesh_attribute_map s_meshAttributeMap[] =
{
	"POSITION",		gltfMesh_Primitive_Attribute::Type::Position,	3,
	"NORMAL",		gltfMesh_Primitive_Attribute::Type::Normal,		3,
	"TANGENT",		gltfMesh_Primitive_Attribute::Type::Tangent,	3,
	"TEXCOORD_0",	gltfMesh_Primitive_Attribute::Type::TexCoord0,	2,
	"TEXCOORD_1",	gltfMesh_Primitive_Attribute::Type::TexCoord1,	2,
	"TEXCOORD_2",	gltfMesh_Primitive_Attribute::Type::TexCoord2,	2,
	"TEXCOORD_3",	gltfMesh_Primitive_Attribute::Type::TexCoord3,	2,
	"TEXCOORD_4",	gltfMesh_Primitive_Attribute::Type::TexCoord4,	2,
	"TEXCOORD_5",	gltfMesh_Primitive_Attribute::Type::TexCoord5,	2,
	"TEXCOORD_6",	gltfMesh_Primitive_Attribute::Type::TexCoord6,	2,
	"TEXCOORD_7",	gltfMesh_Primitive_Attribute::Type::TexCoord7,	2,
	"COLOR_0",		gltfMesh_Primitive_Attribute::Type::Color0,		4,
	"COLOR_1",		gltfMesh_Primitive_Attribute::Type::Color1,		4,
	"COLOR_2",		gltfMesh_Primitive_Attribute::Type::Color2,		4,
	"COLOR_3",		gltfMesh_Primitive_Attribute::Type::Color3,		4,
	"WEIGHTS_0",	gltfMesh_Primitive_Attribute::Type::Weight,		4,
	"JOINTS_0",		gltfMesh_Primitive_Attribute::Type::Joints,		4,
	"",				gltfMesh_Primitive_Attribute::Type::Count
};

gltfMesh_Primitive_Attribute::Type GetAttributeEnum( const char* str , uint* elementSize = nullptr )
{
	int i = -1;
	while( s_meshAttributeMap[++i].attib != gltfMesh_Primitive_Attribute::Type::Count )
		if( !idStr::Icmp( s_meshAttributeMap[i].stringID, str ) )
		{
			if( elementSize != nullptr )
			{
				*elementSize = s_meshAttributeMap[i].elementSize;
			}
			return s_meshAttributeMap[i].attib;
		}

	return gltfMesh_Primitive_Attribute::Type::Count;
}

//https://github.com/KhronosGroup/glTF/issues/832
//gltf_accessor_component_type_map<bgfx::AttribType::Enum> s_bgfxComponentTypeMap[] = {
//	"signed byte",		5120,	bgfx::AttribType::Count, 1 ,
//	"unsigned byte",	5121,	bgfx::AttribType::Uint8, 1 ,
//	"signed short",		5122,	bgfx::AttribType::Int16, 2 ,
//	"unsigned short",	5123,	bgfx::AttribType::Count, 2 ,
//	"unsigned int",		5125,	bgfx::AttribType::Count, 4 ,
//	"float",			5126,	bgfx::AttribType::Float, 4 ,
//	"double",			5130,	bgfx::AttribType::Float, 8 ,
//	"",					0,		bgfx::AttribType::Count, 0
//};
//
gltf_accessor_component_type_map<gltf_accessor_component::Type> s_nativeComponentTypeMap[] =
{
	"signed byte",		5120,	gltf_accessor_component::Type::_byte, 1 ,
	"unsigned byte",	5121,	gltf_accessor_component::Type::_uByte, 1 ,
	"signed short",		5122,	gltf_accessor_component::Type::_short, 2 ,
	"unsigned short",	5123,	gltf_accessor_component::Type::_uShort, 2 ,
	"unsigned int",		5125,	gltf_accessor_component::Type::_uInt, 4 ,
	"float",			5126,	gltf_accessor_component::Type::_float, 4 ,
	"double",			5130,	gltf_accessor_component::Type::_double, 8 ,
	"",					0,		gltf_accessor_component::Type::Count, 0
};
//
gltf_accessor_component::Type GetComponentTypeEnum( int id  , uint* sizeInBytes = nullptr )
{
	int i = -1;
	while( s_nativeComponentTypeMap[++i].id != 0 )
		if( s_nativeComponentTypeMap[i].id == id )
		{
			if( sizeInBytes != nullptr )
			{
				*sizeInBytes = s_nativeComponentTypeMap[i].sizeInBytes;
			}

			return s_nativeComponentTypeMap[i].type;
		}

	return gltf_accessor_component::Type::Count;
}

//some arbitrary amount for now.
#define GLTF_MAX_CHUNKS 32

idList<gltfData*>	gltfData::dataList;
idHashIndex			gltfData::fileDataHash;
gltfItemArray*		gltfItem_Extra::items = new gltfItemArray();

//Helper macros for gltf data deserialize
//NOTE: gltfItems that deviate from the default SET(T*) function cannot be handled with itemref macro.
// target must be an gltfArrayItem.
// type with name will be added to the array.
#define GLTFARRAYITEM(target,name,type) auto * name = new type (#name); target.AddItemDef((parsable*)name)
// name must point to an existing valid entry
// name->Set		(&target->name);
#define GLTFARRAYITEMREF(target,name) name->Set(&target->name)

void gltfPropertyArray::Iterator::operator ++()
{

	//check if by modification, we are iterating again.
	//custom not AOS things can do this since it is not nicely guarded by braces
	if( array->dirty && ( !array->iterating &&  !array->isArrayOfStructs ) )
	{
		array->iterating = array->parser->PeekTokenString( "," );
		if( array->iterating )
		{
			array->properties.AssureSizeAlloc( array->properties.Num() + 1, idListNewElement<gltfPropertyItem> );
			array->parser->ExpectTokenString( "," );
		}
	}

	if( array->iterating )
	{
		p = array->properties[array->properties.Num() - 1];
		p->array = array;
		if( array->isArrayOfStructs )
		{
			array->parser->ParseBracedSection( p->item );
		}
		else
		{
			idToken token;
			array->parser->ExpectAnyToken( &token );
			p->item = token;
		}
		array->iterating = array->parser->PeekTokenString( "," );
		if( array->iterating )
		{
			array->properties.AssureSizeAlloc( array->properties.Num() + 1, idListNewElement<gltfPropertyItem> );
			array->parser->ExpectTokenString( "," );
		}
	}
	else
	{
		if( array->dirty )
		{
			p = array->endPtr;
			array->dirty = false;
		}
		else if( array->index + 1 < array->properties.Num() )
		{
			p = array->properties[++array->index];
		}
		else
		{
			p = array->endPtr;
		}
	}
}

gltfPropertyArray::~gltfPropertyArray()
{
	delete endPtr;
	properties.DeleteContents( true );
}

gltfPropertyArray::gltfPropertyArray( idLexer* Parser, bool AoS/* = true */ )
	: parser( Parser ), iterating( true ), dirty( true ), index( 0 ), isArrayOfStructs( AoS )
{
	properties.AssureSizeAlloc( 32, idListNewElement<gltfPropertyItem> );
	properties.SetNum( 0 );
	endPtr = new gltfPropertyItem();
	endPtr->array = this;
}

gltfPropertyArray::Iterator gltfPropertyArray::begin()
{
	if( iterating )
	{
		if( isArrayOfStructs && !parser->PeekTokenString( "{" ) )
		{
			if( !parser->ExpectTokenString( "[" ) && parser->PeekTokenString( "{" ) )
			{
				common->FatalError( "Malformed gltf array" );
			}
		}
		else if( !isArrayOfStructs && !parser->ExpectTokenString( "[" ) )
		{
			common->FatalError( "Malformed gltf array" );
		}

		properties.AssureSizeAlloc( properties.Num() + 1, idListNewElement<gltfPropertyItem> );
		gltfPropertyItem* start = properties[0];
		start->array = this;
		if( isArrayOfStructs )
		{
			parser->ParseBracedSection( start->item );
		}
		else
		{
			idToken token;
			parser->ExpectAnyToken( &token );
			start->item = token;
		}
		iterating = parser->PeekTokenString( "," );
		if( iterating )
		{
			properties.AssureSizeAlloc( properties.Num() + 1, idListNewElement<gltfPropertyItem> );
			parser->ExpectTokenString( "," );
		}

		return Iterator{ this , start };
	}
	index = 0;
	return Iterator{ this , properties[index]};
}

gltfPropertyArray::Iterator gltfPropertyArray::end()
{
	return Iterator{ this , endPtr};
}

gltfItemArray::~gltfItemArray()
{
	items.DeleteContents( true );
}

int gltfItemArray::Fill( idLexer* lexer, idDict* strPairs )
{
	idToken token;
	bool parsing = true;
	int parseCount = 0;
	lexer->ExpectTokenString( "{" );
	while( parsing && !lexer->PeekTokenString( "}" ) && lexer->ExpectAnyToken( &token ) )
	{
		lexer->ExpectTokenString( ":" );
		idStr key = token;
		idStr value;
		key.StripTrailingWhitespace();
		if( lexer->PeekTokenString( "{" ) )
		{
			lexer->ParseBracedSectionExact( value );
			value.StripTrailingWhitespace();
			strPairs->Set( key, value );
		}
		else
		{
			lexer->ExpectAnyToken( &token );
			value = token;
			value.StripTrailingWhitespace();
			key.StripTrailingWhitespace();
			strPairs->Set( key, token );
		}

		parseCount++;
		parsing = lexer->PeekTokenString( "," );
		if( parsing )
		{
			lexer->ExpectTokenString( "," );
		}
	}
	lexer->ExpectTokenString( "}" );
	return parseCount;
}

int gltfItemArray::Parse( idLexer* lexer, bool forwardLexer/* = false*/ )
{
	idToken token;
	bool parsing = true;
	int parseCount = 0;
	lexer->ExpectTokenString( "{" );
	while( parsing && !lexer->PeekTokenString( "}" ) && lexer->ExpectAnyToken( &token ) )
	{
		lexer->ExpectTokenString( ":" );
		bool parsed = false;
		for( auto item : items )
		{
			if( item->Name() == token )
			{
				lexer->ExpectAnyToken( &token );
				if( forwardLexer )
				{
					item->parse( token , lexer );
				}
				else
				{
					item->parse( token );
				}
				parsed = true;
				break;
			}
		}
		if( !parsed )
		{
			lexer->SkipBracedSection();
		}
		else
		{
			parseCount++;
		}

		parsing = lexer->PeekTokenString( "," );
		if( parsing )
		{
			lexer->ExpectTokenString( "," );
		}
	}
	lexer->ExpectTokenString( "}" );
	return parseCount;
}

byte* gltfData::AddData( int size, int* bufferID/*=nullptr*/ )
{
	if( totalChunks == -1 )
	{
		json  = ( byte* ) Mem_ClearedAlloc( size, TAG_IDLIB_GLTF );
		totalChunks++;
		jsonDataLength = size;
		return json;
	}

	int id = totalChunks;

	if( data == nullptr )
	{
		data = ( byte** ) Mem_ClearedAlloc( GLTF_MAX_CHUNKS * sizeof( byte* ), TAG_IDLIB_GLTF );
	}
	data[totalChunks++] = ( byte* ) Mem_ClearedAlloc( size, TAG_IDLIB_GLTF );

	if( bufferID )
	{
		*bufferID = id;
	}

	return data[id];
}

bool gltfItem_uri::Convert()
{
	//HVG_TODO
	// uri cache.
	//read data
	int length = fileSystem->ReadFile( item->c_str(), NULL );
	idFile* file = fileSystem->OpenFileRead( item->c_str() );

	//create buffer
	gltfBuffer* buffer = data->Buffer();
	buffer->parent = data;
	buffer->name = item->c_str();
	buffer->byteLength = length;
	int bufferID = -1;
	byte* dest = data->AddData( length, &bufferID );

	if( file->Read( dest, length ) != length )
	{
		common->FatalError( "Could not read %s", item->c_str() );
	}

	if( gltf_parseVerbose.GetBool() )
	{
		common->Warning( "gltf Uri %s loaded into buffer[ %i ]", buffer->name.c_str(), bufferID );
	}

	//create bufferview
	//if bufferview is not set, this is an buffer.uri.
	//A bufferview should aready be defined if the buffer is used.
	if( bufferView != nullptr )
	{
		*bufferView = data->BufferViewList().Num();
		gltfBufferView* newBufferView = data->BufferView();
		newBufferView->buffer = bufferID;
		newBufferView->byteLength = length;
		newBufferView->parent = data;
	}

	fileSystem->CloseFile( file );

	return false;
}

void gltfItem_Extra::parse( idToken& token )
{
	parser->UnreadToken( &token );
	parser->ParseBracedSectionExact( item->json );

	idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
	lexer.LoadMemory( item->json, item->json.Size(), "gltfItem_Extra", 0 );
	items->Fill( &lexer, &item->strPairs );
	lexer.Reset();
	items->Parse( &lexer , true );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", item->json.c_str() );
	}
}

void gltfItem_Extra::Register( parsable* extra )
{
	common->DPrintf( "...Registering gltf Extra \"%s\" total(%i)\n", extra->Name().c_str(), items->Num() );
	items->AddItemDef( extra );
}

void gltfItem_animation_sampler::parse( idToken& token )
{
	gltfItemArray animSampler;
	GLTFARRAYITEM( animSampler, input,			gltfItem_integer );
	GLTFARRAYITEM( animSampler, interpolation,	gltfItem );
	GLTFARRAYITEM( animSampler, output,			gltfItem_integer );
	GLTFARRAYITEM( animSampler, extensions,		gltfItem );
	GLTFARRAYITEM( animSampler, extras,			gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfAnimation_Sampler", 0 );

		item->AssureSizeAlloc( item->Num() + 1, idListNewElement<gltfAnimation_Sampler> );
		gltfAnimation_Sampler* gltfAnimSampler = ( *item )[item->Num() - 1];

		GLTFARRAYITEMREF( gltfAnimSampler, input );
		GLTFARRAYITEMREF( gltfAnimSampler, interpolation );
		GLTFARRAYITEMREF( gltfAnimSampler, output );
		GLTFARRAYITEMREF( gltfAnimSampler, extensions );
		extras->Set( &gltfAnimSampler->extras, &lexer );
		animSampler.Parse( &lexer );
		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}

		gltfAnimSampler->intType = gltfAnimation_Sampler::resolveType( gltfAnimSampler->interpolation );
	}
	parser->ExpectTokenString( "]" );
}

void gltfItem_animation_channel_target::parse( idToken& token )
{
	parser->UnreadToken( &token );
	gltfItemArray animChannelTarget;
	GLTFARRAYITEM( animChannelTarget, node,			gltfItem_integer );
	GLTFARRAYITEM( animChannelTarget, path,			gltfItem );
	GLTFARRAYITEM( animChannelTarget, extensions,	gltfItem );
	GLTFARRAYITEM( animChannelTarget, extras,		gltfItem_Extra );

	GLTFARRAYITEMREF( item,	node );
	GLTFARRAYITEMREF( item,	path );
	GLTFARRAYITEMREF( item,	extensions );
	extras->Set( &item->extras, parser );
	animChannelTarget.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}

	item->TRS = gltfAnimation_Channel_Target::resolveType( item->path );
}

void gltfItem_animation_channel::parse( idToken& token )
{
	//parser->UnreadToken( &token );
	gltfItemArray anim;
	GLTFARRAYITEM( anim, sampler,		gltfItem_integer );
	GLTFARRAYITEM( anim, target,		gltfItem_animation_channel_target );
	GLTFARRAYITEM( anim, extensions,	gltfItem );
	GLTFARRAYITEM( anim, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfAnimation_Channel", 0 );


		item->AssureSizeAlloc( item->Num() + 1, idListNewElement<gltfAnimation_Channel> );
		gltfAnimation_Channel* gltfAnimationChannel = ( *item )[item->Num() - 1];

		GLTFARRAYITEMREF( gltfAnimationChannel, sampler );
		target->Set( &gltfAnimationChannel->target, &lexer );
		GLTFARRAYITEMREF( gltfAnimationChannel, extensions );
		extras->Set( &gltfAnimationChannel->extras, &lexer );
		anim.Parse( &lexer );
		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", token.c_str() );
		}

	}
	parser->ExpectTokenString( "]" );
}

void gltfItem_mesh_primitive::parse( idToken& token )
{
	gltfItemArray prim;
	GLTFARRAYITEM( prim, attributes,	gltfItem_mesh_primitive_attribute );
	GLTFARRAYITEM( prim, indices,		gltfItem_integer );
	GLTFARRAYITEM( prim, material,		gltfItem_integer );
	GLTFARRAYITEM( prim, mode,			gltfItem_integer );
	GLTFARRAYITEM( prim, target,		gltfItem );
	GLTFARRAYITEM( prim, extensions,	gltfItem );
	GLTFARRAYITEM( prim, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfItem_mesh_primitiveB", 0 );


		item->AssureSizeAlloc( item->Num() + 1, idListNewElement<gltfMesh_Primitive> );
		gltfMesh_Primitive* gltfMeshPrim = ( *item )[item->Num() - 1];

		attributes->Set( &gltfMeshPrim->attributes, &lexer );
		GLTFARRAYITEMREF( gltfMeshPrim, indices	);
		GLTFARRAYITEMREF( gltfMeshPrim, material );
		GLTFARRAYITEMREF( gltfMeshPrim, mode );
		GLTFARRAYITEMREF( gltfMeshPrim, target );
		GLTFARRAYITEMREF( gltfMeshPrim, extensions );
		extras->Set( &gltfMeshPrim->extras, &lexer );
		prim.Parse( &lexer );
		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser->ExpectTokenString( "]" );
}

void gltfItem_mesh_primitive_attribute::parse( idToken& token )
{
	bool parsing = true;
	while( parsing && parser->ExpectAnyToken( &token ) )
	{

		item->AssureSizeAlloc( item->Num() + 1, idListNewElement<gltfMesh_Primitive_Attribute> );
		gltfMesh_Primitive_Attribute* attr = ( *item )[item->Num() - 1];
		parser->ExpectTokenString( ":" );
		attr->attributeSemantic = token;
		attr->type = GetAttributeEnum( attr->attributeSemantic.c_str(), &attr->elementSize );
		parser->ExpectAnyToken( &token );
		attr->accessorIndex = token.GetIntValue();
		parsing = parser->PeekTokenString( "," );
		if( parsing )
		{
			parser->ExpectTokenString( "," );
		}
	}
	parser->ExpectTokenString( "}" );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_integer_array::parse( idToken& token )
{
	parser->UnreadToken( &token );
	gltfPropertyArray array = gltfPropertyArray( parser, false );
	for( auto& prop : array )
	{
		idStr neg;
		int& value = item->Alloc();
		value = prop.item.GetIntValue();

		if( prop.item.type == TT_PUNCTUATION && prop.item == "-" )
		{
			parser->ExpectTokenType( TT_NUMBER, 0, &prop.item );
			value = -( prop.item.GetIntValue() );
			neg = "-";
		}
		else if( prop.item.type == TT_NUMBER )
		{
			value = prop.item.GetIntValue();
		}
		else
		{
			common->FatalError( "parse error" );
		}

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s%s", neg.c_str(), prop.item.c_str() );
		}
	}
	parser->ExpectTokenString( "]" );
}

void gltfItem_number_array::parse( idToken& token )
{

	parser->UnreadToken( &token );
	gltfPropertyArray array = gltfPropertyArray( parser, false );
	for( auto& prop : array )
	{
		idStr neg;
		double& value = item->Alloc();
		if( prop.item.type == TT_PUNCTUATION && prop.item == "-" )
		{
			parser->ExpectTokenType( TT_NUMBER, 0, &prop.item );
			value = -( prop.item.GetDoubleValue() );
			neg = "-";
		}
		else if( prop.item.type == TT_NUMBER )
		{
			value = prop.item.GetDoubleValue();
		}
		else
		{
			common->FatalError( "parse error" );
		}

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s%s", neg.c_str(), prop.item.c_str() );
		}
	}
	parser->ExpectTokenString( "]" );
}

void gltfItem_vec4::parse( idToken& token )
{

	auto* numbers = new gltfItem_number_array( "" );
	idList<double> numberarray;
	numbers->Set( &numberarray, parser );
	numbers->parse( token );
	if( numbers->item->Num() != 4 )
	{
		common->FatalError( "gltfItem_vec4 : missing arguments, expected 4, got %i", numbers->item->Num() );
	}

	double* val = numbers->item->Ptr();
	*item = idVec4( val[0], val[1], val[2], val[3] );
}

void gltfItem_vec3::parse( idToken& token )
{
	auto* numbers = new gltfItem_number_array( "" );
	idList<double> numberarray;
	numbers->Set( &numberarray, parser );
	numbers->parse( token );
	if( numbers->item->Num() != 3 )
	{
		common->FatalError( "gltfItem_vec3 : missing arguments, expected 3, got %i", numbers->item->Num() );
	}

	double* val = numbers->item->Ptr();
	*item = idVec3( val[0], val[1], val[2] );
}

void gltfItem_vec2::parse( idToken& token )
{
	auto* numbers = new gltfItem_number_array( "" );
	idList<double> numberarray;
	numbers->Set( &numberarray, parser );
	numbers->parse( token );
	if( numbers->item->Num() != 2 )
	{
		common->FatalError( "gltfItem_vec3 : missing arguments, expected 2, got %i", numbers->item->Num() );
	}

	double* val = numbers->item->Ptr();
	*item = idVec2( val[0], val[1] );
}

void gltfItem_quat::parse( idToken& token )
{
	auto* numbers = new gltfItem_number_array( "" );
	idList<double> numberarray;
	numbers->Set( &numberarray, parser );
	numbers->parse( token );
	if( numbers->item->Num() != 4 )
	{
		common->FatalError( "gltfItem_quat : missing arguments, expectd 4, got %i", numbers->item->Num() );
	}

	double* val = numbers->item->Ptr();
	*item = idQuat( val[0] , val[1] , val[2] , val[3] );
}

void gltfItem_mat4::parse( idToken& token )
{
	auto* numbers = new gltfItem_number_array( "" );
	idList<double> numberarray;
	numbers->Set( &numberarray, parser );
	numbers->parse( token );
	if( numbers->item->Num() != 16 )
	{
		common->FatalError( "gltfItem_mat4 : missing arguments, expectd 16, got %i", numbers->item->Num() );
	}

	double* val = numbers->item->Ptr();
	*item = idMat4(
				val[0], val[1], val[2], val[3],
				val[4], val[5], val[6], val[7],
				val[8], val[9], val[10], val[11],
				val[12], val[13], val[14], val[15]
			);
}

void gltfItem_accessor_sparse::parse( idToken& token )
{
	parser->Warning( "%s is untested!", "gltfItem_accessor_sparse" );

	gltfItemArray sparse;
	GLTFARRAYITEM( sparse, count,		gltfItem_integer );
	GLTFARRAYITEM( sparse, indices,		gltfItem_accessor_sparse_indices );
	GLTFARRAYITEM( sparse, values,		gltfItem_accessor_sparse_values );
	GLTFARRAYITEM( sparse, extensions,	gltfItem );
	GLTFARRAYITEM( sparse, extras,		gltfItem_Extra );

	GLTFARRAYITEMREF( item, count );
	indices->Set( &item->indices, parser );
	values->Set( &item->values, parser );
	GLTFARRAYITEMREF( item, extensions );
	extras->Set( &item->extras, parser );
	sparse.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_accessor_sparse_values::parse( idToken& token )
{
	parser->Warning( "%s is untested!", "gltfItem_accessor_sparse_values" );

	gltfItemArray values;
	GLTFARRAYITEM( values, bufferView, gltfItem_integer );
	GLTFARRAYITEM( values, byteOffset, gltfItem_integer );
	GLTFARRAYITEM( values, extensions, gltfItem );
	GLTFARRAYITEM( values, extras, gltfItem_Extra );

	GLTFARRAYITEMREF( item, bufferView );
	GLTFARRAYITEMREF( item, byteOffset );
	GLTFARRAYITEMREF( item, extensions );
	extras->Set( &item->extras, parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_accessor_sparse_indices::parse( idToken& token )
{
	parser->Warning( "%s is untested!", "gltfItem_accessor_sparse_indices" );

	gltfItemArray indices;
	GLTFARRAYITEM( indices, bufferView, gltfItem_integer );
	GLTFARRAYITEM( indices, byteOffset, gltfItem_integer );
	GLTFARRAYITEM( indices, componentType, gltfItem_integer );
	GLTFARRAYITEM( indices, extensions, gltfItem );
	GLTFARRAYITEM( indices, extras, gltfItem_Extra );

	GLTFARRAYITEMREF( item, bufferView );
	GLTFARRAYITEMREF( item, byteOffset );
	GLTFARRAYITEMREF( item, componentType );
	GLTFARRAYITEMREF( item, extensions );
	extras->Set( &item->extras, parser );
	indices.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_camera_orthographic::parse( idToken& token )
{
	gltfPropertyArray array = gltfPropertyArray( parser );
	for( auto& prop : array )
	{
		common->Printf( "%s", prop.item.c_str() );
	}
	parser->ExpectTokenString( "]" );
}

void gltfItem_camera_perspective::parse( idToken& token )
{
	parser->UnreadToken( &token );
	gltfItemArray cameraPerspective;
	GLTFARRAYITEM( cameraPerspective, aspectRatio, gltfItem_number );
	GLTFARRAYITEM( cameraPerspective, yfov, gltfItem_number );
	GLTFARRAYITEM( cameraPerspective, zfar, gltfItem_number );
	GLTFARRAYITEM( cameraPerspective, znear, gltfItem_number );
	GLTFARRAYITEM( cameraPerspective, extensions, gltfItem );
	GLTFARRAYITEM( cameraPerspective, extras, gltfItem_Extra );

	GLTFARRAYITEMREF( item, aspectRatio );
	GLTFARRAYITEMREF( item, yfov );
	GLTFARRAYITEMREF( item, zfar );
	GLTFARRAYITEMREF( item, znear );
	GLTFARRAYITEMREF( item, extensions );
	extras->Set( &item->extras, parser );
	cameraPerspective.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_occlusion_texture::parse( idToken& token )
{
	parser->UnreadToken( &token );
	gltfItemArray textureInfo;
	GLTFARRAYITEM( textureInfo, index,				gltfItem_integer );
	GLTFARRAYITEM( textureInfo, texCoord,			gltfItem_integer );
	GLTFARRAYITEM( textureInfo, strength,			gltfItem_number );
	GLTFARRAYITEM( textureInfo, extensions,			gltfItem_texture_info_extensions );
	GLTFARRAYITEM( textureInfo, extras,				gltfItem_Extra );

	GLTFARRAYITEMREF( item, index );
	GLTFARRAYITEMREF( item, texCoord );
	GLTFARRAYITEMREF( item, strength );
	extensions->Set( &item->extensions, parser );
	extras->Set( &item->extras, parser );
	textureInfo.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_normal_texture::parse( idToken& token )
{
	parser->UnreadToken( &token );
	gltfItemArray textureInfo;
	GLTFARRAYITEM( textureInfo, index,				gltfItem_integer );
	GLTFARRAYITEM( textureInfo, texCoord,			gltfItem_integer );
	GLTFARRAYITEM( textureInfo, scale,				gltfItem_number );
	GLTFARRAYITEM( textureInfo, extensions,			gltfItem_texture_info_extensions );
	GLTFARRAYITEM( textureInfo, extras,				gltfItem_Extra );

	GLTFARRAYITEMREF( item,					index );
	GLTFARRAYITEMREF( item,					texCoord );
	GLTFARRAYITEMREF( item,					scale );
	extensions->Set( &item->extensions,	parser );
	extras->Set( &item->extras,		parser );
	textureInfo.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_texture_info::parse( idToken& token )
{
	parser->UnreadToken( &token );
	gltfItemArray textureInfo;
	GLTFARRAYITEM( textureInfo, index,				gltfItem_integer );
	GLTFARRAYITEM( textureInfo, texCoord,			gltfItem_integer );
	GLTFARRAYITEM( textureInfo, extensions,			gltfItem_texture_info_extensions );
	GLTFARRAYITEM( textureInfo, extras,				gltfItem_Extra );

	GLTFARRAYITEMREF( item, index );
	GLTFARRAYITEMREF( item, texCoord );
	extensions->Set( &item->extensions, parser );
	extras->Set( &item->extras, parser );
	textureInfo.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_pbrMetallicRoughness::parse( idToken& token )
{
	parser->UnreadToken( &token );
	gltfItemArray pbrMetallicRoughness;
	GLTFARRAYITEM( pbrMetallicRoughness, baseColorFactor,			gltfItem_vec4 );
	GLTFARRAYITEM( pbrMetallicRoughness, baseColorTexture,			gltfItem_texture_info );
	GLTFARRAYITEM( pbrMetallicRoughness, metallicFactor,			gltfItem_number );
	GLTFARRAYITEM( pbrMetallicRoughness, roughnessFactor,			gltfItem_number );
	GLTFARRAYITEM( pbrMetallicRoughness, metallicRoughnessTexture,	gltfItem_texture_info );
	GLTFARRAYITEM( pbrMetallicRoughness, extensions,				gltfItem );
	GLTFARRAYITEM( pbrMetallicRoughness, extras,					gltfItem_Extra );

	baseColorFactor->Set( &item->baseColorFactor,	parser );
	baseColorTexture->Set( &item->baseColorTexture, parser );
	GLTFARRAYITEMREF( item, metallicFactor );
	GLTFARRAYITEMREF( item, roughnessFactor );
	metallicRoughnessTexture->Set( &item->metallicRoughnessTexture, parser );
	GLTFARRAYITEMREF( item, extensions );
	extras->Set( &item->extras, parser );
	pbrMetallicRoughness.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_TextureInfo_KHR_texture_transform::parse( idToken& token )
{
	parser->UnreadToken( &token );
	gltfItemArray texureTransform;
	GLTFARRAYITEM( texureTransform, offset,		gltfItem_vec2 );
	GLTFARRAYITEM( texureTransform, rotation,	gltfItem_number );
	GLTFARRAYITEM( texureTransform, scale,		gltfItem_vec2 );
	GLTFARRAYITEM( texureTransform, texCoord,	gltfItem_integer );

	item->KHR_texture_transform = new gltfExt_KHR_texture_transform();

	offset->Set( &item->KHR_texture_transform->offset,	parser );
	GLTFARRAYITEMREF( item->KHR_texture_transform,			rotation );
	scale->Set( &item->KHR_texture_transform->scale,	parser );
	GLTFARRAYITEMREF( item->KHR_texture_transform,			texCoord );
	texureTransform.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_Material_KHR_materials_pbrSpecularGlossiness::parse( idToken& token )
{
	parser->UnreadToken( &token );
	gltfItemArray khrPbr;
	GLTFARRAYITEM( khrPbr, diffuseFactor,				gltfItem_vec4 );
	GLTFARRAYITEM( khrPbr, diffuseTexture,				gltfItem_texture_info );
	GLTFARRAYITEM( khrPbr, specularFactor,				gltfItem_vec3 );
	GLTFARRAYITEM( khrPbr, glossinessFactor,			gltfItem_number );
	GLTFARRAYITEM( khrPbr, specularGlossinessTexture,	gltfItem_texture_info );
	GLTFARRAYITEM( khrPbr, extensions,					gltfItem );
	GLTFARRAYITEM( khrPbr, extras,						gltfItem_Extra );

	item->KHR_materials_pbrSpecularGlossiness = new gltfExt_KHR_materials_pbrSpecularGlossiness();

	diffuseFactor->Set( &item->KHR_materials_pbrSpecularGlossiness->diffuseFactor,				parser	);
	diffuseTexture->Set( &item->KHR_materials_pbrSpecularGlossiness->diffuseTexture,				parser	);
	specularFactor->Set( &item->KHR_materials_pbrSpecularGlossiness->specularFactor,				parser	);
	GLTFARRAYITEMREF( item->KHR_materials_pbrSpecularGlossiness,						glossinessFactor );
	specularGlossinessTexture->Set( &item->KHR_materials_pbrSpecularGlossiness->specularGlossinessTexture,	parser	);
	GLTFARRAYITEMREF( item->KHR_materials_pbrSpecularGlossiness,							extensions	);
	extras->Set( &item->KHR_materials_pbrSpecularGlossiness->extras,						parser	);
	khrPbr.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_Node_KHR_lights_punctual::parse( idToken& token )
{

	parser->UnreadToken( &token );
	gltfItemArray xlight;
	GLTFARRAYITEM( xlight, light, gltfItem_integer );
	item->KHR_lights_punctual = new gltfNode_KHR_lights_punctual();
	light->Set( &item->KHR_lights_punctual->light );
	xlight.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_KHR_lights_punctual::parse( idToken& token )
{
	idToken localToken;
	parser->ExpectTokenString( "lights" );
	parser->ExpectTokenString( ":" );

	gltfItemArray light;
	GLTFARRAYITEM( light, color,		gltfItem_vec3 );
	GLTFARRAYITEM( light, intensity,	gltfItem_number );
	//GLTFARRAYITEM( light, spot,			gltfItem );
	GLTFARRAYITEM( light, type,			gltfItem );
	GLTFARRAYITEM( light, range,		gltfItem_number );
	GLTFARRAYITEM( light, name,			gltfItem );
	GLTFARRAYITEM( light, extensions,	gltfItem );
	GLTFARRAYITEM( light, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfNode_light", 0 );

		item->KHR_lights_punctual.AssureSizeAlloc(
			item->KHR_lights_punctual.Num() + 1,
			idListNewElement<gltfExt_KHR_lights_punctual> );

		gltfExt_KHR_lights_punctual* gltfLight =
			item->KHR_lights_punctual[item->KHR_lights_punctual.Num() - 1];

		color->Set( &gltfLight->color, &lexer	);
		GLTFARRAYITEMREF( gltfLight, intensity	);
		//GLTFARRAYITEMREF	(gltfLight, spot			);
		GLTFARRAYITEMREF( gltfLight, type	);
		GLTFARRAYITEMREF( gltfLight, range	);
		GLTFARRAYITEMREF( gltfLight, name	);
		GLTFARRAYITEMREF( gltfLight, extensions	);
		extras->Set( &gltfLight->extras, &lexer	);

		light.Parse( &lexer );

		gltfLight->intType = gltfExt_KHR_lights_punctual::resolveType( gltfLight->type );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}

	parser->ExpectTokenString( "]" );
	parser->ExpectTokenString( "}" );
}

void gltfItem_node_extensions::parse( idToken& token )
{
	parser->UnreadToken( &token );
	gltfItemArray extensions;
	GLTFARRAYITEM( extensions, KHR_lights_punctual, gltfItem_Node_KHR_lights_punctual );

	KHR_lights_punctual->Set( item, parser );
	extensions.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}

}

void gltfItem_material_extensions::parse( idToken& token )
{
	parser->UnreadToken( &token );

	gltfItemArray extensions;
	GLTFARRAYITEM( extensions, KHR_materials_pbrSpecularGlossiness, gltfItem_Material_KHR_materials_pbrSpecularGlossiness );

	KHR_materials_pbrSpecularGlossiness->Set( item, parser );
	extensions.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}

void gltfItem_texture_info_extensions::parse( idToken& token )
{
	parser->UnreadToken( &token );

	gltfItemArray extensions;
	GLTFARRAYITEM( extensions, KHR_texture_transform, gltfItem_TextureInfo_KHR_texture_transform );

	KHR_texture_transform->Set( item, parser );
	extensions.Parse( parser );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", token.c_str() );
	}
}


void GLTF_Parser::Shutdown()
{
	currentFile.FreeData();
	if( currentAsset )
	{
		delete currentAsset;
	}
	currentAsset = nullptr;
	buffersDone = false;
	bufferViewsDone = false;
}
GLTF_Parser::GLTF_Parser()
	: parser( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES ) , buffersDone( false ), bufferViewsDone( false ) { }

void GLTF_Parser::Parse_ASSET( idToken& token )
{
	idStr section;
	parser.ParseBracedSection( section );
	common->Printf( "%s\n", section.c_str() );
}

void GLTF_Parser::Parse_SCENE( idToken& token )
{
	currentAsset->DefaultScene() = parser.ParseInt();

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( " ^1 %s scene ^6 : ^8 %i", token.c_str(), currentAsset->DefaultScene() );
	}
}

void GLTF_Parser::Parse_SCENES( idToken& token )
{
	gltfItemArray scene;
	GLTFARRAYITEM( scene, nodes, gltfItem_integer_array );
	GLTFARRAYITEM( scene, name, gltfItem );
	GLTFARRAYITEM( scene, extensions, gltfItem );
	GLTFARRAYITEM( scene, extras, gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfScene", 0 );

		gltfScene* gltfscene = currentAsset->Scene();
		nodes->Set( &gltfscene->nodes, &lexer );
		GLTFARRAYITEMREF( gltfscene, name );
		GLTFARRAYITEMREF( gltfscene, extensions );
		extras->Set( &gltfscene->extras, &lexer );
		scene.Parse( &lexer );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_CAMERAS( idToken& token )
{
	gltfItemArray camera;
	GLTFARRAYITEM( camera, orthographic, gltfItem_camera_orthographic );
	GLTFARRAYITEM( camera, perspective, gltfItem_camera_perspective );
	GLTFARRAYITEM( camera, type, gltfItem );
	GLTFARRAYITEM( camera, name, gltfItem );
	GLTFARRAYITEM( camera, extensions, gltfItem );
	GLTFARRAYITEM( camera, extras, gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfCamera", 0 );

		gltfCamera* item = currentAsset->Camera();
		orthographic->Set( &item->orthographic, &lexer );
		perspective->Set( &item->perspective, &lexer );

		GLTFARRAYITEMREF( item, type );
		GLTFARRAYITEMREF( item, name );
		GLTFARRAYITEMREF( item, extensions );
		extras->Set( &item->extras, &lexer );

		camera.Parse( &lexer );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_NODES( idToken& token )
{

	gltfItemArray node;
	GLTFARRAYITEM( node, camera,		gltfItem_integer );
	GLTFARRAYITEM( node, children,		gltfItem_integer_array );
	GLTFARRAYITEM( node, skin,			gltfItem_integer );
	GLTFARRAYITEM( node, matrix,		gltfItem_mat4 );
	GLTFARRAYITEM( node, mesh,			gltfItem_integer );
	GLTFARRAYITEM( node, rotation,		gltfItem_quat );
	GLTFARRAYITEM( node, scale,			gltfItem_vec3 );
	GLTFARRAYITEM( node, translation,	gltfItem_vec3 );
	GLTFARRAYITEM( node, weights,		gltfItem_number_array );
	GLTFARRAYITEM( node, name,			gltfItem );
	GLTFARRAYITEM( node, extensions,	gltfItem_node_extensions );
	GLTFARRAYITEM( node, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfNode", 0 );

		gltfNode* gltfnode = currentAsset->Node();

		GLTFARRAYITEMREF( gltfnode, camera );
		matrix->Set( &gltfnode->matrix, &lexer );
		children->Set( &gltfnode->children, &lexer );
		GLTFARRAYITEMREF( gltfnode, skin );
		matrix->Set( &gltfnode->matrix, &lexer );
		GLTFARRAYITEMREF( gltfnode, mesh );
		rotation->Set( &gltfnode->rotation, &lexer );
		scale->Set( &gltfnode->scale, &lexer );
		translation->Set( &gltfnode->translation, &lexer );
		weights->Set( &gltfnode->weights, &lexer );
		GLTFARRAYITEMREF( gltfnode, name );
		extensions->Set( &gltfnode->extensions, &lexer );
		extras->Set( &gltfnode->extras, &lexer );
		node.Parse( &lexer );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_MATERIALS( idToken& token )
{
	gltfItemArray material;
	GLTFARRAYITEM( material, pbrMetallicRoughness,	gltfItem_pbrMetallicRoughness );
	GLTFARRAYITEM( material, normalTexture,			gltfItem_normal_texture );
	GLTFARRAYITEM( material, occlusionTexture,		gltfItem_occlusion_texture );
	GLTFARRAYITEM( material, emissiveTexture,		gltfItem_texture_info );
	GLTFARRAYITEM( material, emissiveFactor,		gltfItem_vec3 );
	GLTFARRAYITEM( material, alphaMode,				gltfItem );
	GLTFARRAYITEM( material, alphaCutoff,			gltfItem_number );
	GLTFARRAYITEM( material, doubleSided,			gltfItem_boolean );
	GLTFARRAYITEM( material, name,					gltfItem );
	GLTFARRAYITEM( material, extensions,			gltfItem_material_extensions );
	GLTFARRAYITEM( material, extras,				gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfMaterial", 0 );

		gltfMaterial* gltfmaterial = currentAsset->Material();

		pbrMetallicRoughness->Set( &gltfmaterial->pbrMetallicRoughness,	&lexer	);
		normalTexture->Set( &gltfmaterial->normalTexture,			&lexer	);
		occlusionTexture->Set( &gltfmaterial->occlusionTexture,		&lexer	);
		emissiveTexture->Set( &gltfmaterial->emissiveTexture,		&lexer	);
		emissiveFactor->Set( &gltfmaterial->emissiveFactor,		&lexer	);
		GLTFARRAYITEMREF( gltfmaterial, alphaMode	);
		GLTFARRAYITEMREF( gltfmaterial, alphaCutoff	);
		GLTFARRAYITEMREF( gltfmaterial, doubleSided	);
		GLTFARRAYITEMREF( gltfmaterial, name	);
		extensions->Set( &gltfmaterial->extensions,			&lexer	);
		extras->Set( &gltfmaterial->extras,				&lexer	);
		material.Parse( &lexer );

		gltfmaterial->intType = gltfMaterial::resolveAlphaMode( gltfmaterial->alphaMode );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_MESHES( idToken& token )
{
	gltfItemArray mesh;
	GLTFARRAYITEM( mesh, primitives,	gltfItem_mesh_primitive ); // object
	GLTFARRAYITEM( mesh, weights,		gltfItem_number_array ); //number[1 - *]
	GLTFARRAYITEM( mesh, name,			gltfItem );
	GLTFARRAYITEM( mesh, extensions,	gltfItem );
	GLTFARRAYITEM( mesh, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfMesh", 0 );

		gltfMesh* gltfmesh = currentAsset->Mesh();

		primitives->Set( &gltfmesh->primitives, &lexer );
		weights->Set( &gltfmesh->weights, &lexer );
		GLTFARRAYITEMREF( gltfmesh,  name );
		GLTFARRAYITEMREF( gltfmesh,  extensions );
		extras->Set( &gltfmesh->extras, &lexer );
		mesh.Parse( &lexer );
		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_TEXTURES( idToken& token )
{
	gltfItemArray texture;
	GLTFARRAYITEM( texture, sampler,	gltfItem_integer );
	GLTFARRAYITEM( texture, source,		gltfItem_integer );
	GLTFARRAYITEM( texture, name,		gltfItem );
	GLTFARRAYITEM( texture, extensions, gltfItem_texture_info_extensions );
	GLTFARRAYITEM( texture, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfTexture", 0 );

		gltfTexture* gltftexture = currentAsset->Texture();

		GLTFARRAYITEMREF( gltftexture,				sampler );
		GLTFARRAYITEMREF( gltftexture,				source );
		GLTFARRAYITEMREF( gltftexture,				name );
		extensions->Set( &gltftexture->extensions,	&lexer	);
		extras->Set( &gltftexture->extras,		&lexer );
		texture.Parse( &lexer );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_IMAGES( idToken& token )
{
	//reference impl
	gltfPropertyArray array = gltfPropertyArray( &parser );

	gltfItemArray propItems;
	auto uri		= new gltfItem_uri( "uri" );
	propItems.AddItemDef( ( parsable* )uri );
	auto mimeType	= new gltfItem( "mimeType" );
	propItems.AddItemDef( ( parsable* )mimeType );
	auto bufferView = new gltfItem_integer( "bufferView" );
	propItems.AddItemDef( ( parsable* )bufferView );
	auto name		= new gltfItem( "name" );
	propItems.AddItemDef( ( parsable* )name );
	auto extensions = new gltfItem( "extensions" );
	propItems.AddItemDef( ( parsable* )extensions );
	auto extras		= new gltfItem_Extra( "extras" );
	propItems.AddItemDef( ( parsable* ) extras );

	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfImage", 0 );

		gltfImage* image = currentAsset->Image();
		uri->Set( &image->uri, &image->bufferView, currentAsset );
		mimeType->Set( &image->mimeType );
		bufferView->Set( &image->bufferView );
		name->Set( &image->name );
		extensions->Set( &image->extensions );
		extras->Set( &image->extras, &lexer );
		propItems.Parse( &lexer );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}

		//automate..
		//image->bgfxTexture.handle.idx = UINT16_MAX;
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_ACCESSORS( idToken& token )
{
	gltfItemArray accessor;
	GLTFARRAYITEM( accessor, bufferView,	gltfItem_integer );
	GLTFARRAYITEM( accessor, byteOffset,	gltfItem_integer );
	GLTFARRAYITEM( accessor, componentType,	gltfItem_integer );
	GLTFARRAYITEM( accessor, normalized,	gltfItem_boolean );
	GLTFARRAYITEM( accessor, count,			gltfItem_integer );
	GLTFARRAYITEM( accessor, type,			gltfItem );
	GLTFARRAYITEM( accessor, max,			gltfItem_number_array );
	GLTFARRAYITEM( accessor, min,			gltfItem_number_array );
	GLTFARRAYITEM( accessor, sparse,		gltfItem_accessor_sparse );
	GLTFARRAYITEM( accessor, name,			gltfItem );
	GLTFARRAYITEM( accessor, extensions,	gltfItem );
	GLTFARRAYITEM( accessor, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfAccessor", 0 );

		gltfAccessor* item = currentAsset->Accessor();
		GLTFARRAYITEMREF( item, bufferView );
		GLTFARRAYITEMREF( item, byteOffset );
		GLTFARRAYITEMREF( item, componentType );
		GLTFARRAYITEMREF( item, normalized );
		GLTFARRAYITEMREF( item, count );
		GLTFARRAYITEMREF( item, type );
		max->Set( &item->max, &lexer );
		min->Set( &item->min, &lexer );
		sparse->Set( &item->sparse, &lexer );
		GLTFARRAYITEMREF( item, name );
		GLTFARRAYITEMREF( item, extensions );
		extras->Set( &item->extras, &lexer );
		accessor.Parse( &lexer );

		GetComponentTypeEnum( item->componentType, &item->typeSize );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_BUFFERVIEWS( idToken& token )
{
	gltfItemArray bv;
	GLTFARRAYITEM( bv, buffer,		gltfItem_integer );
	GLTFARRAYITEM( bv, byteLength,	gltfItem_integer );
	GLTFARRAYITEM( bv, byteStride,	gltfItem_integer );
	GLTFARRAYITEM( bv, byteOffset,	gltfItem_integer );
	GLTFARRAYITEM( bv, target,		gltfItem_integer );
	GLTFARRAYITEM( bv, name,		gltfItem );
	GLTFARRAYITEM( bv, extensions,	gltfItem );
	GLTFARRAYITEM( bv, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfBufferView", 0 );

		gltfBufferView* gltfBV = currentAsset->BufferView();
		GLTFARRAYITEMREF( gltfBV, buffer	);
		GLTFARRAYITEMREF( gltfBV, byteLength );
		GLTFARRAYITEMREF( gltfBV, byteStride );
		GLTFARRAYITEMREF( gltfBV, byteOffset );
		GLTFARRAYITEMREF( gltfBV, target	);
		GLTFARRAYITEMREF( gltfBV, name	);
		GLTFARRAYITEMREF( gltfBV, extensions );
		extras->Set( &gltfBV->extras, &lexer );
		bv.Parse( &lexer );
		gltfBV->parent = currentAsset;

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_SAMPLERS( idToken& token )
{
	gltfItemArray sampl;
	GLTFARRAYITEM( sampl, magFilter,	gltfItem_integer );
	GLTFARRAYITEM( sampl, minFilter,	gltfItem_integer );
	GLTFARRAYITEM( sampl, wrapS,		gltfItem_integer );
	GLTFARRAYITEM( sampl, wrapT,		gltfItem_integer );
	GLTFARRAYITEM( sampl, name,			gltfItem );
	GLTFARRAYITEM( sampl, extensions,	gltfItem );
	GLTFARRAYITEM( sampl, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfSampler", 0 );

		gltfSampler* gltfSampl = currentAsset->Sampler();
		GLTFARRAYITEMREF( gltfSampl, magFilter );
		GLTFARRAYITEMREF( gltfSampl, minFilter );
		GLTFARRAYITEMREF( gltfSampl, wrapS );
		GLTFARRAYITEMREF( gltfSampl, wrapT );
		GLTFARRAYITEMREF( gltfSampl, name	);
		GLTFARRAYITEMREF( gltfSampl, extensions );
		extras->Set( &gltfSampl->extras, &lexer );
		sampl.Parse( &lexer );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_BUFFERS( idToken& token )
{
	gltfItemArray buf;
	GLTFARRAYITEM( buf, uri,		gltfItem_uri );
	GLTFARRAYITEM( buf, byteLength, gltfItem_integer );
	GLTFARRAYITEM( buf, name,		gltfItem );
	GLTFARRAYITEM( buf, extensions, gltfItem );
	GLTFARRAYITEM( buf, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfBuffer", 0 );

		gltfBuffer* gltfBuf = currentAsset->Buffer();
		gltfBuf->parent = currentAsset;

		uri->Set( &gltfBuf->uri, nullptr , currentAsset );
		GLTFARRAYITEMREF( gltfBuf, byteLength );
		GLTFARRAYITEMREF( gltfBuf, name );
		GLTFARRAYITEMREF( gltfBuf, extensions );
		extras->Set( &gltfBuf->extras, &lexer );
		buf.Parse( &lexer );
		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_ANIMATIONS( idToken& token )
{
	gltfItemArray anim;
	GLTFARRAYITEM( anim, channels,		gltfItem_animation_channel ); //channel[1 - *]
	GLTFARRAYITEM( anim, samplers,		gltfItem_animation_sampler ); //sampler[1 - *]
	GLTFARRAYITEM( anim, name,			gltfItem );
	GLTFARRAYITEM( anim, extensions,	gltfItem );
	GLTFARRAYITEM( anim, extras,		gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfAnimation", 0 );

		gltfAnimation* gltfanim = currentAsset->Animation();

		channels->Set( &gltfanim->channels, &lexer );
		samplers->Set( &gltfanim->samplers, &lexer );
		GLTFARRAYITEMREF( gltfanim,  name );
		GLTFARRAYITEMREF( gltfanim,  extensions );
		extras->Set( &gltfanim->extras, &lexer );
		anim.Parse( &lexer );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_SKINS( idToken& token )
{
	gltfItemArray skin;
	GLTFARRAYITEM( skin, inverseBindMatrices,	gltfItem_integer );
	GLTFARRAYITEM( skin, skeleton,				gltfItem_integer );
	GLTFARRAYITEM( skin, joints,				gltfItem_integer_array );
	GLTFARRAYITEM( skin, name,					gltfItem );
	GLTFARRAYITEM( skin, extensions,			gltfItem );
	GLTFARRAYITEM( skin, extras,				gltfItem_Extra );

	gltfPropertyArray array = gltfPropertyArray( &parser );
	for( auto& prop : array )
	{
		idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
		lexer.LoadMemory( prop.item.c_str(), prop.item.Size(), "gltfSkin", 0 );

		gltfSkin* gltfSkin = currentAsset->Skin();

		GLTFARRAYITEMREF( gltfSkin, inverseBindMatrices );
		GLTFARRAYITEMREF( gltfSkin, skeleton );
		joints->Set( &gltfSkin->joints, &lexer	);
		GLTFARRAYITEMREF( gltfSkin, name );
		GLTFARRAYITEMREF( gltfSkin, extensions );
		extras->Set( &gltfSkin->extras, &lexer );
		skin.Parse( &lexer );

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_EXTENSIONS( idToken& token )
{
	idStr json;
	parser.ParseBracedSection( json );

	gltfItemArray extensions;
	//GLTFARRAYITEM( extensions, KHR_materials_pbrSpecularGlossiness, gltfItem_KHR_materials_pbrSpecularGlossiness );
	GLTFARRAYITEM( extensions, KHR_lights_punctual, gltfItem_KHR_lights_punctual );

	idLexer lexer( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
	lexer.LoadMemory( json.c_str(), json.Size(), "Extensions", 0 );
	gltfExtensions* gltfextension = currentAsset->Extensions();
	//KHR_materials_pbrSpecularGlossiness->Set( &gltfextensions, &lexer );
	KHR_lights_punctual->Set( gltfextension, &lexer );
	extensions.Parse( &lexer );

	if( gltf_parseVerbose.GetBool() )
	{
		common->Printf( "%s", json.c_str() );
	}
}

void GLTF_Parser::Parse_EXTENSIONS_USED( idToken& token )
{
	gltfPropertyArray array = gltfPropertyArray( &parser, false );
	for( auto& prop : array )
	{
		gltfExtensionsUsed* ext = currentAsset->ExtensionsUsed();
		ext->extension = prop.item;
		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "%s", prop.item.c_str() );
		}
	}
	parser.ExpectTokenString( "]" );
}

void GLTF_Parser::Parse_EXTENSIONS_REQUIRED( idToken& token )
{
	parser.ExpectTokenString( "[" );
	idStrList exts;
	idToken item;
	bool parsing = true;
	while( parsing && parser.ExpectAnyToken( &item ) )
	{
		if( item.type != TT_STRING )
		{
			common->FatalError( "malformed extensions_used array" );
		}
		idStr& extension = exts.Alloc();
		extension = item.c_str();
		parsing = parser.PeekTokenString( "," );
		if( parsing )
		{
			parser.ExpectTokenString( "," );
		}
	}
	parser.ExpectTokenString( "]" );
	for( auto& out : exts )
	{
		common->Printf( "%s", out.c_str() );
	}
}

gltfProperty GLTF_Parser::ParseProp( idToken& token )
{
	parser.ExpectTokenString( ":" );
	gltfProperty prop = ResolveProp( token );

	bool skipping = false;
	if( !buffersDone || !bufferViewsDone )
	{
		if( prop == BUFFERS && !buffersDone )
		{
			Parse_BUFFERS( token );
			return prop;
		}
		if( prop == BUFFERVIEWS && !bufferViewsDone )
		{
			Parse_BUFFERVIEWS( token );
			return prop;
		}
		skipping = true;
		if( gltf_parseVerbose.GetBool() )
		{
			common->DPrintf( "Searching for buffer tag. Skipping %s.", token.c_str() );
		}
	}
	else
	{
		if( ( prop == BUFFERS && buffersDone )  || ( prop == BUFFERVIEWS && bufferViewsDone ) )
		{
			skipping = true;
			if( gltf_parseVerbose.GetBool() )
			{
				common->DPrintf( "Skipping %s , already done.", token.c_str() );
			}
		}
	}

	if( skipping )
	{
		//1. search for {} scope.
		//2. search for [] scope.
		//3. single token.

		idToken skipTok;
		int sectionsSkipped = 0;
		if( parser.PeekTokenString( "{" ) )
		{
			parser.SkipBracedSection( true, BRSKIP_BRACES, &sectionsSkipped );
		}
		if( !sectionsSkipped && parser.PeekTokenString( "[" ) )
		{
			parser.SkipBracedSection( true, BRSKIP_BRACKET, &sectionsSkipped );
		}
		if( !sectionsSkipped )
		{
			parser.ExpectAnyToken( &skipTok );
		}

		return gltfProperty::INVALID;
	}

	switch( prop )
	{
		case ASSET:
			Parse_ASSET( token );
			break;
		case CAMERAS:
			Parse_CAMERAS( token );
			break;
		case SCENE:
			Parse_SCENE( token );
			break;
		case SCENES:
			Parse_SCENES( token );
			break;
		case NODES:
			Parse_NODES( token );
			break;
		case MATERIALS:
			Parse_MATERIALS( token );
			break;
		case MESHES:
			Parse_MESHES( token );
			break;
		case TEXTURES:
			Parse_TEXTURES( token );
			break;
		case IMAGES:
			Parse_IMAGES( token );
			break;
		case ACCESSORS:
			Parse_ACCESSORS( token );
			break;
		case BUFFERVIEWS:
			//Parse_BUFFERVIEWS( token );
			if( !bufferViewsDone )
			{
				common->FatalError( "Bufferviews should already be parsed!" );
			}
			break;
		case SAMPLERS:
			Parse_SAMPLERS( token );
			break;
		case BUFFERS:
			if( !buffersDone )
			{
				common->FatalError( "Buffers should already be parsed!" );
			}
			break;
		case ANIMATIONS:
			Parse_ANIMATIONS( token );
			break;
		case SKINS:
			Parse_SKINS( token );
			break;
		case EXTENSIONS:
			Parse_EXTENSIONS( token );
			break;
		case EXTENSIONS_USED:
			Parse_EXTENSIONS_USED( token );
			break;
		case EXTENSIONS_REQUIRED:
			Parse_EXTENSIONS_REQUIRED( token );
			break;
		default:
			common->FatalError( "UnImplemented GLTF property : %s", token.c_str() );
	}
	return prop;
}

gltfProperty GLTF_Parser::ResolveProp( idToken& token )
{
	if( !idStr::Icmp( token.c_str(), "asset"	) )
	{
		return gltfProperty::ASSET;
	}
	else if( !idStr::Icmp( token.c_str(), "cameras"	) )
	{
		return gltfProperty::CAMERAS;
	}
	else if( !idStr::Icmp( token.c_str(), "scene"	) )
	{
		return gltfProperty::SCENE;
	}
	else if( !idStr::Icmp( token.c_str(), "scenes"	) )
	{
		return gltfProperty::SCENES;
	}
	else if( !idStr::Icmp( token.c_str(), "nodes"	) )
	{
		return gltfProperty::NODES;
	}
	else if( !idStr::Icmp( token.c_str(), "materials"	) )
	{
		return gltfProperty::MATERIALS;
	}
	else if( !idStr::Icmp( token.c_str(), "meshes"	) )
	{
		return gltfProperty::MESHES;
	}
	else if( !idStr::Icmp( token.c_str(), "textures"	) )
	{
		return gltfProperty::TEXTURES;
	}
	else if( !idStr::Icmp( token.c_str(), "images"	) )
	{
		return gltfProperty::IMAGES;
	}
	else if( !idStr::Icmp( token.c_str(), "accessors"	) )
	{
		return gltfProperty::ACCESSORS;
	}
	else if( !idStr::Icmp( token.c_str(), "bufferViews"	) )
	{
		return gltfProperty::BUFFERVIEWS;
	}
	else if( !idStr::Icmp( token.c_str(), "samplers"	) )
	{
		return gltfProperty::SAMPLERS;
	}
	else if( !idStr::Icmp( token.c_str(), "buffers"	) )
	{
		return gltfProperty::BUFFERS;
	}
	else if( !idStr::Icmp( token.c_str(), "animations"	) )
	{
		return gltfProperty::ANIMATIONS;
	}
	else if( !idStr::Icmp( token.c_str(), "skins"	) )
	{
		return gltfProperty::SKINS;
	}
	else if( !idStr::Icmp( token.c_str(), "extensions"	) )
	{
		return gltfProperty::EXTENSIONS;
	}
	else if( !idStr::Icmp( token.c_str(), "extensionsused"	) )
	{
		return gltfProperty::EXTENSIONS_USED;
	}
	else if( !idStr::Icmp( token.c_str(), "extensionsrequired" ) )
	{
		return gltfProperty::EXTENSIONS_REQUIRED;
	}

	return gltfProperty::INVALID;
}

bool GLTF_Parser::loadGLB( idStr filename )
{
	if( fileSystem->GetFileLength( filename ) <= 0 )
	{
		common->Warning( " %s does not exist!", filename.c_str() );
		return false;
	}

	idFile* file = fileSystem->OpenFileRead( filename );

	if( file->Length() < 20 )
	{
		common->FatalError( "Too short data size for glTF Binary." );
		return false;
	}
	idStr gltfMagic( "glTF" );
	unsigned char fileMagic[5];

	file->Read( ( void* )&fileMagic, 4 );
	fileMagic[4] = 0;
	if( gltfMagic.Icmp( ( const char* )&fileMagic ) == 0 )
	{
		common->Printf( "reading %s...\n", filename.c_str() );
	}
	else
	{
		common->Error( "invalid magic" );
		return false;
	}

	unsigned int version = 0;	// 4 bytes
	unsigned int length = 0;	// 4 bytes

	//HVG_TODO
	//handle 0 bin chunk -> size is chunk[0].size  + 20;
	file->ReadUnsignedInt( version );
	file->ReadUnsignedInt( length );
	length -= 12; // header size

	unsigned int chunk_type = 0;	// 4 bytes
	unsigned int chunk_length = 0;	// 4 bytes
	byte* data = nullptr;
	gltfData* dataCache = gltfData::Data( filename, true );
	currentAsset = dataCache;

	int chunkCount = 0;
	while( length )
	{
		unsigned int prev_length = chunk_length;
		length -= file->ReadUnsignedInt( chunk_length );
		length -= file->ReadUnsignedInt( chunk_type );

		data = dataCache->AddData( chunk_length );

		int read = file->Read( ( void* )data, chunk_length );
		if( read != chunk_length )
		{
			common->FatalError( "Could not read full chunk (%i bytes) in file %s", chunk_length, filename.c_str() );
		}
		length -= read;
		if( chunk_type == gltfChunk_Type_JSON )
		{
			currentFile = filename ;
			parser.LoadMemory( ( const char* )data, chunk_length, "gltfJson", 0 );
		}
		else if( !chunkCount )
		{
			common->FatalError( "first chunk was not a json chunk" );
		}
		else
		{
			common->Printf( "BINCHUNK %i %i bytes\n", chunkCount, chunk_length );
		}
		if( chunkCount++ && length )
		{
			common->FatalError( "corrupt glb file." );
		}
	}

	Parse();
	delete file;
	return true;
}

bool GLTF_Parser::Parse()
{
	bool parsing = true;
	parser.ExpectTokenString( "{" );
	while( parsing && parser.ExpectAnyToken( &token ) )
	{
		if( token.type != TT_STRING )
		{
			common->FatalError( "Expected an \"string\" " );
		}

		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( token.c_str() );
		}
		gltfProperty prop = ParseProp( token );

		if( ( prop == BUFFERS && !buffersDone ) )
		{
			parser.Reset();
			parser.ExpectTokenString( "{" );
			buffersDone = true;
			continue;
		}
		if( ( prop == BUFFERVIEWS && !bufferViewsDone ) )
		{
			parser.Reset();
			parser.ExpectTokenString( "{" );
			bufferViewsDone = true;
			continue;
		}
		if( gltf_parseVerbose.GetBool() )
		{
			common->Printf( "\n" );
		}
		parsing = parser.PeekTokenString( "," );
		if( parsing )
		{
			parser.ExpectTokenString( "," );
		}
		else
		{
			//we are at the end, and no bufferview or buffer has been found.
			if( !buffersDone || !bufferViewsDone )
			{
				if( !buffersDone )
				{
					buffersDone = true;
					common->Printf( "no %s found", "buffers" );
				}
				if( !bufferViewsDone )
				{
					common->Printf( "no %s found", "bufferviews" );
					bufferViewsDone = true;
				}
				parser.Reset();
				parser.ExpectTokenString( "{" );
				parsing = true;
				continue;
			}


			parser.ExpectTokenString( "}" );
		}
	}

	//parser should be at end.
#if 0
	// RB: this results into a random "Unknown punctuation" error sometimes
	parser.ReadToken( &token );
	if( parser.EndOfFile() )
	{
		common->Printf( "%s ^2loaded\n", currentFile.c_str() );
	}
	else
	{
		common->FatalError( "%s not fully loaded.", currentFile.c_str() );
	}
#else
	common->Printf( "%s ^2loaded\n", currentFile.c_str() );
#endif

	buffersDone = false;
	bufferViewsDone = false;
	return true;
}

bool GLTF_Parser::Load( idStr filename )
{

	//.. and destroy data !!
	gltfData* data = gltfData::Data( filename );
	currentFile = filename;
	if( data != nullptr )
	{
		currentAsset = data;
		parser.FreeSource();
		return true;
	}

	common->SetRefreshOnPrint( true );
	if( filename.CheckExtension( ".glb" ) )
	{
		if( !loadGLB( filename ) )
		{
			return false;
		}
	}
	else if( filename.CheckExtension( ".gltf" ) )
	{
		int length = fileSystem->ReadFile( filename, NULL );
		if( !length )
		{
			common->FatalError( "Failed to read file" );
		}

		data = gltfData::Data( filename , true );
		byte* dataBuff = data->AddData( length );
		currentAsset = data;

		idFile* file = fileSystem->OpenFileRead( filename );
		if( file->Read( dataBuff, length ) != length )
		{
			common->FatalError( "Cannot read file, %s", filename.c_str() );
		}

		fileSystem->CloseFile( file );

		if( !parser.LoadMemory( ( const char* )dataBuff, length, "GLTF_ASCII_JSON", 0 ) )
		{
			return false;
		}

		Parse();

	}
	else
	{
		return false;
	}

	parser.Reset();
	parser.FreeSource();
	common->SetRefreshOnPrint( false );

	//fix up node hierarchy
	auto& nodeList = currentAsset->NodeList();
	for( auto& scene : currentAsset->SceneList() )
	{
		for( auto& node : scene->nodes )
		{
			SetNodeParent( nodeList[node] );
		}
	}

	//set skeleton ID's
	for( auto* skin : currentAsset->SkinList() )
	{
		if( skin->skeleton == -1 )
		{
			skin->skeleton = currentAsset->GetNodeIndex( currentAsset->GetNode( skin->name ) );
		}
	}
	//prefix with id
	if( gltfParser_PrefixNodeWithID.GetBool() )
	{
		for( int i = 0; i < nodeList.Num(); i++ )
		{
			nodeList[i]->name = "[" + idStr( i ) + "]" + nodeList[i]->name;
		}
	}
	//CreateBgfxData();
	return true;
}

void GLTF_Parser::SetNodeParent( gltfNode* node, gltfNode* parent )
{
	node->parent = parent;
	for( auto& child : node->children )
	{
		SetNodeParent( currentAsset->NodeList()[child], node );
	}
}

idList<float>& gltfData::GetAccessorView( gltfAccessor* accessor )
{
	idList<float>*& floatView = accessor->floatView;;

	if( floatView == nullptr )
	{
		gltfBufferView* attrBv = bufferViews[accessor->bufferView];
		gltfData* attrData = attrBv->parent;
		gltfBuffer* attrbuff = attrData->BufferList()[attrBv->buffer];
		assert( sizeof( float ) == accessor->typeSize );

		idFile_Memory bin = idFile_Memory( "GetAccessorView(float)",
										   ( const char* )( ( attrData->GetData( attrBv->buffer ) + attrBv->byteOffset + accessor->byteOffset ) ), attrBv->byteLength );

		floatView = new idList<float>( 16 );
		floatView->AssureSize( accessor->count );
		for( int i = 0; i < accessor->count; i++ )
		{
			bin.Read( ( void* ) & ( *floatView )[i] , accessor->typeSize );
		}
		if( attrBv->byteStride )
		{
			bin.Seek( attrBv->byteStride - ( accessor->typeSize ), FS_SEEK_CUR );
		}
	}
	return *floatView;
}

idList<idMat4>& gltfData::GetAccessorViewMat( gltfAccessor* accessor )
{
	idList<idMat4>*& matView = accessor->matView;
	if( matView == nullptr )
	{
		gltfBufferView* attrBv = bufferViews[accessor->bufferView];
		gltfData* attrData = attrBv->parent;
		gltfBuffer* attrbuff = attrData->BufferList()[attrBv->buffer];
		assert( sizeof( float ) == accessor->typeSize );

		idFile_Memory bin = idFile_Memory( "GetAccessorView(idMat4*)",
										   ( const char* )( ( attrData->GetData( attrBv->buffer ) + attrBv->byteOffset + accessor->byteOffset ) ), attrBv->byteLength );

		size_t elementSize = accessor->typeSize * 16;
		matView = new idList<idMat4>( 16 );
		matView->AssureSize( accessor->count );
		for( int i = 0; i < accessor->count; i++ )
		{
			bin.Read( ( void* ) & ( *matView )[i] , elementSize );
		}
		if( attrBv->byteStride )
		{
			bin.Seek( attrBv->byteStride - elementSize, FS_SEEK_CUR );
		}
	}
	return *matView;
}


template <>
idList<idVec3*>& gltfData::GetAccessorView( gltfAccessor* accessor )
{
	idList<idVec3*>*& vecView = accessor->vecView;

	if( vecView == nullptr )
	{
		gltfBufferView* attrBv = bufferViews[accessor->bufferView];
		gltfData* attrData = attrBv->parent;
		gltfBuffer* attrbuff = attrData->BufferList()[attrBv->buffer];
		assert( sizeof( float ) == accessor->typeSize );

		idFile_Memory bin = idFile_Memory( "GetAccessorView(idVec3*)",
										   ( const char* )( ( attrData->GetData( attrBv->buffer ) + attrBv->byteOffset + accessor->byteOffset ) ), attrBv->byteLength );

		vecView = new idList<idVec3*>( 16 );
		vecView->AssureSizeAlloc( accessor->count, idListNewElement<idVec3> );
		for( int i = 0; i < accessor->count; i++ )
		{
			idVec3& vec = *( *vecView )[i];
			bin.Read( ( void* ) &vec.x , accessor->typeSize );
			bin.Read( ( void* ) &vec.y , accessor->typeSize );
			bin.Read( ( void* ) &vec.z , accessor->typeSize );
		}
		if( attrBv->byteStride )
		{
			bin.Seek( attrBv->byteStride - ( 3 * accessor->typeSize ), FS_SEEK_CUR );
		}
	}
	return *vecView;
}

template <>
idList<idQuat*>& gltfData::GetAccessorView( gltfAccessor* accessor )
{
	idList<idQuat*>*& quatView = accessor->quatView;

	if( quatView == nullptr )
	{
		gltfBufferView* attrBv = bufferViews[accessor->bufferView];
		gltfData* attrData = attrBv->parent;
		gltfBuffer* attrbuff = attrData->BufferList()[attrBv->buffer];
		assert( sizeof( float ) == accessor->typeSize );

		idFile_Memory bin = idFile_Memory( "GetAccessorView(idQuat*)",
										   ( const char* )( ( attrData->GetData( attrBv->buffer ) + attrBv->byteOffset + accessor->byteOffset ) ), attrBv->byteLength );

		quatView = new idList<idQuat*>( 16 );
		quatView->AssureSizeAlloc( accessor->count, idListNewElement<idQuat> );
		for( int i = 0; i < accessor->count; i++ )
		{
			idQuat& vec = *( *quatView )[i];
			bin.Read( ( void* ) &vec.x, accessor->typeSize );
			bin.Read( ( void* ) &vec.y, accessor->typeSize );
			bin.Read( ( void* ) &vec.z, accessor->typeSize );
			bin.Read( ( void* ) &vec.w, accessor->typeSize );
		}
		if( attrBv->byteStride )
		{
			bin.Seek( attrBv->byteStride - ( 4 * accessor->typeSize ), FS_SEEK_CUR );
		}
	}
	return *quatView;
}


gltfData::~gltfData()
{

	if( data )
	{
		while( totalChunks )
		{
			Mem_Free( data[--totalChunks] );
		}
		Mem_Free( data );
	}

	if( json )
	{
		Mem_Free( json );
	}

	data = nullptr;
	json = nullptr;
	ClearData( fileName );

}

void gltfData::ClearData( idStr& fileName )
{
	int key = fileDataHash.GenerateKey( fileName );
	int index = fileDataHash.GetFirst( key );

	if( index != -1 )
	{
		dataList.RemoveIndex( index );
		dataList.Condense();
		fileDataHash.RemoveIndex( key, index );
	}
	else
	{
		common->DWarning( " tried to clear GLTF data while no data was loaded for %s", fileName.c_str() );
	}

}


#undef GLTFARRAYITEM
#undef GLTFARRAYITEMREF

CONSOLE_COMMAND_COMPILE( LoadGLTF, "Loads an .gltf or .glb file", idCmdSystem::ArgCompletion_MapName )
{

	if( args.Argc() > 1 )
	{
		GLTF_Parser gltf;
		gltf.Load( args.Argv( 1 ) );
	}

}
//+set r_fullscreen 0 +set com_allowConsole 1 +set developer 1 +set fs_debug 0 +set win_outputDebugString 1 +set fs_basepath "E:\SteamLibrary\steamapps\common\DOOM 3 BFG Edition\"


//  not dots allowed in [%s]!
// [filename].[%i|%s].[gltf|glb]
bool gltfManager::ExtractIdentifier( idStr& filename, int& id, idStr& name )
{
	idStr extension;
	idStr targetStr;
	filename.ExtractFileExtension( extension );
	idStr file = filename;
	file = file.StripFileExtension();
	file.ExtractFileExtension( targetStr );

	if( !extension.Length() )
	{
		idLib::Warning( "no gltf identifier" );
		return false;
	}

	if( targetStr.Length() )
	{
		filename = file.Left( file.Length() - targetStr.Length() - 1 ) + "." + extension;

		idParser	parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS );
		parser.LoadMemory( targetStr.c_str(), targetStr.Size(), "model:GltfID" );

		idToken token;
		if( parser.ExpectAnyToken( &token ) )
		{
			if( ( token.type == TT_NUMBER ) && ( token.subtype & TT_INTEGER ) )
			{
				id = token.GetIntValue();
			}
			else if( token.type == TT_NAME )
			{
				name = token;
			}
			else
			{
				parser.Warning( "malformed gltf mesh identifier" );
				return false;
			}
			return true;
		}
		else
		{
			parser.Warning( "malformed gltf mesh identifier" );
		}
	}

	return false;
}