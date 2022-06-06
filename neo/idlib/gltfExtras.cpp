#include "precompiled.h"
#pragma hdrstop
#include "gltfExtras.h"

extern idCVar gltf_parseVerbose;

void gltfExtra_Scatter::parse( idToken& token, idLexer* parser )
{

	parser->UnreadToken( &token );

	gltfItemArray scatterInfo;
	GLTFARRAYITEM( scatterInfo, emitter, gltfObject );
	scatterInfo.Parse( parser, true );
}

void gltfExtra_cvar::parse( idToken& token, idLexer* parser )
{

	parser->UnreadToken( &token );
	gltfItemArray cvarInfo;
	idStr n, t, v, d;
	GLTFARRAYITEMREF( cvarInfo, name, gltfItem , n );
	GLTFARRAYITEMREF( cvarInfo, type, gltfItem , t );
	GLTFARRAYITEMREF( cvarInfo, value, gltfItem, v );
	GLTFARRAYITEMREF( cvarInfo, desc, gltfItem , d );
	int total = cvarInfo.Parse( parser );
	assert( total == 3 );
	idCVar* gltExtra_cvar = new idCVar(
		n.c_str(),
		v.c_str(),
		CVAR_SYSTEM | CVAR_BOOL,
		d.c_str()
	);

	cvarSystem->Register( gltExtra_cvar );
}