/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"
#include "RenderProgs_embedded.h"

idCVar r_skipStripDeadCode( "r_skipStripDeadCode", "0", CVAR_BOOL, "Skip stripping dead code" );
idCVar r_useUniformArrays( "r_useUniformArrays", "1", CVAR_BOOL, "" );

// DG: the AMD drivers output a lot of useless warnings which are fscking annoying, added this CVar to suppress them
idCVar r_displayGLSLCompilerMessages( "r_displayGLSLCompilerMessages", "1", CVAR_BOOL | CVAR_ARCHIVE, "Show info messages the GPU driver outputs when compiling the shaders" );
// DG end

// RB begin
idCVar r_alwaysExportGLSL( "r_alwaysExportGLSL", "1", CVAR_BOOL, "" );
// RB end

#define VERTEX_UNIFORM_ARRAY_NAME				"_va_"
#define FRAGMENT_UNIFORM_ARRAY_NAME				"_fa_"

static const int AT_VS_IN  = BIT( 1 );
static const int AT_VS_OUT = BIT( 2 );
static const int AT_PS_IN  = BIT( 3 );
static const int AT_PS_OUT = BIT( 4 );
// RB begin
static const int AT_VS_OUT_RESERVED = BIT( 5 );
static const int AT_PS_IN_RESERVED	= BIT( 6 );
static const int AT_PS_OUT_RESERVED = BIT( 7 );
// RB end

struct idCGBlock
{
	idStr prefix;	// tokens that comes before the name
	idStr name;		// the name
	idStr postfix;	// tokens that comes after the name
	bool used;		// whether or not this block is referenced anywhere
};

/*
================================================
attribInfo_t
================================================
*/
struct attribInfo_t
{
	const char* 	type;
	const char* 	name;
	const char* 	semantic;
	const char* 	glsl;
	int				bind;
	int				flags;
	int				vertexMask;
};

/*
================================================
vertexMask_t

NOTE: There is a PS3 dependency between the bit flag specified here and the vertex
attribute index and attribute semantic specified in DeclRenderProg.cpp because the
stored render prog vertexMask is initialized with cellCgbGetVertexConfiguration().
The ATTRIB_INDEX_ defines are used to make sure the vertexMask_t and attrib assignment
in DeclRenderProg.cpp are in sync.

Even though VERTEX_MASK_XYZ_SHORT and VERTEX_MASK_ST_SHORT are not real attributes,
they come before the VERTEX_MASK_MORPH to reduce the range of vertex program
permutations defined by the vertexMask_t bits on the Xbox 360 (see MAX_VERTEX_DECLARATIONS).
================================================
*/
enum vertexMask_t
{
	VERTEX_MASK_XYZ			= BIT( PC_ATTRIB_INDEX_VERTEX ),
	VERTEX_MASK_ST			= BIT( PC_ATTRIB_INDEX_ST ),
	VERTEX_MASK_NORMAL		= BIT( PC_ATTRIB_INDEX_NORMAL ),
	VERTEX_MASK_COLOR		= BIT( PC_ATTRIB_INDEX_COLOR ),
	VERTEX_MASK_TANGENT		= BIT( PC_ATTRIB_INDEX_TANGENT ),
	VERTEX_MASK_COLOR2		= BIT( PC_ATTRIB_INDEX_COLOR2 ),
};

