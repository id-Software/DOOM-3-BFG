#include "gltfParser.h"

#ifndef gltfExtraParser
#define gltfExtraParser(className,ptype)													\
	class gltfExtra_##className : public parsable, public parseType<ptype>					\
	{public:																				\
		gltfExtra_##className( idStr Name ) : name( Name ){ item = nullptr; }				\
		virtual void parse( idToken &token ){parse(token,nullptr);}							\
		virtual void parse( idToken &token , idLexer * parser );							\
		virtual idStr &Name( ) { return name; }												\
	private:																				\
		idStr name;}
#pragma endregion 
#endif

//Helper macros for gltf data deserialize
#define GLTFARRAYITEM(target,name,type) auto * name = new type (#name); target.AddItemDef((parsable*)name)
#define GLTFARRAYITEMREF(target,name,type,ref) auto * name = new type (#name); target.AddItemDef((parsable*)name); name->Set(&ref)
#ifndef GLTF_EXTRAS_H
#define GLTF_EXTRAS_H

class test {
public: 
	test(){ }
};


gltfExtraParser( Scatter, test );

gltfExtraParser( cvar, idCVar );
#endif // GLTF_EXTRAS_H

#ifndef gltfExternalParser
#undef gltfExtraParser
#endif