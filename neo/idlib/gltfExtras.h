/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2022 - 2023 Harrie van Ginneken

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

#include "gltfParser.h"

#ifndef gltfExtraParser
#define gltfExtraParser(className,ptype)										\
	class gltfExtra_##className : public parsable, public parseType<ptype>		\
	{public:																	\
		gltfExtra_##className( idStr Name ) : name( Name ){ item = nullptr; }	\
		virtual void parse( idToken &token ) {parse(token,nullptr); }			\
		virtual void parse( idToken &token , idLexer * parser );				\
		virtual idStr &Name() { return name; }									\
	private:																	\
		idStr name;}

#endif

//Helper macros for gltf data deserialize
#define GLTFARRAYITEM(target,name,type) auto * name = new type (#name); target.AddItemDef((parsable*)name)
#define GLTFARRAYITEMREF(target,name,type,ref) auto * name = new type (#name); target.AddItemDef((parsable*)name); name->Set(&ref)

#ifndef GLTF_EXTRAS_H
#define GLTF_EXTRAS_H

class gltfExtraStub
{
public:
	gltfExtraStub() { }
};

gltfExtraParser( Scatter, gltfExtraStub );

gltfExtraParser( CameraLensFrames, idList<double> );

#endif // GLTF_EXTRAS_H
#undef gltfExtraParser