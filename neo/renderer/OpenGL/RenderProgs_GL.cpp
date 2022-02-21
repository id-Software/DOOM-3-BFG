/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2018 Robert Beckebans
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

#include "../RenderCommon.h"


idCVar r_displayGLSLCompilerMessages( "r_displayGLSLCompilerMessages", "1", CVAR_BOOL | CVAR_ARCHIVE, "Show info messages the GPU driver outputs when compiling the shaders" );
idCVar r_alwaysExportGLSL( "r_alwaysExportGLSL", "1", CVAR_BOOL, "" );

/*
========================
idRenderProgManager::StartFrame
========================
*/
void idRenderProgManager::StartFrame()
{

}

/*
================================================================================================
idRenderProgManager::BindProgram
================================================================================================
*/
void idRenderProgManager::BindProgram( int index )
{
	if( current == index )
	{
		return;
	}

	current = index;

	RENDERLOG_PRINTF( "Binding GLSL Program %s\n", renderProgs[ index ].name.c_str() );
	glUseProgram( renderProgs[ index ].progId );
}

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind()
{
	current = -1;

	glUseProgram( 0 );
}

/*
================================================================================================
idRenderProgManager::LoadShader
================================================================================================
*/
void idRenderProgManager::LoadShader( int index, rpStage_t stage )
{
	if( shaders[index].progId != INVALID_PROGID )
	{
		return; // Already loaded
	}

	LoadShader( shaders[index] );
}