attribInfo_t attribsPC[] =
{
	// vertex attributes
	{ "float4",		"position",		"POSITION",		"in_Position",			PC_ATTRIB_INDEX_VERTEX,			AT_VS_IN,		VERTEX_MASK_XYZ },
	{ "float2",		"texcoord",		"TEXCOORD0",	"in_TexCoord",			PC_ATTRIB_INDEX_ST,				AT_VS_IN,		VERTEX_MASK_ST },
	{ "float4",		"normal",		"NORMAL",		"in_Normal",			PC_ATTRIB_INDEX_NORMAL,			AT_VS_IN,		VERTEX_MASK_NORMAL },
	{ "float4",		"tangent",		"TANGENT",		"in_Tangent",			PC_ATTRIB_INDEX_TANGENT,		AT_VS_IN,		VERTEX_MASK_TANGENT },
	{ "float4",		"color",		"COLOR0",		"in_Color",				PC_ATTRIB_INDEX_COLOR,			AT_VS_IN,		VERTEX_MASK_COLOR },
	{ "float4",		"color2",		"COLOR1",		"in_Color2",			PC_ATTRIB_INDEX_COLOR2,			AT_VS_IN,		VERTEX_MASK_COLOR2 },
	
	// pre-defined vertex program output
	{ "float4",		"position",		"POSITION",		"gl_Position",			0,	AT_VS_OUT | AT_VS_OUT_RESERVED,		0 },
	{ "float",		"clip0",		"CLP0",			"gl_ClipDistance[0]",	0,	AT_VS_OUT,		0 },
	{ "float",		"clip1",		"CLP1",			"gl_ClipDistance[1]",	0,	AT_VS_OUT,		0 },
	{ "float",		"clip2",		"CLP2",			"gl_ClipDistance[2]",	0,	AT_VS_OUT,		0 },
	{ "float",		"clip3",		"CLP3",			"gl_ClipDistance[3]",	0,	AT_VS_OUT,		0 },
	{ "float",		"clip4",		"CLP4",			"gl_ClipDistance[4]",	0,	AT_VS_OUT,		0 },
	{ "float",		"clip5",		"CLP5",			"gl_ClipDistance[5]",	0,	AT_VS_OUT,		0 },
	
	// pre-defined fragment program input
	{ "float4",		"position",		"WPOS",			"gl_FragCoord",			0,	AT_PS_IN | AT_PS_IN_RESERVED,		0 },
	{ "half4",		"hposition",	"WPOS",			"gl_FragCoord",			0,	AT_PS_IN | AT_PS_IN_RESERVED,		0 },
	{ "float",		"facing",		"FACE",			"gl_FrontFacing",		0,	AT_PS_IN | AT_PS_IN_RESERVED,		0 },
	
	// fragment program output
	{ "float4",		"color",		"COLOR",		"fo_FragColor",		0,	AT_PS_OUT /*| AT_PS_OUT_RESERVED*/,		0 }, // GLSL version 1.2 doesn't allow for custom color name mappings
	{ "half4",		"hcolor",		"COLOR",		"fo_FragColor",		0,	AT_PS_OUT /*| AT_PS_OUT_RESERVED*/,		0 },
	{ "float4",		"color0",		"COLOR0",		"fo_FragColor",		0,	AT_PS_OUT /*| AT_PS_OUT_RESERVED*/,		0 },
	{ "float4",		"color1",		"COLOR1",		"fo_FragColor",		1,	AT_PS_OUT /*| AT_PS_OUT_RESERVED*/,		0 },
	{ "float4",		"color2",		"COLOR2",		"fo_FragColor",		2,	AT_PS_OUT /*| AT_PS_OUT_RESERVED*/,		0 },
	{ "float4",		"color3",		"COLOR3",		"fo_FragColor",		3,	AT_PS_OUT /*| AT_PS_OUT_RESERVED*/,		0 },
	{ "float",		"depth",		"DEPTH",		"gl_FragDepth",		4,	AT_PS_OUT | AT_PS_OUT_RESERVED,		0 },
	
	// vertex to fragment program pass through
#if 0 //defined(USE_GLES2)
	{ "float4",		"color",		"COLOR",		"vofi_FrontColor",			0,	AT_VS_OUT | AT_VS_OUT_RESERVED,	0 },
	{ "float4",		"color0",		"COLOR0",		"vofi_Color",				0,	AT_VS_OUT | AT_VS_OUT_RESERVED,	0 },
	{ "float4",		"color1",		"COLOR1",		"vofi_FrontSecondaryColor",	0,	AT_VS_OUT | AT_VS_OUT_RESERVED,	0 },
#else
	{ "float4",		"color",		"COLOR",		"vofi_Color",				0,	AT_VS_OUT,	0 },
	{ "float4",		"color0",		"COLOR0",		"vofi_Color",				0,	AT_VS_OUT,	0 },
	{ "float4",		"color1",		"COLOR1",		"vofi_SecondaryColor",		0,	AT_VS_OUT,	0 },
#endif
	
	
	{ "float4",		"color",		"COLOR",		"vofi_Color",				0,	AT_PS_IN,	0 },
	{ "float4",		"color0",		"COLOR0",		"vofi_Color",				0,	AT_PS_IN,	0 },
	{ "float4",		"color1",		"COLOR1",		"vofi_SecondaryColor",		0,	AT_PS_IN,	0 },
	
	{ "half4",		"hcolor",		"COLOR",		"vofi_Color",				0,	AT_PS_IN,		0 },
	{ "half4",		"hcolor0",		"COLOR0",		"vofi_Color",				0,	AT_PS_IN,		0 },
	{ "half4",		"hcolor1",		"COLOR1",		"vofi_SecondaryColor",		0,	AT_PS_IN,		0 },
	
	{ "float4",		"texcoord0",	"TEXCOORD0_centroid",	"vofi_TexCoord0",	0,	AT_PS_IN,	0 },
	{ "float4",		"texcoord1",	"TEXCOORD1_centroid",	"vofi_TexCoord1",	0,	AT_PS_IN,	0 },
	{ "float4",		"texcoord2",	"TEXCOORD2_centroid",	"vofi_TexCoord2",	0,	AT_PS_IN,	0 },
	{ "float4",		"texcoord3",	"TEXCOORD3_centroid",	"vofi_TexCoord3",	0,	AT_PS_IN,	0 },
	{ "float4",		"texcoord4",	"TEXCOORD4_centroid",	"vofi_TexCoord4",	0,	AT_PS_IN,	0 },
	{ "float4",		"texcoord5",	"TEXCOORD5_centroid",	"vofi_TexCoord5",	0,	AT_PS_IN,	0 },
	{ "float4",		"texcoord6",	"TEXCOORD6_centroid",	"vofi_TexCoord6",	0,	AT_PS_IN,	0 },
	{ "float4",		"texcoord7",	"TEXCOORD7_centroid",	"vofi_TexCoord7",	0,	AT_PS_IN,	0 },
	{ "float4",		"texcoord8",	"TEXCOORD8_centroid",	"vofi_TexCoord8",	0,	AT_PS_IN,	0 },
	{ "float4",		"texcoord9",	"TEXCOORD9_centroid",	"vofi_TexCoord9",	0,	AT_PS_IN,	0 },
	
	{ "float4",		"texcoord0",	"TEXCOORD0",	"vofi_TexCoord0",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord1",	"TEXCOORD1",	"vofi_TexCoord1",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord2",	"TEXCOORD2",	"vofi_TexCoord2",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord3",	"TEXCOORD3",	"vofi_TexCoord3",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord4",	"TEXCOORD4",	"vofi_TexCoord4",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord5",	"TEXCOORD5",	"vofi_TexCoord5",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord6",	"TEXCOORD6",	"vofi_TexCoord6",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord7",	"TEXCOORD7",	"vofi_TexCoord7",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord8",	"TEXCOORD8",	"vofi_TexCoord8",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord9",	"TEXCOORD9",	"vofi_TexCoord9",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	
	{ "half4",		"htexcoord0",	"TEXCOORD0",	"vofi_TexCoord0",		0,	AT_PS_IN,		0 },
	{ "half4",		"htexcoord1",	"TEXCOORD1",	"vofi_TexCoord1",		0,	AT_PS_IN,		0 },
	{ "half4",		"htexcoord2",	"TEXCOORD2",	"vofi_TexCoord2",		0,	AT_PS_IN,		0 },
	{ "half4",		"htexcoord3",	"TEXCOORD3",	"vofi_TexCoord3",		0,	AT_PS_IN,		0 },
	{ "half4",		"htexcoord4",	"TEXCOORD4",	"vofi_TexCoord4",		0,	AT_PS_IN,		0 },
	{ "half4",		"htexcoord5",	"TEXCOORD5",	"vofi_TexCoord5",		0,	AT_PS_IN,		0 },
	{ "half4",		"htexcoord6",	"TEXCOORD6",	"vofi_TexCoord6",		0,	AT_PS_IN,		0 },
	{ "half4",		"htexcoord7",	"TEXCOORD7",	"vofi_TexCoord7",		0,	AT_PS_IN,		0 },
	{ "half4",		"htexcoord8",	"TEXCOORD8",	"vofi_TexCoord8",		0,	AT_PS_IN,		0 },
	{ "half4",		"htexcoord9",	"TEXCOORD9",	"vofi_TexCoord9",		0,	AT_PS_IN,		0 },
	{ "float",		"fog",			"FOG",			"gl_FogFragCoord",		0,	AT_VS_OUT,		0 },
	{ "float4",		"fog",			"FOG",			"gl_FogFragCoord",		0,	AT_PS_IN,		0 },
	{ NULL,			NULL,			NULL,			NULL,					0,	0,				0 }
};

const char* types[] =
{
	"int",
	"float",
	"half",
	"fixed",
	"bool",
	"cint",
	"cfloat",
	"void"
};
static const int numTypes = sizeof( types ) / sizeof( types[0] );

const char* typePosts[] =
{
	"1", "2", "3", "4",
	"1x1", "1x2", "1x3", "1x4",
	"2x1", "2x2", "2x3", "2x4",
	"3x1", "3x2", "3x3", "3x4",
	"4x1", "4x2", "4x3", "4x4"
};
static const int numTypePosts = sizeof( typePosts ) / sizeof( typePosts[0] );

const char* prefixes[] =
{
	"static",
	"const",
	"uniform",
	"struct",
	
	"sampler",
	
	"sampler1D",
	"sampler2D",
	"sampler3D",
	"samplerCUBE",
	
	"sampler1DShadow",		// GLSL
	"sampler2DShadow",		// GLSL
	"sampler2DArrayShadow",	// GLSL
	"sampler3DShadow",		// GLSL
	"samplerCubeShadow",	// GLSL
	
	"sampler2DArray",		// GLSL"
	
	"sampler2DMS",			// GLSL
};
static const int numPrefixes = sizeof( prefixes ) / sizeof( prefixes[0] );

// For GLSL we need to have the names for the renderparms so we can look up their run time indices within the renderprograms
static const char* GLSLParmNames[RENDERPARM_TOTAL] =
{
	"rpScreenCorrectionFactor",
	"rpWindowCoord",
	"rpDiffuseModifier",
	"rpSpecularModifier",
	
	"rpLocalLightOrigin",
	"rpLocalViewOrigin",
	
	"rpLightProjectionS",
	"rpLightProjectionT",
	"rpLightProjectionQ",
	"rpLightFalloffS",
	
	"rpBumpMatrixS",
	"rpBumpMatrixT",
	
	"rpDiffuseMatrixS",
	"rpDiffuseMatrixT",
	
	"rpSpecularMatrixS",
	"rpSpecularMatrixT",
	
	"rpVertexColorModulate",
	"rpVertexColorAdd",
	
	"rpColor",
	"rpViewOrigin",
	"rpGlobalEyePos",
	
	"rpMVPmatrixX",
	"rpMVPmatrixY",
	"rpMVPmatrixZ",
	"rpMVPmatrixW",
	
	"rpModelMatrixX",
	"rpModelMatrixY",
	"rpModelMatrixZ",
	"rpModelMatrixW",
	
	"rpProjectionMatrixX",
	"rpProjectionMatrixY",
	"rpProjectionMatrixZ",
	"rpProjectionMatrixW",
	
	"rpModelViewMatrixX",
	"rpModelViewMatrixY",
	"rpModelViewMatrixZ",
	"rpModelViewMatrixW",
	
	"rpTextureMatrixS",
	"rpTextureMatrixT",
	
	"rpTexGen0S",
	"rpTexGen0T",
	"rpTexGen0Q",
	"rpTexGen0Enabled",
	
	"rpTexGen1S",
	"rpTexGen1T",
	"rpTexGen1Q",
	"rpTexGen1Enabled",
	
	"rpWobbleSkyX",
	"rpWobbleSkyY",
	"rpWobbleSkyZ",
	
	"rpOverbright",
	"rpEnableSkinning",
	"rpAlphaTest",
	
	// RB begin
	"rpAmbientColor",
	
	"rpGlobalLightOrigin",
	"rpJitterTexScale",
	"rpJitterTexOffset",
	"rpCascadeDistances",
	
	"rpShadowMatrices",
	"rpShadowMatrix0Y",
	"rpShadowMatrix0Z",
	"rpShadowMatrix0W",
	
	"rpShadowMatrix1X",
	"rpShadowMatrix1Y",
	"rpShadowMatrix1Z",
	"rpShadowMatrix1W",
	
	"rpShadowMatrix2X",
	"rpShadowMatrix2Y",
	"rpShadowMatrix2Z",
	"rpShadowMatrix2W",
	
	"rpShadowMatrix3X",
	"rpShadowMatrix3Y",
	"rpShadowMatrix3Z",
	"rpShadowMatrix3W",
	
	"rpShadowMatrix4X",
	"rpShadowMatrix4Y",
	"rpShadowMatrix4Z",
	"rpShadowMatrix4W",
	
	"rpShadowMatrix5X",
	"rpShadowMatrix5Y",
	"rpShadowMatrix5Z",
	"rpShadowMatrix5W",
	// RB end
};

// RB begin
const char* idRenderProgManager::GLSLMacroNames[MAX_SHADER_MACRO_NAMES] =
{
	"USE_GPU_SKINNING",
	"LIGHT_POINT",
	"LIGHT_PARALLEL",
};
// RB end


// RB: added embedded Cg shader resources
static const char* FindEmbeddedSourceShader( const char* name )
{
	const char* embeddedSource = NULL;
	for( int i = 0 ; cg_renderprogs[i].name ; i++ )
	{
		if( !idStr::Icmp( cg_renderprogs[i].name, name ) )
		{
			embeddedSource = cg_renderprogs[i].shaderText;
			break;
		}
	}
	
	return embeddedSource;
}

class idParser_EmbeddedGLSL : public idParser
{
public:
	idParser_EmbeddedGLSL( int flags ) : idParser( flags )
	{
	}
	
private:
	int		Directive_include( idToken* token, bool supressWarning )
	{
		if( idParser::Directive_include( token, true ) )
		{
			// RB: try local shaders in base/renderprogs/ first
			return true;
		}
		
		idLexer* script;
		
		idStr path;
		
		/*
		token was already parsed
		if( !idParser::ReadSourceToken( &token ) )
		{
			idParser::Error( "#include without file name" );
			return false;
		}
		*/
		
		if( token->linesCrossed > 0 )
		{
			idParser::Error( "#include without file name" );
			return false;
		}
		
		if( token->type == TT_STRING )
		{
			script = new idLexer;
			
			// try relative to the current file
			path = scriptstack->GetFileName();
			path.StripFilename();
			path += "/";
			path += *token;
			
			//if( !script->LoadFile( path, OSPath ) )
			const char* embeddedSource = FindEmbeddedSourceShader( path );
			if( embeddedSource == NULL )
			{
				// try absolute path
				path = *token;
				embeddedSource = FindEmbeddedSourceShader( path );
				if( embeddedSource == NULL )
				{
					// try from the include path
					path = includepath + *token;
					embeddedSource = FindEmbeddedSourceShader( path );
				}
			}
			
			if( embeddedSource == NULL || !script->LoadMemory( embeddedSource, strlen( embeddedSource ), path ) )
			{
				delete script;
				script = NULL;
			}
		}
		else if( token->type == TT_PUNCTUATION && *token == "<" )
		{
			path = idParser::includepath;
			while( idParser::ReadSourceToken( token ) )
			{
				if( token->linesCrossed > 0 )
				{
					idParser::UnreadSourceToken( token );
					break;
				}
				if( token->type == TT_PUNCTUATION && *token == ">" )
				{
					break;
				}
				path += *token;
			}
			if( *token != ">" )
			{
				idParser::Warning( "#include missing trailing >" );
			}
			if( !path.Length() )
			{
				idParser::Error( "#include without file name between < >" );
				return false;
			}
			if( idParser::flags & LEXFL_NOBASEINCLUDES )
			{
				return true;
			}
			script = new idLexer;
			
			const char* embeddedSource = FindEmbeddedSourceShader( includepath + path );
			
			if( embeddedSource == NULL || !script->LoadMemory( embeddedSource, strlen( embeddedSource ), path ) )
			{
				delete script;
				script = NULL;
			}
		}
		else
		{
			idParser::Error( "#include without file name" );
			return false;
		}
		
		if( !script )
		{
			idParser::Error( "file '%s' not found", path.c_str() );
			return false;
		}
		script->SetFlags( idParser::flags );
		script->SetPunctuations( idParser::punctuations );
		idParser::PushScript( script );
		return true;
	}
};
// RB end

/*
========================
StripDeadCode
========================
*/
idStr StripDeadCode( const idStr& in, const char* name, const idStrList& compileMacros, bool builtin )
{
	if( r_skipStripDeadCode.GetBool() )
	{
		return in;
	}
	
	//idLexer src( LEXFL_NOFATALERRORS );
	idParser_EmbeddedGLSL src( LEXFL_NOFATALERRORS );
	src.LoadMemory( in.c_str(), in.Length(), name );
	src.AddDefine( "PC" );
	
	for( int i = 0; i < compileMacros.Num(); i++ )
	{
		src.AddDefine( compileMacros[i] );
	}
	
	switch( glConfig.driverType )
	{
		case GLDRV_OPENGL_ES2:
		case GLDRV_OPENGL_ES3:
			//src.AddDefine( "GLES2" );
			break;
	}
	
	if( !builtin && glConfig.gpuSkinningAvailable )
	{
		src.AddDefine( "USE_GPU_SKINNING" );
	}
	
	if( r_useUniformArrays.GetBool() )
	{
		src.AddDefine( "USE_UNIFORM_ARRAYS" );
	}
	
	if( r_useHalfLambertLighting.GetBool() )
	{
		src.AddDefine( "USE_HALF_LAMBERT" );
	}
	
	idList< idCGBlock > blocks;
	
	blocks.SetNum( 100 );
	
	idToken token;
	while( !src.EndOfFile() )
	{
		idCGBlock& block = blocks.Alloc();
		// read prefix
		while( src.ReadToken( &token ) )
		{
			bool found = false;
			for( int i = 0; i < numPrefixes; i++ )
			{
				if( token == prefixes[i] )
				{
					found = true;
					break;
				}
			}
			if( !found )
			{
				for( int i = 0; i < numTypes; i++ )
				{
					if( token == types[i] )
					{
						found = true;
						break;
					}
					int typeLen = idStr::Length( types[i] );
					if( token.Cmpn( types[i], typeLen ) == 0 )
					{
						for( int j = 0; j < numTypePosts; j++ )
						{
							if( idStr::Cmp( token.c_str() + typeLen, typePosts[j] ) == 0 )
							{
								found = true;
								break;
							}
						}
						if( found )
						{
							break;
						}
					}
				}
			}
			if( found )
			{
				if( block.prefix.Length() > 0 && token.WhiteSpaceBeforeToken() )
				{
					block.prefix += ' ';
				}
				block.prefix += token;
			}
			else
			{
				src.UnreadToken( &token );
				break;
			}
		}
		if( !src.ReadToken( &token ) )
		{
			blocks.SetNum( blocks.Num() - 1 );
			break;
		}
		block.name = token;
		
		if( src.PeekTokenString( "=" ) || src.PeekTokenString( ":" ) || src.PeekTokenString( "[" ) )
		{
			src.ReadToken( &token );
			block.postfix = token;
			while( src.ReadToken( &token ) )
			{
				if( token == ";" )
				{
					block.postfix += ';';
					break;
				}
				else
				{
					if( token.WhiteSpaceBeforeToken() )
					{
						block.postfix += ' ';
					}
					block.postfix += token;
				}
			}
		}
		else if( src.PeekTokenString( "(" ) )
		{
			idStr parms, body;
			src.ParseBracedSection( parms, -1, true, '(', ')' );
			if( src.CheckTokenString( ";" ) )
			{
				block.postfix = parms + ";";
			}
			else
			{
				src.ParseBracedSection( body, -1, true, '{', '}' );
				block.postfix = parms + " " + body;
			}
		}
		else if( src.PeekTokenString( "{" ) )
		{
			src.ParseBracedSection( block.postfix, -1, true, '{', '}' );
			if( src.CheckTokenString( ";" ) )
			{
				block.postfix += ';';
			}
		}
		else if( src.CheckTokenString( ";" ) )
		{
			block.postfix = idStr( ';' );
		}
		else
		{
			src.Warning( "Could not strip dead code -- unknown token %s\n", token.c_str() );
			return in;
		}
	}
	
	idList<int, TAG_RENDERPROG> stack;
	for( int i = 0; i < blocks.Num(); i++ )
	{
		blocks[i].used = ( ( blocks[i].name == "main" )
						   || blocks[i].name.Right( 4 ) == "_ubo"
						 );
						 
		if( blocks[i].name == "include" )
		{
			blocks[i].used = true;
			blocks[i].name = ""; // clear out the include tag
		}
		
		if( blocks[i].used )
		{
			stack.Append( i );
		}
	}
	
	while( stack.Num() > 0 )
	{
		int i = stack[stack.Num() - 1];
		stack.SetNum( stack.Num() - 1 );
		
		idLexer src( LEXFL_NOFATALERRORS );
		src.LoadMemory( blocks[i].postfix.c_str(), blocks[i].postfix.Length(), name );
		while( src.ReadToken( &token ) )
		{
			for( int j = 0; j < blocks.Num(); j++ )
			{
				if( !blocks[j].used )
				{
					if( token == blocks[j].name )
					{
						blocks[j].used = true;
						stack.Append( j );
					}
				}
			}
		}
	}
	
	idStr out;
	
	for( int i = 0; i < blocks.Num(); i++ )
	{
		if( blocks[i].used )
		{
			out += blocks[i].prefix;
			out += ' ';
			out += blocks[i].name;
			out += ' ';
			out += blocks[i].postfix;
			out += '\n';
		}
	}
	
	return out;
}

struct typeConversion_t
{
	const char* typeCG;
	const char* typeGLSL;
} typeConversion[] =
{
	{ "void",				"void" },
	
	{ "fixed",				"float" },
	
	{ "float",				"float" },
	{ "float2",				"vec2" },
	{ "float3",				"vec3" },
	{ "float4",				"vec4" },
	
	{ "half",				"float" },
	{ "half2",				"vec2" },
	{ "half3",				"vec3" },
	{ "half4",				"vec4" },
	
	{ "int",				"int" },
	{ "int2",				"ivec2" },
	{ "int3",				"ivec3" },
	{ "int4",				"ivec4" },
	
	{ "bool",				"bool" },
	{ "bool2",				"bvec2" },
	{ "bool3",				"bvec3" },
	{ "bool4",				"bvec4" },
	
	{ "float2x2",			"mat2x2" },
	{ "float2x3",			"mat2x3" },
	{ "float2x4",			"mat2x4" },
	
	{ "float3x2",			"mat3x2" },
	{ "float3x3",			"mat3x3" },
	{ "float3x4",			"mat3x4" },
	
	{ "float4x2",			"mat4x2" },
	{ "float4x3",			"mat4x3" },
	{ "float4x4",			"mat4x4" },
	
	{ "sampler1D",			"sampler1D" },
	{ "sampler2D",			"sampler2D" },
	{ "sampler3D",			"sampler3D" },
	{ "samplerCUBE",		"samplerCube" },
	
	{ "sampler1DShadow",	"sampler1DShadow" },
	{ "sampler2DShadow",	"sampler2DShadow" },
	{ "sampler3DShadow",	"sampler3DShadow" },
	{ "samplerCubeShadow",	"samplerCubeShadow" },
	
	{ "sampler2DMS",		"sampler2DMS" },
	
	{ NULL, NULL }
};

// RB begin
#if defined(USE_GLES2)
const char* vertexInsert_GLSL_ES_1_0 =
{
	"#version 100\n"
	"#define GLES2\n"
	"#define PC\n"
	"precision mediump float;\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	//"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	"\n"
};
#else
const char* vertexInsert_GLSL_ES_1_0 =
{
	"#version 100\n"
	
#if !defined(USE_MESA)
	"#define GLES2\n"
#endif
	
	"#define PC\n"
	
#if 1 //defined(__ANDROID__)
	"precision mediump float;\n"
#else
	"precision highp float;\n"
#endif
	
#if defined(USE_GPU_SKINNING) && !defined(__ANDROID__)
	"#extension GL_ARB_gpu_shader5 : enable\n"
#endif
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	//"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	"\n"
};
#endif // #if defined(USE_GLES2)

const char* vertexInsert_GLSL_ES_3_00 =
{
	"#version 300 es\n"
	"#define PC\n"
	"precision mediump float;\n"
	
	//"#extension GL_ARB_gpu_shader5 : enable\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	//"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	"\n"
};

const char* vertexInsert_GLSL_1_50 =
{
	"#version 150\n"
	"#define PC\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	"\n"
};


#if defined(USE_GLES2)
const char* fragmentInsert_GLSL_ES_1_0 =
{
	"#version 100\n"
	"#define GLES2\n"
	"#define PC\n"
	"precision mediump float;\n"
	"#extension GL_OES_standard_derivatives : enable\n"
	"\n"
	"void clip( float v ) { if ( v < 0.0 ) { discard; } }\n"
	"void clip( vec2 v ) { if ( any( lessThan( v, vec2( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec3 v ) { if ( any( lessThan( v, vec3( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec4 v ) { if ( any( lessThan( v, vec4( 0.0 ) ) ) ) { discard; } }\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"\n"
	"vec4 tex2D( sampler2D sampler, vec2 texcoord ) { return texture2D( sampler, texcoord.xy ); }\n"
	//"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord ) { return vec4( texture( sampler, texcoord.xyz ) ); }\n"
	"\n"
	//"vec4 tex2D( sampler2D sampler, vec2 texcoord, vec2 dx, vec2 dy ) { return textureGrad( sampler, texcoord.xy, dx, dy ); }\n"
	//"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord, vec2 dx, vec2 dy ) { return vec4( textureGrad( sampler, texcoord.xyz, dx, dy ) ); }\n"
	//"\n"
	"vec4 texCUBE( samplerCube sampler, vec3 texcoord ) { return textureCube( sampler, texcoord.xyz ); }\n"
	//"vec4 texCUBE( samplerCubeShadow sampler, vec4 texcoord ) { return vec4( textureCube( sampler, texcoord.xyzw ) ); }\n"
	"\n"
	//"vec4 tex1Dproj( sampler1D sampler, vec2 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"vec4 tex2Dproj( sampler2D sampler, vec3 texcoord ) { return texture2DProj( sampler, texcoord ); }\n"
	//"vec4 tex3Dproj( sampler3D sampler, vec4 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"\n"
	//"vec4 tex1Dbias( sampler1D sampler, vec4 texcoord ) { return texture( sampler, texcoord.x, texcoord.w ); }\n"
	//"vec4 tex2Dbias( sampler2D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xy, texcoord.w ); }\n"
	//"vec4 tex3Dbias( sampler3D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	//"vec4 texCUBEbias( samplerCube sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	//"\n"
	//"vec4 tex1Dlod( sampler1D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.x, texcoord.w ); }\n"
	//"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	//"vec4 tex3Dlod( sampler3D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	//"vec4 texCUBElod( samplerCube sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	//"\n"
};
#else
const char* fragmentInsert_GLSL_ES_1_0 =
{
	"#version 100\n"
	
#if !defined(USE_MESA)
	"#define GLES2\n"
#endif
	
	"#define PC\n"
	
#if 1 //defined(__ANDROID__)
	"precision mediump float;\n"
	//"#extension GL_OES_standard_derivatives : enable\n"
#else
	"precision highp float;\n"
#endif
	"\n"
	"void clip( float v ) { if ( v < 0.0 ) { discard; } }\n"
	"void clip( vec2 v ) { if ( any( lessThan( v, vec2( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec3 v ) { if ( any( lessThan( v, vec3( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec4 v ) { if ( any( lessThan( v, vec4( 0.0 ) ) ) ) { discard; } }\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"\n"
	"vec4 tex2D( sampler2D sampler, vec2 texcoord ) { return texture2D( sampler, texcoord.xy ); }\n"
	//"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord ) { return vec4( texture( sampler, texcoord.xyz ) ); }\n"
	"\n"
	//"vec4 tex2D( sampler2D sampler, vec2 texcoord, vec2 dx, vec2 dy ) { return textureGrad( sampler, texcoord.xy, dx, dy ); }\n"
	//"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord, vec2 dx, vec2 dy ) { return vec4( textureGrad( sampler, texcoord.xyz, dx, dy ) ); }\n"
	//"\n"
	"vec4 texCUBE( samplerCube sampler, vec3 texcoord ) { return textureCube( sampler, texcoord.xyz ); }\n"
	//"vec4 texCUBE( samplerCubeShadow sampler, vec4 texcoord ) { return vec4( textureCube( sampler, texcoord.xyzw ) ); }\n"
	"\n"
	//"vec4 tex1Dproj( sampler1D sampler, vec2 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"vec4 tex2Dproj( sampler2D sampler, vec3 texcoord ) { return texture2DProj( sampler, texcoord ); }\n"
	//"vec4 tex3Dproj( sampler3D sampler, vec4 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"\n"
	//"vec4 tex1Dbias( sampler1D sampler, vec4 texcoord ) { return texture( sampler, texcoord.x, texcoord.w ); }\n"
	//"vec4 tex2Dbias( sampler2D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xy, texcoord.w ); }\n"
	//"vec4 tex3Dbias( sampler3D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	//"vec4 texCUBEbias( samplerCube sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	//"\n"
	//"vec4 tex1Dlod( sampler1D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.x, texcoord.w ); }\n"
	//"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	//"vec4 tex3Dlod( sampler3D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	//"vec4 texCUBElod( samplerCube sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	//"\n"
};
#endif // #if defined(USE_GLES2)


const char* fragmentInsert_GLSL_ES_3_00 =
{
	"#version 300 es\n"
	"#define PC\n"
	"precision mediump float;\n"
	"precision lowp sampler2D;\n"
	"precision lowp sampler2DShadow;\n"
	"precision lowp sampler2DArray;\n"
	"precision lowp sampler2DArrayShadow;\n"
	"precision lowp samplerCube;\n"
	"precision lowp samplerCubeShadow;\n"
	"precision lowp sampler3D;\n"
	"\n"
	"void clip( float v ) { if ( v < 0.0 ) { discard; } }\n"
	"void clip( vec2 v ) { if ( any( lessThan( v, vec2( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec3 v ) { if ( any( lessThan( v, vec3( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec4 v ) { if ( any( lessThan( v, vec4( 0.0 ) ) ) ) { discard; } }\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"\n"
	"vec4 tex2D( sampler2D sampler, vec2 texcoord ) { return texture( sampler, texcoord.xy ); }\n"
	"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord ) { return vec4( texture( sampler, texcoord.xyz ) ); }\n"
	"\n"
	"vec4 tex2D( sampler2D sampler, vec2 texcoord, vec2 dx, vec2 dy ) { return textureGrad( sampler, texcoord.xy, dx, dy ); }\n"
	"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord, vec2 dx, vec2 dy ) { return vec4( textureGrad( sampler, texcoord.xyz, dx, dy ) ); }\n"
	"\n"
	"vec4 texCUBE( samplerCube sampler, vec3 texcoord ) { return texture( sampler, texcoord.xyz ); }\n"
	"vec4 texCUBE( samplerCubeShadow sampler, vec4 texcoord ) { return vec4( texture( sampler, texcoord.xyzw ) ); }\n"
	"\n"
	//"vec4 tex1Dproj( sampler1D sampler, vec2 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"vec4 tex2Dproj( sampler2D sampler, vec3 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"vec4 tex3Dproj( sampler3D sampler, vec4 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"\n"
	//"vec4 tex1Dbias( sampler1D sampler, vec4 texcoord ) { return texture( sampler, texcoord.x, texcoord.w ); }\n"
	"vec4 tex2Dbias( sampler2D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xy, texcoord.w ); }\n"
	"vec4 tex3Dbias( sampler3D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	"vec4 texCUBEbias( samplerCube sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	"\n"
	//"vec4 tex1Dlod( sampler1D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.x, texcoord.w ); }\n"
	"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	"vec4 tex3Dlod( sampler3D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	"vec4 texCUBElod( samplerCube sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	"\n"
};

const char* fragmentInsert_GLSL_1_50 =
{
	"#version 150\n"
	"#define PC\n"
	"\n"
	"void clip( float v ) { if ( v < 0.0 ) { discard; } }\n"
	"void clip( vec2 v ) { if ( any( lessThan( v, vec2( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec3 v ) { if ( any( lessThan( v, vec3( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec4 v ) { if ( any( lessThan( v, vec4( 0.0 ) ) ) ) { discard; } }\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"\n"
	"vec4 tex2D( sampler2D sampler, vec2 texcoord ) { return texture( sampler, texcoord.xy ); }\n"
	"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord ) { return vec4( texture( sampler, texcoord.xyz ) ); }\n"
	"\n"
	"vec4 tex2D( sampler2D sampler, vec2 texcoord, vec2 dx, vec2 dy ) { return textureGrad( sampler, texcoord.xy, dx, dy ); }\n"
	"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord, vec2 dx, vec2 dy ) { return vec4( textureGrad( sampler, texcoord.xyz, dx, dy ) ); }\n"
	"\n"
	"vec4 texCUBE( samplerCube sampler, vec3 texcoord ) { return texture( sampler, texcoord.xyz ); }\n"
	"vec4 texCUBE( samplerCubeShadow sampler, vec4 texcoord ) { return vec4( texture( sampler, texcoord.xyzw ) ); }\n"
	"\n"
	"vec4 tex1Dproj( sampler1D sampler, vec2 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"vec4 tex2Dproj( sampler2D sampler, vec3 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"vec4 tex3Dproj( sampler3D sampler, vec4 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"\n"
	"vec4 tex1Dbias( sampler1D sampler, vec4 texcoord ) { return texture( sampler, texcoord.x, texcoord.w ); }\n"
	"vec4 tex2Dbias( sampler2D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xy, texcoord.w ); }\n"
	"vec4 tex3Dbias( sampler3D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	"vec4 texCUBEbias( samplerCube sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	"\n"
	"vec4 tex1Dlod( sampler1D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.x, texcoord.w ); }\n"
	"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	"vec4 tex3Dlod( sampler3D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	"vec4 texCUBElod( samplerCube sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	"\n"
};
// RB end

struct builtinConversion_t
{
	const char* nameCG;
	const char* nameGLSL;
} builtinConversion[] =
{
	{ "frac",		"fract" },
	{ "lerp",		"mix" },
	{ "rsqrt",		"inversesqrt" },
	{ "ddx",		"dFdx" },
	{ "ddy",		"dFdy" },
	
	{ NULL, NULL }
};

struct inOutVariable_t
{
	idStr	type;
	idStr	nameCg;
	idStr	nameGLSL;
	bool	declareInOut;
};

/*
========================
ParseInOutStruct
========================
*/
void ParseInOutStruct( idLexer& src, int attribType, int attribIgnoreType, idList< inOutVariable_t >& inOutVars )
{
	src.ExpectTokenString( "{" );
	
	while( !src.CheckTokenString( "}" ) )
	{
		inOutVariable_t var;
		
		idToken token;
		src.ReadToken( &token );
		var.type = token;
		src.ReadToken( &token );
		var.nameCg = token;
		
		if( !src.CheckTokenString( ":" ) )
		{
			src.SkipUntilString( ";" );
			continue;
		}
		
		src.ReadToken( &token );
		var.nameGLSL = token;
		src.ExpectTokenString( ";" );
		
		// convert the type
		for( int i = 0; typeConversion[i].typeCG != NULL; i++ )
		{
			if( var.type.Cmp( typeConversion[i].typeCG ) == 0 )
			{
				var.type = typeConversion[i].typeGLSL;
				break;
			}
		}
		
		// convert the semantic to a GLSL name
		for( int i = 0; attribsPC[i].semantic != NULL; i++ )
		{
			if( ( attribsPC[i].flags & attribType ) != 0 )
			{
				if( var.nameGLSL.Cmp( attribsPC[i].semantic ) == 0 )
				{
					var.nameGLSL = attribsPC[i].glsl;
					break;
				}
			}
		}
		
		// check if it was defined previously
		var.declareInOut = true;
		for( int i = 0; i < inOutVars.Num(); i++ )
		{
			if( var.nameGLSL == inOutVars[i].nameGLSL )
			{
				var.declareInOut = false;
				break;
			}
		}
		
		// RB: ignore reserved builtin gl_ uniforms
		switch( glConfig.driverType )
		{
			//case GLDRV_OPENGL32_CORE_PROFILE:
			//case GLDRV_OPENGL_ES2:
			//case GLDRV_OPENGL_ES3:
			//case GLDRV_OPENGL_MESA:
			default:
			{
				for( int i = 0; attribsPC[i].semantic != NULL; i++ )
				{
					if( var.nameGLSL.Cmp( attribsPC[i].glsl ) == 0 )
					{
						if( ( attribsPC[i].flags & attribIgnoreType ) != 0 )
						{
							var.declareInOut = false;
							break;
						}
					}
				}
				
				break;
			}
		}
		// RB end
		
		inOutVars.Append( var );
	}
	
	src.ExpectTokenString( ";" );
}

/*
========================
ConvertCG2GLSL
========================
*/
idStr ConvertCG2GLSL( const idStr& in, const char* name, bool isVertexProgram, idStr& uniforms )
{
	idStr program;
	program.ReAllocate( in.Length() * 2, false );
	
	idList< inOutVariable_t, TAG_RENDERPROG > varsIn;
	idList< inOutVariable_t, TAG_RENDERPROG > varsOut;
	idList< idStr > uniformList;
	
	idLexer src( LEXFL_NOFATALERRORS );
	src.LoadMemory( in.c_str(), in.Length(), name );
	
	bool inMain = false;
	const char* uniformArrayName = isVertexProgram ? VERTEX_UNIFORM_ARRAY_NAME : FRAGMENT_UNIFORM_ARRAY_NAME;
	char newline[128] = { "\n" };
	
	idToken token;
	while( src.ReadToken( &token ) )
	{
		// check for uniforms
		
		// RB: added special case for matrix arrays
		while( token == "uniform" && ( src.CheckTokenString( "float4" ) || src.CheckTokenString( "float4x4" ) ) )
		{
			src.ReadToken( &token );
			idStr uniform = token;
			
			// strip ': register()' from uniforms
			if( src.CheckTokenString( ":" ) )
			{
				if( src.CheckTokenString( "register" ) )
				{
					src.SkipUntilString( ";" );
				}
			}
			
			
			if( src.PeekTokenString( "[" ) )
			{
				while( src.ReadToken( &token ) )
				{
					uniform += token;
					
					if( token == "]" )
					{
						break;
					}
				}
			}
			
			uniformList.Append( uniform );
			
			src.ReadToken( &token );
		}
		// RB end
		
		// convert the in/out structs
		if( token == "struct" )
		{
			if( src.CheckTokenString( "VS_IN" ) )
			{
				ParseInOutStruct( src, AT_VS_IN, 0, varsIn );
				program += "\n\n";
				for( int i = 0; i < varsIn.Num(); i++ )
				{
					if( varsIn[i].declareInOut )
					{
						// RB begin
						switch( glConfig.driverType )
						{
							case GLDRV_OPENGL_ES2:
							case GLDRV_OPENGL_ES3:
								//case GLDRV_OPENGL_MESA:
							{
								program += "attribute " + varsIn[i].type + " " + varsIn[i].nameGLSL + ";\n";
								break;
							}
							
							default:
							{
								program += "in " + varsIn[i].type + " " + varsIn[i].nameGLSL + ";\n";
								break;
							}
						}
						// RB end
					}
				}
				continue;
			}
			else if( src.CheckTokenString( "VS_OUT" ) )
			{
				// RB begin
				ParseInOutStruct( src, AT_VS_OUT, AT_VS_OUT_RESERVED, varsOut );
				// RB end
				
				program += "\n";
				for( int i = 0; i < varsOut.Num(); i++ )
				{
					if( varsOut[i].declareInOut )
					{
						// RB begin
						switch( glConfig.driverType )
						{
							case GLDRV_OPENGL_ES2:
							case GLDRV_OPENGL_ES3:
								//case GLDRV_OPENGL_MESA:
							{
								program += "varying " + varsOut[i].type + " " + varsOut[i].nameGLSL + ";\n";
								break;
							}
							
							default:
							{
								program += "out " + varsOut[i].type + " " + varsOut[i].nameGLSL + ";\n";
								break;
							}
						}
						// RB end
					}
				}
				continue;
			}
			else if( src.CheckTokenString( "PS_IN" ) )
			{
				ParseInOutStruct( src, AT_PS_IN, AT_PS_IN_RESERVED, varsIn );
				program += "\n\n";
				for( int i = 0; i < varsIn.Num(); i++ )
				{
					if( varsIn[i].declareInOut )
					{
						// RB begin
						switch( glConfig.driverType )
						{
							case GLDRV_OPENGL_ES2:
							case GLDRV_OPENGL_ES3:
								//case GLDRV_OPENGL_MESA:
							{
								program += "varying " + varsIn[i].type + " " + varsIn[i].nameGLSL + ";\n";
								break;
							}
							
							default:
								program += "in " + varsIn[i].type + " " + varsIn[i].nameGLSL + ";\n";
								break;
						}
						// RB end
					}
				}
				inOutVariable_t var;
				var.type = "vec4";
				var.nameCg = "position";
				var.nameGLSL = "gl_FragCoord";
				varsIn.Append( var );
				continue;
			}
			else if( src.CheckTokenString( "PS_OUT" ) )
			{
				// RB begin
				ParseInOutStruct( src, AT_PS_OUT, AT_PS_OUT_RESERVED, varsOut );
				// RB end
				
				program += "\n";
				for( int i = 0; i < varsOut.Num(); i++ )
				{
					if( varsOut[i].declareInOut )
					{
						program += "out " + varsOut[i].type + " " + varsOut[i].nameGLSL + ";\n";
					}
				}
				continue;
			}
		}
		
		// strip 'static'
		if( token == "static" )
		{
			program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
			src.SkipWhiteSpace( true ); // remove white space between 'static' and the next token
			continue;
		}
		
		// strip ': register()' from uniforms
		if( token == ":" )
		{
			if( src.CheckTokenString( "register" ) )
			{
				src.SkipUntilString( ";" );
				program += ";";
				continue;
			}
		}
		
		// strip in/program parameters from main
		if( token == "void" && src.CheckTokenString( "main" ) )
		{
			src.ExpectTokenString( "(" );
			while( src.ReadToken( &token ) )
			{
				if( token == ")" )
				{
					break;
				}
			}
			
			program += "\nvoid main()";
			inMain = true;
			continue;
		}
		
		// strip 'const' from local variables in main()
		if( token == "const" && inMain )
		{
			program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
			src.SkipWhiteSpace( true ); // remove white space between 'const' and the next token
			continue;
		}
		
		// maintain indentation
		if( token == "{" )
		{
			program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
			program += "{";
			
			int len = Min( idStr::Length( newline ) + 1, ( int )sizeof( newline ) - 1 );
			newline[len - 1] = '\t';
			newline[len - 0] = '\0';
			continue;
		}
		if( token == "}" )
		{
			int len = Max( idStr::Length( newline ) - 1, 0 );
			newline[len] = '\0';
			
			program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
			program += "}";
			continue;
		}
		
		// check for a type conversion
		bool foundType = false;
		for( int i = 0; typeConversion[i].typeCG != NULL; i++ )
		{
			if( token.Cmp( typeConversion[i].typeCG ) == 0 )
			{
				program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
				program += typeConversion[i].typeGLSL;
				foundType = true;
				break;
			}
		}
		if( foundType )
		{
			continue;
		}
		
		if( r_useUniformArrays.GetBool() )
		{
			// check for uniforms that need to be converted to the array
			bool isUniform = false;
			for( int i = 0; i < uniformList.Num(); i++ )
			{
				if( token == uniformList[i] )
				{
					program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
					
					// RB: for some unknown reasons has the Nvidia driver problems with regular uniforms when using glUniform4fv
					// so we need these uniform arrays
					// I added special check rpShadowMatrices so we can still index the uniforms from the shader
					
					if( idStr::Cmp( uniformList[i].c_str(), "rpShadowMatrices" ) == 0 )
					{
						program += va( "%s[/* %s */ %d + ", uniformArrayName, uniformList[i].c_str(), i );
						
						if( src.ExpectTokenString( "[" ) )
						{
							idStr uniformIndexing;
							
							while( src.ReadToken( &token ) )
							{
								if( token == "]" )
								{
									break;
								}
								
								uniformIndexing += token;
								uniformIndexing += " ";
							}
							
							program += uniformIndexing + "]";
						}
					}
					else
					{
						program += va( "%s[%d /* %s */]", uniformArrayName, i, uniformList[i].c_str() );
					}
					// RB end
					isUniform = true;
					break;
				}
			}
			if( isUniform )
			{
				continue;
			}
		}
		
		// check for input/output parameters
		if( src.CheckTokenString( "." ) )
		{
		
			if( token == "vertex" || token == "fragment" )
			{
				idToken member;
				src.ReadToken( &member );
				
				bool foundInOut = false;
				for( int i = 0; i < varsIn.Num(); i++ )
				{
					if( member.Cmp( varsIn[i].nameCg ) == 0 )
					{
						program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
						program += varsIn[i].nameGLSL;
						foundInOut = true;
						break;
					}
				}
				if( !foundInOut )
				{
					src.Error( "invalid input parameter %s.%s", token.c_str(), member.c_str() );
					program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
					program += token;
					program += ".";
					program += member;
				}
				continue;
			}
			
			if( token == "result" )
			{
				idToken member;
				src.ReadToken( &member );
				
				bool foundInOut = false;
				for( int i = 0; i < varsOut.Num(); i++ )
				{
					if( member.Cmp( varsOut[i].nameCg ) == 0 )
					{
						program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
						program += varsOut[i].nameGLSL;
						foundInOut = true;
						break;
					}
				}
				if( !foundInOut )
				{
					src.Error( "invalid output parameter %s.%s", token.c_str(), member.c_str() );
					program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
					program += token;
					program += ".";
					program += member;
				}
				continue;
			}
			
			program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
			program += token;
			program += ".";
			continue;
		}
		
		// check for a function conversion
		bool foundFunction = false;
		for( int i = 0; builtinConversion[i].nameCG != NULL; i++ )
		{
			if( token.Cmp( builtinConversion[i].nameCG ) == 0 )
			{
				program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
				program += builtinConversion[i].nameGLSL;
				foundFunction = true;
				break;
			}
		}
		if( foundFunction )
		{
			continue;
		}
		
		program += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
		program += token;
	}
	
	idStr out;
	
	// RB: changed to allow multiple versions of GLSL
	if( isVertexProgram )
	{
		switch( glConfig.driverType )
		{
			case GLDRV_OPENGL_ES2:
			case GLDRV_OPENGL_ES3:
			{
				out.ReAllocate( idStr::Length( vertexInsert_GLSL_ES_1_0 ) + in.Length() * 2, false );
				out += vertexInsert_GLSL_ES_1_0;
				break;
			}
			
			case GLDRV_OPENGL_MESA:
			{
				out.ReAllocate( idStr::Length( vertexInsert_GLSL_ES_3_00 ) + in.Length() * 2, false );
				out += vertexInsert_GLSL_ES_3_00;
				break;
			}
			
			default:
			{
				out.ReAllocate( idStr::Length( vertexInsert_GLSL_1_50 ) + in.Length() * 2, false );
				out += vertexInsert_GLSL_1_50;
				break;
			}
		}
		
		
	}
	else
	{
		switch( glConfig.driverType )
		{
			case GLDRV_OPENGL_ES2:
			case GLDRV_OPENGL_ES3:
			{
				out.ReAllocate( idStr::Length( fragmentInsert_GLSL_ES_1_0 ) + in.Length() * 2, false );
				out += fragmentInsert_GLSL_ES_1_0;
				break;
			}
			
			case GLDRV_OPENGL_MESA:
			{
				out.ReAllocate( idStr::Length( fragmentInsert_GLSL_ES_3_00 ) + in.Length() * 2, false );
				out += fragmentInsert_GLSL_ES_3_00;
				break;
			}
			
			default:
			{
				out.ReAllocate( idStr::Length( fragmentInsert_GLSL_1_50 ) + in.Length() * 2, false );
				out += fragmentInsert_GLSL_1_50;
				break;
			}
		}
	}
	// RB end
	
	if( uniformList.Num() > 0 )
	{
		if( r_useUniformArrays.GetBool() )
		{
			int extraSize = 0;
			for( int i = 0; i < uniformList.Num(); i++ )
			{
				if( idStr::Cmp( uniformList[i].c_str(), "rpShadowMatrices" ) == 0 )
				{
					extraSize += ( 6 * 4 );
				}
			}
			
			out += va( "\nuniform vec4 %s[%d];\n", uniformArrayName, uniformList.Num() + extraSize );
		}
		else
		{
			out += "\n";
			for( int i = 0; i < uniformList.Num(); i++ )
			{
				out += "uniform vec4 ";
				out += uniformList[i];
				out += ";\n";
			}
		}
	}
	
	out += program;
	
	for( int i = 0; i < uniformList.Num(); i++ )
	{
		uniforms += uniformList[i];
		uniforms += "\n";
	}
	uniforms += "\n";
	
	return out;
}

/*
================================================================================================
idRenderProgManager::LoadGLSLShader
================================================================================================
*/
GLuint idRenderProgManager::LoadGLSLShader( GLenum target, const char* name, const char* nameOutSuffix, uint32 shaderFeatures, bool builtin, idList<int>& uniforms )
{

	idStr inFile;
	idStr outFileHLSL;
	idStr outFileGLSL;
	idStr outFileUniforms;
	
	// RB: replaced backslashes
	inFile.Format( "renderprogs/%s", name );
	inFile.StripFileExtension();
	outFileHLSL.Format( "renderprogs/hlsl/%s%s", name, nameOutSuffix );
	outFileHLSL.StripFileExtension();
	
	switch( glConfig.driverType )
	{
		case GLDRV_OPENGL_ES2:
		case GLDRV_OPENGL_ES3:
		{
			outFileGLSL.Format( "renderprogs/glsles-1_0/%s%s", name, nameOutSuffix );
			outFileUniforms.Format( "renderprogs/glsles-1_0/%s%s", name, nameOutSuffix );
			break;
		}
		
		case GLDRV_OPENGL_MESA:
		{
			outFileGLSL.Format( "renderprogs/glsles-3_00/%s%s", name, nameOutSuffix );
			outFileUniforms.Format( "renderprogs/glsles-3_00/%s%s", name, nameOutSuffix );
			break;
		}
		
		default:
		{
			outFileGLSL.Format( "renderprogs/glsl-1_50/%s%s", name, nameOutSuffix );
			outFileUniforms.Format( "renderprogs/glsl-1_50/%s%s", name, nameOutSuffix );
		}
	}
	
	outFileGLSL.StripFileExtension();
	outFileUniforms.StripFileExtension();
	
	if( target == GL_FRAGMENT_SHADER )
	{
		inFile += ".pixel";
		outFileHLSL += "_fragment.hlsl";
		outFileGLSL += "_fragment.glsl";
		outFileUniforms += "_fragment.uniforms";
	}
	else
	{
		inFile += ".vertex";
		outFileHLSL += "_vertex.hlsl";
		outFileGLSL += "_vertex.glsl";
		outFileUniforms += "_vertex.uniforms";
	}
	
	// first check whether we already have a valid GLSL file and compare it to the hlsl timestamp;
	ID_TIME_T hlslTimeStamp;
	int hlslFileLength = fileSystem->ReadFile( inFile.c_str(), NULL, &hlslTimeStamp );
	
	ID_TIME_T glslTimeStamp;
	int glslFileLength = fileSystem->ReadFile( outFileGLSL.c_str(), NULL, &glslTimeStamp );
	
	// if the glsl file doesn't exist or we have a newer HLSL file we need to recreate the glsl file.
	idStr programGLSL;
	idStr programUniforms;
	if( ( glslFileLength <= 0 ) || ( hlslTimeStamp != FILE_NOT_FOUND_TIMESTAMP && hlslTimeStamp > glslTimeStamp ) || r_alwaysExportGLSL.GetBool() )
	{
		const char* hlslFileBuffer = NULL;
		int len = 0;
		
		if( hlslFileLength <= 0 )
		{
			// hlsl file doesn't even exist bail out
			hlslFileBuffer = FindEmbeddedSourceShader( inFile.c_str() );
			if( hlslFileBuffer == NULL )
			{
				return false;
			}
			len = strlen( hlslFileBuffer );
		}
		else
		{
			len = fileSystem->ReadFile( inFile.c_str(), ( void** ) &hlslFileBuffer );
		}
		
		if( len <= 0 )
		{
			return false;
		}
		
		idStrList compileMacros;
		for( int j = 0; j < MAX_SHADER_MACRO_NAMES; j++ )
		{
			if( BIT( j ) & shaderFeatures )
			{
				const char* macroName = GetGLSLMacroName( ( shaderFeature_t ) j );
				compileMacros.Append( idStr( macroName ) );
			}
		}
		
		idStr hlslCode( hlslFileBuffer );
		idStr programHLSL = StripDeadCode( hlslCode, inFile, compileMacros, builtin );
		programGLSL = ConvertCG2GLSL( programHLSL, inFile, target == GL_VERTEX_SHADER, programUniforms );
		
		fileSystem->WriteFile( outFileHLSL, programHLSL.c_str(), programHLSL.Length(), "fs_savepath" );
		fileSystem->WriteFile( outFileGLSL, programGLSL.c_str(), programGLSL.Length(), "fs_savepath" );
		if( r_useUniformArrays.GetBool() )
		{
			fileSystem->WriteFile( outFileUniforms, programUniforms.c_str(), programUniforms.Length(), "fs_savepath" );
		}
	}
	else
	{
		// read in the glsl file
		void* fileBufferGLSL = NULL;
		int lengthGLSL = fileSystem->ReadFile( outFileGLSL.c_str(), &fileBufferGLSL );
		if( lengthGLSL <= 0 )
		{
			idLib::Error( "GLSL file %s could not be loaded and may be corrupt", outFileGLSL.c_str() );
		}
		programGLSL = ( const char* ) fileBufferGLSL;
		Mem_Free( fileBufferGLSL );
		
		if( r_useUniformArrays.GetBool() )
		{
			// read in the uniform file
			void* fileBufferUniforms = NULL;
			int lengthUniforms = fileSystem->ReadFile( outFileUniforms.c_str(), &fileBufferUniforms );
			if( lengthUniforms <= 0 )
			{
				idLib::Error( "uniform file %s could not be loaded and may be corrupt", outFileUniforms.c_str() );
			}
			programUniforms = ( const char* ) fileBufferUniforms;
			Mem_Free( fileBufferUniforms );
		}
	}
	
	// find the uniforms locations in either the vertex or fragment uniform array
	if( r_useUniformArrays.GetBool() )
	{
		uniforms.Clear();
		
		idLexer src( programUniforms, programUniforms.Length(), "uniforms" );
		idToken token;
		while( src.ReadToken( &token ) )
		{
			int index = -1;
			for( int i = 0; i < RENDERPARM_TOTAL && index == -1; i++ )
			{
				const char* parmName = GetGLSLParmName( i );
				if( token == parmName )
				{
					index = i;
				}
			}
			for( int i = 0; i < MAX_GLSL_USER_PARMS && index == -1; i++ )
			{
				const char* parmName = GetGLSLParmName( RENDERPARM_USER + i );
				if( token == parmName )
				{
					index = RENDERPARM_USER + i;
				}
			}
			if( index == -1 )
			{
				idLib::Error( "couldn't find uniform %s for %s", token.c_str(), outFileGLSL.c_str() );
			}
			uniforms.Append( index );
		}
	}
	
	// create and compile the shader
	const GLuint shader = glCreateShader( target );
	if( shader )
	{
		const char* source[1] = { programGLSL.c_str() };
		
		glShaderSource( shader, 1, source, NULL );
		glCompileShader( shader );
		
		int infologLength = 0;
		glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infologLength );
		if( infologLength > 1 )
		{
			idTempArray<char> infoLog( infologLength );
			int charsWritten = 0;
			glGetShaderInfoLog( shader, infologLength, &charsWritten, infoLog.Ptr() );
			
			// catch the strings the ATI and Intel drivers output on success
			if( strstr( infoLog.Ptr(), "successfully compiled to run on hardware" ) != NULL ||
					strstr( infoLog.Ptr(), "No errors." ) != NULL )
			{
				//idLib::Printf( "%s program %s from %s compiled to run on hardware\n", typeName, GetName(), GetFileName() );
			}
			else if( r_displayGLSLCompilerMessages.GetBool() ) // DG:  check for the CVar I added above
			{
				idLib::Printf( "While compiling %s program %s\n", ( target == GL_FRAGMENT_SHADER ) ? "fragment" : "vertex" , inFile.c_str() );
				
				const char separator = '\n';
				idList<idStr> lines;
				lines.Clear();
				idStr source( programGLSL );
				lines.Append( source );
				for( int index = 0, ofs = lines[index].Find( separator ); ofs != -1; index++, ofs = lines[index].Find( separator ) )
				{
					lines.Append( lines[index].c_str() + ofs + 1 );
					lines[index].CapLength( ofs );
				}
				
				idLib::Printf( "-----------------\n" );
				for( int i = 0; i < lines.Num(); i++ )
				{
					idLib::Printf( "%3d: %s\n", i + 1, lines[i].c_str() );
				}
				idLib::Printf( "-----------------\n" );
				
				idLib::Printf( "%s\n", infoLog.Ptr() );
			}
		}
		
		GLint compiled = GL_FALSE;
		glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
		if( compiled == GL_FALSE )
		{
			glDeleteShader( shader );
			return INVALID_PROGID;
		}
	}
	
	return shader;
}
/*
================================================================================================
idRenderProgManager::FindGLSLProgram
================================================================================================
*/
int	 idRenderProgManager::FindGLSLProgram( const char* name, int vIndex, int fIndex )
{

	for( int i = 0; i < glslPrograms.Num(); ++i )
	{
		if( ( glslPrograms[i].vertexShaderIndex == vIndex ) && ( glslPrograms[i].fragmentShaderIndex == fIndex ) )
		{
			LoadGLSLProgram( i, vIndex, fIndex );
			return i;
		}
	}
	
	glslProgram_t program;
	program.name = name;
	int index = glslPrograms.Append( program );
	LoadGLSLProgram( index, vIndex, fIndex );
	return index;
}

/*
================================================================================================
idRenderProgManager::GetGLSLParmName
================================================================================================
*/
const char* idRenderProgManager::GetGLSLParmName( int rp ) const
{
	if( rp >= RENDERPARM_USER )
	{
		int userParmIndex = rp - RENDERPARM_USER;
		return va( "rpUser%d", userParmIndex );
	}
	assert( rp < RENDERPARM_TOTAL );
	return GLSLParmNames[ rp ];
}

// RB begin
const char* idRenderProgManager::GetGLSLMacroName( shaderFeature_t sf ) const
{
	assert( sf < MAX_SHADER_MACRO_NAMES );
	
	return GLSLMacroNames[ sf ];
}
// RB end

/*
================================================================================================
idRenderProgManager::SetUniformValue
================================================================================================
*/
void idRenderProgManager::SetUniformValue( const renderParm_t rp, const float* value )
{
	for( int i = 0; i < 4; i++ )
	{
		glslUniforms[rp][i] = value[i];
	}
}

/*
================================================================================================
idRenderProgManager::CommitUnforms
================================================================================================
*/
void idRenderProgManager::CommitUniforms()
{
	const int progID = GetGLSLCurrentProgram();
	const glslProgram_t& prog = glslPrograms[progID];
	
	//GL_CheckErrors();
	
	if( r_useUniformArrays.GetBool() )
	{
		ALIGNTYPE16 idVec4 localVectors[RENDERPARM_USER + MAX_GLSL_USER_PARMS];
		
		if( prog.vertexShaderIndex >= 0 )
		{
			const idList<int>& vertexUniforms = vertexShaders[prog.vertexShaderIndex].uniforms;
			if( prog.vertexUniformArray != -1 && vertexUniforms.Num() > 0 )
			{
				int totalUniforms = 0;
				for( int i = 0; i < vertexUniforms.Num(); i++ )
				{
					// RB: HACK rpShadowMatrices[6 * 4]
					if( vertexUniforms[i] == RENDERPARM_SHADOW_MATRIX_0_X )
					{
						for( int j = 0; j < ( 6 * 4 ); j++ )
						{
							localVectors[i + j] = glslUniforms[vertexUniforms[i] + j];
							totalUniforms++;
						}
						
					}
					else
					{
						localVectors[i] = glslUniforms[vertexUniforms[i]];
						totalUniforms++;
					}
				}
				glUniform4fv( prog.vertexUniformArray, totalUniforms, localVectors->ToFloatPtr() );
			}
		}
		
		if( prog.fragmentShaderIndex >= 0 )
		{
			const idList<int>& fragmentUniforms = fragmentShaders[prog.fragmentShaderIndex].uniforms;
			if( prog.fragmentUniformArray != -1 && fragmentUniforms.Num() > 0 )
			{
				int totalUniforms = 0;
				for( int i = 0; i < fragmentUniforms.Num(); i++ )
				{
					// RB: HACK rpShadowMatrices[6 * 4]
					if( fragmentUniforms[i] == RENDERPARM_SHADOW_MATRIX_0_X )
					{
						for( int j = 0; j < ( 6 * 4 ); j++ )
						{
							localVectors[i + j] = glslUniforms[fragmentUniforms[i] + j];
							totalUniforms++;
						}
						
					}
					else
					{
						localVectors[i] = glslUniforms[fragmentUniforms[i]];
						totalUniforms++;
					}
				}
				glUniform4fv( prog.fragmentUniformArray, totalUniforms, localVectors->ToFloatPtr() );
			}
		}
	}
	else
	{
		for( int i = 0; i < prog.uniformLocations.Num(); i++ )
		{
			const glslUniformLocation_t& uniformLocation = prog.uniformLocations[i];
			
			// RB: HACK rpShadowMatrices[6 * 4]
			if( uniformLocation.parmIndex == RENDERPARM_SHADOW_MATRIX_0_X )
			{
				glUniform4fv( uniformLocation.uniformIndex, 6 * 4, glslUniforms[uniformLocation.parmIndex].ToFloatPtr() );
			}
			else
			{
				glUniform4fv( uniformLocation.uniformIndex, 1, glslUniforms[uniformLocation.parmIndex].ToFloatPtr() );
				
#if 0
				if( GL_CheckErrors() )
				{
					const char* parmName = GetGLSLParmName( uniformLocation.parmIndex );
					const char* value = glslUniforms[uniformLocation.parmIndex].ToString();
					
					idLib::Printf( "glUniform4fv( %i = %s, value = %s ) failed for %s\n", uniformLocation.parmIndex, parmName, value, prog.name.c_str() );
				}
#endif
			}
			// RB end
		}
	}
	
	//GL_CheckErrors();
}

class idSort_QuickUniforms : public idSort_Quick< glslUniformLocation_t, idSort_QuickUniforms >
{
public:
	int Compare( const glslUniformLocation_t& a, const glslUniformLocation_t& b ) const
	{
		return a.uniformIndex - b.uniformIndex;
	}
};

/*
================================================================================================
idRenderProgManager::LoadGLSLProgram
================================================================================================
*/
void idRenderProgManager::LoadGLSLProgram( const int programIndex, const int vertexShaderIndex, const int fragmentShaderIndex )
{
	glslProgram_t& prog = glslPrograms[programIndex];
	
	if( prog.progId != INVALID_PROGID )
	{
		return; // Already loaded
	}
	
	GLuint vertexProgID = ( vertexShaderIndex != -1 ) ? vertexShaders[ vertexShaderIndex ].progId : INVALID_PROGID;
	GLuint fragmentProgID = ( fragmentShaderIndex != -1 ) ? fragmentShaders[ fragmentShaderIndex ].progId : INVALID_PROGID;
	
	const GLuint program = glCreateProgram();
	if( program )
	{
	
		if( vertexProgID != INVALID_PROGID )
		{
			glAttachShader( program, vertexProgID );
		}
		
		if( fragmentProgID != INVALID_PROGID )
		{
			glAttachShader( program, fragmentProgID );
		}
		
		// bind vertex attribute locations
		for( int i = 0; attribsPC[i].glsl != NULL; i++ )
		{
			if( ( attribsPC[i].flags & AT_VS_IN ) != 0 )
			{
				glBindAttribLocation( program, attribsPC[i].bind, attribsPC[i].glsl );
			}
		}
		
		glLinkProgram( program );
		
		int infologLength = 0;
		glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infologLength );
		if( infologLength > 1 )
		{
			char* infoLog = ( char* )malloc( infologLength );
			int charsWritten = 0;
			glGetProgramInfoLog( program, infologLength, &charsWritten, infoLog );
			
			// catch the strings the ATI and Intel drivers output on success
			if( strstr( infoLog, "Vertex shader(s) linked, fragment shader(s) linked." ) != NULL || strstr( infoLog, "No errors." ) != NULL )
			{
				//idLib::Printf( "render prog %s from %s linked\n", GetName(), GetFileName() );
			}
			else
			{
				idLib::Printf( "While linking GLSL program %d with vertexShader %s and fragmentShader %s\n",
							   programIndex,
							   ( vertexShaderIndex >= 0 ) ? vertexShaders[vertexShaderIndex].name.c_str() : "<Invalid>",
							   ( fragmentShaderIndex >= 0 ) ? fragmentShaders[ fragmentShaderIndex ].name.c_str() : "<Invalid>" );
				idLib::Printf( "%s\n", infoLog );
			}
			
			free( infoLog );
		}
	}
	
	int linked = GL_FALSE;
	glGetProgramiv( program, GL_LINK_STATUS, &linked );
	if( linked == GL_FALSE )
	{
		glDeleteProgram( program );
		idLib::Error( "While linking GLSL program %d with vertexShader %s and fragmentShader %s\n",
					  programIndex,
					  ( vertexShaderIndex >= 0 ) ? vertexShaders[vertexShaderIndex].name.c_str() : "<Invalid>",
					  ( fragmentShaderIndex >= 0 ) ? fragmentShaders[ fragmentShaderIndex ].name.c_str() : "<Invalid>" );
		return;
	}
	
	if( r_useUniformArrays.GetBool() )
	{
		prog.vertexUniformArray = glGetUniformLocation( program, VERTEX_UNIFORM_ARRAY_NAME );
		prog.fragmentUniformArray = glGetUniformLocation( program, FRAGMENT_UNIFORM_ARRAY_NAME );
		
		assert( prog.vertexUniformArray != -1 || vertexShaderIndex < 0 || vertexShaders[vertexShaderIndex].uniforms.Num() == 0 );
		assert( prog.fragmentUniformArray != -1 || fragmentShaderIndex < 0 || fragmentShaders[fragmentShaderIndex].uniforms.Num() == 0 );
	}
	else
	{
		// store the uniform locations after we have linked the GLSL program
		prog.uniformLocations.Clear();
		for( int i = 0; i < RENDERPARM_TOTAL; i++ )
		{
			const char* parmName = GetGLSLParmName( i );
			GLint loc = glGetUniformLocation( program, parmName );
			if( loc != -1 )
			{
				glslUniformLocation_t uniformLocation;
				uniformLocation.parmIndex = i;
				uniformLocation.uniformIndex = loc;
				prog.uniformLocations.Append( uniformLocation );
			}
		}
		
		// store the USER uniform locations
		for( int i = 0; i < MAX_GLSL_USER_PARMS; i++ )
		{
			const char* parmName = GetGLSLParmName( RENDERPARM_USER + i );
			GLint loc = glGetUniformLocation( program, parmName );
			if( loc != -1 )
			{
				glslUniformLocation_t uniformLocation;
				uniformLocation.parmIndex = RENDERPARM_USER + i;
				uniformLocation.uniformIndex = loc;
				prog.uniformLocations.Append( uniformLocation );
			}
		}
		
		// sort the uniforms based on index
		prog.uniformLocations.SortWithTemplate( idSort_QuickUniforms() );
	}
	
	// RB: only load joint uniform buffers if available
	if( glConfig.gpuSkinningAvailable )
	{
		// get the uniform buffer binding for skinning joint matrices
		GLint blockIndex = glGetUniformBlockIndex( program, "matrices_ubo" );
		if( blockIndex != -1 )
		{
			glUniformBlockBinding( program, blockIndex, 0 );
		}
	}
	// RB end
	
	// set the texture unit locations once for the render program. We only need to do this once since we only link the program once
	glUseProgram( program );
	int numSamplerUniforms = 0;
	for( int i = 0; i < MAX_PROG_TEXTURE_PARMS; ++i )
	{
		GLint loc = glGetUniformLocation( program, va( "samp%d", i ) );
		if( loc != -1 )
		{
			glUniform1i( loc, i );
			numSamplerUniforms++;
		}
	}
	
	// RB: make sure that we collected all uniforms we are interested in
	if( !r_useUniformArrays.GetBool() )
	{
		int numActiveUniforms;
		glGetProgramiv( program, GL_ACTIVE_UNIFORMS, &numActiveUniforms );
		GL_CheckErrors();
		
		if( ( numActiveUniforms - numSamplerUniforms ) != prog.uniformLocations.Num() )
		{
			int				size;
			GLenum			type;
			char            uniformName[1000];
			
			for( int i = 0; i < numActiveUniforms; i++ )
			{
				glGetActiveUniform( program, i, sizeof( uniformName ), NULL, &size, &type, uniformName );
				
				idLib::Printf( "active uniform: '%s'\n", uniformName );
			}
		}
	}
	
	idStr programName = vertexShaders[ vertexShaderIndex ].name;
	programName.StripFileExtension();
	prog.name = programName;
	prog.progId = program;
	prog.fragmentShaderIndex = fragmentShaderIndex;
	prog.vertexShaderIndex = vertexShaderIndex;
}

/*
================================================================================================
idRenderProgManager::ZeroUniforms
================================================================================================
*/
void idRenderProgManager::ZeroUniforms()
{
	memset( glslUniforms.Ptr(), 0, glslUniforms.Allocated() );
}

