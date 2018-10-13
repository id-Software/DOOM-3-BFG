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
idRenderProgManager::CommitUnforms
================================================================================================
*/
void idRenderProgManager::CommitUniforms( uint64 stateBits )
{
#if !defined(USE_VULKAN)
	const int progID = GetGLSLCurrentProgram();
	const renderProg_t& prog = glslPrograms[progID];
	
	//GL_CheckErrors();
	
	ALIGNTYPE16 idVec4 localVectors[RENDERPARM_TOTAL];
	
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
	
	//GL_CheckErrors();
#endif
}