/*
================================================================================================
idRenderProgManager::LoadGLSLShader
================================================================================================
*/
void idRenderProgManager::LoadShader( shader_t& shader )
{
	idStr inFile;
	idStr outFileHLSL;
	idStr outFileGLSL;
	idStr outFileUniforms;

	// RB: replaced backslashes
	inFile.Format( "renderprogs/%s", shader.name.c_str() );
	inFile.StripFileExtension();
	outFileHLSL.Format( "renderprogs/hlsl/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
	outFileHLSL.StripFileExtension();

	switch( glConfig.driverType )
	{
		case GLDRV_OPENGL_MESA:
		{
			outFileGLSL.Format( "renderprogs/glsles-3_00/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
			outFileUniforms.Format( "renderprogs/glsles-3_00/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
			break;
		}

		case GLDRV_VULKAN:
		{
			outFileGLSL.Format( "renderprogs/vkglsl/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
			outFileUniforms.Format( "renderprogs/vkglsl/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
			break;
		}

		default:
		{
			//SRS - OSX supports only up to GLSL 4.1
#if defined(__APPLE__)
			outFileGLSL.Format( "renderprogs/glsl-4_10/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
			outFileUniforms.Format( "renderprogs/glsl-4_10/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
#else
			outFileGLSL.Format( "renderprogs/glsl-4_50/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
			outFileUniforms.Format( "renderprogs/glsl-4_50/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
#endif
		}
	}

	outFileGLSL.StripFileExtension();
	outFileUniforms.StripFileExtension();

	GLenum glTarget;
	if( shader.stage == SHADER_STAGE_FRAGMENT )
	{
		glTarget = GL_FRAGMENT_SHADER;
		inFile += ".ps.hlsl";
		outFileHLSL += ".ps.hlsl";
		outFileGLSL += ".frag";
		outFileUniforms += ".frag.layout";
	}
	else
	{
		glTarget = GL_VERTEX_SHADER;
		inFile += ".vs.hlsl";
		outFileHLSL += ".vs.hlsl";
		outFileGLSL += ".vert";
		outFileUniforms += ".vert.layout";
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
				return;
			}
			len = strlen( hlslFileBuffer );
		}
		else
		{
			len = fileSystem->ReadFile( inFile.c_str(), ( void** ) &hlslFileBuffer );
		}

		if( len <= 0 )
		{
			return;
		}

		idStrList compileMacros;
		for( int j = 0; j < MAX_SHADER_MACRO_NAMES; j++ )
		{
			if( BIT( j ) & shader.shaderFeatures )
			{
				const char* macroName = GetGLSLMacroName( ( shaderFeature_t ) j );
				compileMacros.Append( idStr( macroName ) );
			}
		}

		// FIXME: we should really scan the program source code for using rpEnableSkinning but at this
		// point we directly load a binary and the program source code is not available on the consoles
		bool hasGPUSkinning = false;

		if(	idStr::Icmp( shader.name.c_str(), "heatHaze" ) == 0 ||
				idStr::Icmp( shader.name.c_str(), "heatHazeWithMask" ) == 0 ||
				idStr::Icmp( shader.name.c_str(), "heatHazeWithMaskAndVertex" ) == 0 ||
				( BIT( USE_GPU_SKINNING ) & shader.shaderFeatures ) )
		{
			hasGPUSkinning = true;
		}

		idStr hlslCode( hlslFileBuffer );
		idStr programHLSL = StripDeadCode( hlslCode, inFile, compileMacros, shader.builtin );
		programGLSL = ConvertCG2GLSL( programHLSL, inFile.c_str(), shader.stage, programUniforms, false, hasGPUSkinning, shader.vertexLayout );
		fileSystem->WriteFile( outFileHLSL, programHLSL.c_str(), programHLSL.Length(), "fs_savepath" );
		fileSystem->WriteFile( outFileGLSL, programGLSL.c_str(), programGLSL.Length(), "fs_savepath" );
		fileSystem->WriteFile( outFileUniforms, programUniforms.c_str(), programUniforms.Length(), "fs_savepath" );
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

	// RB: find the uniforms locations in either the vertex or fragment uniform array
	// this uses the new layout structure
	{
		shader.uniforms.Clear();

		idLexer src( programUniforms, programUniforms.Length(), "uniforms" );
		idToken token;
		if( src.ExpectTokenString( "uniforms" ) )
		{
			src.ExpectTokenString( "[" );

			while( !src.CheckTokenString( "]" ) )
			{
				src.ReadToken( &token );

				int index = -1;
				for( int i = 0; i < RENDERPARM_TOTAL && index == -1; i++ )
				{
					const char* parmName = GetGLSLParmName( i );
					if( token == parmName )
					{
						index = i;
					}
				}

				if( index == -1 )
				{
					idLib::Error( "couldn't find uniform %s for %s", token.c_str(), outFileGLSL.c_str() );
				}
				shader.uniforms.Append( index );
			}
		}
	}

	// create and compile the shader
	shader.progId = glCreateShader( glTarget );
	if( shader.progId )
	{
		const char* source[1] = { programGLSL.c_str() };

		glShaderSource( shader.progId, 1, source, NULL );
		glCompileShader( shader.progId );

		int infologLength = 0;
		glGetShaderiv( shader.progId, GL_INFO_LOG_LENGTH, &infologLength );
		if( infologLength > 1 )
		{
			idTempArray<char> infoLog( infologLength );
			int charsWritten = 0;
			glGetShaderInfoLog( shader.progId, infologLength, &charsWritten, infoLog.Ptr() );

			// catch the strings the ATI and Intel drivers output on success
			if( strstr( infoLog.Ptr(), "successfully compiled to run on hardware" ) != NULL ||
					strstr( infoLog.Ptr(), "No errors." ) != NULL )
			{
				//idLib::Printf( "%s program %s from %s compiled to run on hardware\n", typeName, GetName(), GetFileName() );
			}
			else if( r_displayGLSLCompilerMessages.GetBool() ) // DG:  check for the CVar I added above
			{
				idLib::Printf( "While compiling %s program %s\n", ( shader.stage == SHADER_STAGE_FRAGMENT ) ? "fragment" : "vertex" , inFile.c_str() );

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
		glGetShaderiv( shader.progId, GL_COMPILE_STATUS, &compiled );
		if( compiled == GL_FALSE )
		{
			glDeleteShader( shader.progId );
			return;
		}
	}
}

/*
================================================================================================
idRenderProgManager::LoadGLSLProgram
================================================================================================
*/
void idRenderProgManager::LoadGLSLProgram( const int programIndex, const int vertexShaderIndex, const int fragmentShaderIndex )
{
	renderProg_t& prog = renderProgs[programIndex];

	if( prog.progId != INVALID_PROGID )
	{
		return; // Already loaded
	}

	//shader_t& vertexShader = shaders[ vertexShaderIndex ];
	//shader_t& fragmentShader = shaders[ fragmentShaderIndex ];

	GLuint vertexProgID = ( vertexShaderIndex != -1 ) ? shaders[ vertexShaderIndex ].progId : INVALID_PROGID;
	GLuint fragmentProgID = ( fragmentShaderIndex != -1 ) ? shaders[ fragmentShaderIndex ].progId : INVALID_PROGID;

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
							   ( vertexShaderIndex >= 0 ) ? shaders[vertexShaderIndex].name.c_str() : "<Invalid>",
							   ( fragmentShaderIndex >= 0 ) ? shaders[ fragmentShaderIndex ].name.c_str() : "<Invalid>" );
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
					  ( vertexShaderIndex >= 0 ) ? shaders[vertexShaderIndex].name.c_str() : "<Invalid>",
					  ( fragmentShaderIndex >= 0 ) ? shaders[ fragmentShaderIndex ].name.c_str() : "<Invalid>" );
		return;
	}

	//shaders[ vertexShaderIndex ].uniformArray = glGetUniformLocation( program, VERTEX_UNIFORM_ARRAY_NAME );
	//shaders[ fragmentShaderIndex ].uniformArray = glGetUniformLocation( program, FRAGMENT_UNIFORM_ARRAY_NAME );

	if( vertexShaderIndex > -1 && shaders[ vertexShaderIndex ].uniforms.Num() > 0 )
	{
		shader_t& vertexShader = shaders[ vertexShaderIndex ];
		vertexShader.uniformArray = glGetUniformLocation( program, VERTEX_UNIFORM_ARRAY_NAME );
	}

	if( fragmentShaderIndex > -1 && shaders[ fragmentShaderIndex ].uniforms.Num() > 0 )
	{
		shader_t& fragmentShader = shaders[ fragmentShaderIndex ];
		fragmentShader.uniformArray = glGetUniformLocation( program, FRAGMENT_UNIFORM_ARRAY_NAME );
	}

	assert( shaders[ vertexShaderIndex ].uniformArray != -1 || vertexShaderIndex > -1 || shaders[vertexShaderIndex].uniforms.Num() == 0 );
	assert( shaders[ fragmentShaderIndex ].uniformArray != -1 || fragmentShaderIndex > -1 || shaders[fragmentShaderIndex].uniforms.Num() == 0 );


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

	idStr programName = shaders[ vertexShaderIndex ].name;
	programName.StripFileExtension();
	prog.name = programName;
	prog.progId = program;
	prog.fragmentShaderIndex = fragmentShaderIndex;
	prog.vertexShaderIndex = vertexShaderIndex;

	// RB: removed idStr::Icmp( name, "heatHaze.vfp" ) == 0  hack
	// this requires r_useUniformArrays 1
	for( int i = 0; i < shaders[vertexShaderIndex].uniforms.Num(); i++ )
	{
		if( shaders[vertexShaderIndex].uniforms[i] == RENDERPARM_ENABLE_SKINNING )
		{
			prog.usesJoints = true;
			prog.optionalSkinning = true;
		}
	}
	// RB end
}



/*
================================================================================================
idRenderProgManager::CommitUnforms
================================================================================================
*/
void idRenderProgManager::CommitUniforms( uint64 stateBits )
{
	const int progID = current;
	const renderProg_t& prog = renderProgs[progID];

	//GL_CheckErrors();

	ALIGNTYPE16 idVec4 localVectors[RENDERPARM_TOTAL];

	auto commitarray = [&]( idVec4( &vectors )[ RENDERPARM_TOTAL ] , shader_t& shader )
	{
		const int numUniforms = shader.uniforms.Num();
		if( shader.uniformArray != -1 && numUniforms > 0 )
		{
			int totalUniforms = 0;
			for( int i = 0; i < numUniforms; ++i )
			{
				// RB: HACK rpShadowMatrices[6 * 4]
				if( shader.uniforms[i] == RENDERPARM_SHADOW_MATRIX_0_X )
				{
					for( int j = 0; j < ( 6 * 4 ); j++ )
					{
						vectors[i + j] = uniforms[ shader.uniforms[i] + j];
						totalUniforms++;
					}

				}
				else
				{
					vectors[i] = uniforms[ shader.uniforms[i] ];
					totalUniforms++;
				}
			}
			glUniform4fv( shader.uniformArray, totalUniforms, localVectors->ToFloatPtr() );
		}
	};

	if( prog.vertexShaderIndex >= 0 )
	{
		commitarray( localVectors, shaders[ prog.vertexShaderIndex ] );
	}

	if( prog.fragmentShaderIndex >= 0 )
	{
		commitarray( localVectors, shaders[ prog.fragmentShaderIndex ] );
	}

	//GL_CheckErrors();
}

/*
================================================================================================
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders()
{
	Unbind();

	for( int i = 0; i < shaders.Num(); i++ )
	{
		if( shaders[i].progId != INVALID_PROGID )
		{
			glDeleteShader( shaders[i].progId );
			shaders[i].progId = INVALID_PROGID;
		}
	}

	for( int i = 0; i < renderProgs.Num(); ++i )
	{
		if( renderProgs[i].progId != INVALID_PROGID )
		{
			glDeleteProgram( renderProgs[i].progId );
			renderProgs[i].progId = INVALID_PROGID;
		}
	}
}

/*
====================
idRenderBackend::ResizeImages
====================
*/
void idRenderBackend::ResizeImages()
{
	// TODO resize framebuffers here
}
