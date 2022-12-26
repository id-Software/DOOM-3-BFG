/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2022 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

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
#include "nvrhi/common/shader-blob.h"
#include <sys/DeviceManager.h>


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
	if( currentIndex == index )
	{
		return;
	}

	currentIndex = index;
}

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind()
{
	currentIndex = -1;
}

/*
================================================================================================
idRenderProgManager::LoadShader
================================================================================================
*/
void idRenderProgManager::LoadShader( int index, rpStage_t stage )
{
	if( shaders[index].handle )
	{
		return; // Already loaded
	}

	LoadShader( shaders[index] );
}

extern DeviceManager* deviceManager;

/*
================================================================================================
idRenderProgManager::LoadGLSLShader
================================================================================================
*/
void idRenderProgManager::LoadShader( shader_t& shader )
{
	idStr stage;
	nvrhi::ShaderType shaderType{};

	if( shader.stage == SHADER_STAGE_VERTEX )
	{
		stage = "vs";
		shaderType = nvrhi::ShaderType::Vertex;
	}
	else if( shader.stage == SHADER_STAGE_FRAGMENT )
	{
		stage = "ps";
		shaderType = nvrhi::ShaderType::Pixel;
	}
	else if( shader.stage == SHADER_STAGE_COMPUTE )
	{
		stage = "cs";
		shaderType = nvrhi::ShaderType::Compute;
	}

	idStr adjustedName = shader.name;
	adjustedName.StripFileExtension();
	if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::D3D12 )
	{
		adjustedName = idStr( "renderprogs2/dxil/" ) + adjustedName + "." + stage + ".bin";
	}
	else if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN )
	{
		adjustedName = idStr( "renderprogs2/spirv/" ) + adjustedName + "." + stage + ".bin";
	}
	else
	{
		common->FatalError( "Unsupported graphics api" );
	}

	ShaderBlob shaderBlob = GetBytecode( adjustedName );

	if( !shaderBlob.data )
	{
		return;
	}

	idList<nvrhi::ShaderConstant> constants;

	for( int i = 0; i < shader.macros.Num(); i++ )
	{
		constants.Append( nvrhi::ShaderConstant
		{
			shader.macros[i].name.c_str(),
			shader.macros[i].definition.c_str()
		} );
	}

	nvrhi::ShaderDesc desc = nvrhi::ShaderDesc( shaderType );
	desc.debugName = ( idStr( shader.name ) + idStr( shader.nameOutSuffix ) ).c_str();

	nvrhi::ShaderDesc descCopy = desc;
	// TODO(Stephen): Might not want to hard-code this.
	descCopy.entryName = "main";

	nvrhi::ShaderConstant* shaderConstant( nullptr );

	nvrhi::ShaderHandle shaderHandle = nvrhi::createShaderPermutation( device, descCopy, shaderBlob.data, shaderBlob.size,
									   ( constants.Num() > 0 ) ? &constants[0] : shaderConstant, uint32_t( constants.Num() ) );

	shader.handle = shaderHandle;
}

/*
================================================================================================
idRenderProgManager::GetBytecode
================================================================================================
*/
ShaderBlob idRenderProgManager::GetBytecode( const char* fileName )
{
	ShaderBlob blob;

	blob.size = fileSystem->ReadFile( fileName, &blob.data );

	if( !blob.data )
	{
		common->FatalError( "Couldn't read the binary file for shader %s", fileName );
	}

	return blob;
}

/*
================================================================================================
idRenderProgManager::LoadGLSLProgram
================================================================================================
*/
void idRenderProgManager::LoadProgram( const int programIndex, const int vertexShaderIndex, const int fragmentShaderIndex )
{
	renderProg_t& prog = renderProgs[programIndex];
	prog.fragmentShaderIndex = fragmentShaderIndex;
	prog.vertexShaderIndex = vertexShaderIndex;
	if( prog.vertexLayout != LAYOUT_UNKNOWN )
	{
		prog.inputLayout = device->createInputLayout(
							   &vertexLayoutDescs[prog.vertexLayout][0],
							   vertexLayoutDescs[prog.vertexLayout].Num(),
							   shaders[prog.vertexShaderIndex].handle );
	}
	prog.bindingLayouts = bindingLayouts[prog.bindingLayoutType];
}

/*
================================================================================================
idRenderProgManager::LoadComputeProgram
================================================================================================
*/
void idRenderProgManager::LoadComputeProgram( const int programIndex, const int computeShaderIndex )
{
	renderProg_t& prog = renderProgs[programIndex];
	prog.computeShaderIndex = computeShaderIndex;
	if( prog.vertexLayout != LAYOUT_UNKNOWN )
	{
		prog.inputLayout = device->createInputLayout(
							   &vertexLayoutDescs[prog.vertexLayout][0],
							   vertexLayoutDescs[prog.vertexLayout].Num(),
							   shaders[prog.vertexShaderIndex].handle );
	}
	prog.bindingLayouts = bindingLayouts[prog.bindingLayoutType];
}


/*
================================================================================================
idRenderProgManager::FindProgram
================================================================================================
*/
int	 idRenderProgManager::FindProgram( const char* name, int vIndex, int fIndex, bindingLayoutType_t bindingType )
{
	for( int i = 0; i < renderProgs.Num(); ++i )
	{
		if( ( renderProgs[i].vertexShaderIndex == vIndex ) && ( renderProgs[i].fragmentShaderIndex == fIndex ) )
		{
			return i;
		}
	}

	renderProg_t program;
	program.name = name;
	program.vertexLayout = LAYOUT_DRAW_VERT;
	program.bindingLayoutType = bindingType;
	int index = renderProgs.Append( program );
	LoadProgram( index, vIndex, fIndex );
	return index;
}

int idRenderProgManager::UniformSize()
{
	return uniforms.Allocated();
}

/*
================================================================================================
idRenderProgManager::CommitUnforms
================================================================================================
*/
void idRenderProgManager::CommitUniforms( uint64 stateBits )
{
}

/*
================================================================================================
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders()
{
	Unbind();

	tr.backend.ResetPipelineCache();

	for( int i = 0; i < shaders.Num(); i++ )
	{
		if( shaders[i].handle )
		{
			shaders[i].handle.Reset();
		}
	}
}

/*
================================================================================================
idRenderProgManager::SetUniformValue
================================================================================================
*/
void idRenderProgManager::SetUniformValue( const renderParm_t rp, const float* value )
{
	for( int i = 0; i < 4; i++ )
	{
		uniforms[rp][i] = value[i];
	}

	uniformsChanged = true;
}

/*
================================================================================================
idRenderProgManager::ZeroUniforms
================================================================================================
*/
void idRenderProgManager::ZeroUniforms()
{
	memset( uniforms.Ptr(), 0, uniforms.Allocated() );

	uniformsChanged = true;
}

// Only updates the constant buffer if it was updated at all
bool idRenderProgManager::CommitConstantBuffer( nvrhi::ICommandList* commandList, bool bindingLayoutTypeChanged )
{
	// RB: It would be better to NUM_BINDING_LAYOUTS uniformsChanged entrys but we don't know the current binding layout type when we set the uniforms.
	// The vkDoom3 backend even didn't bother with this and always fired the uniforms for each draw call.
	if( uniformsChanged || bindingLayoutTypeChanged )
	{
		commandList->writeBuffer( constantBuffer[BindingLayoutType()], uniforms.Ptr(), uniforms.Allocated() );

		uniformsChanged = false;

		return true;
	}

	return false;
}