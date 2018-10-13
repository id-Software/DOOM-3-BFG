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

#pragma hdrstop
#include "precompiled.h"

#include "../RenderCommon.h"


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




