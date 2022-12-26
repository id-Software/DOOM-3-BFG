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
#include "../RenderProgs.h"


void RpPrintState( uint64 stateBits );

struct vertexLayout_t
{
	VkPipelineVertexInputStateCreateInfo inputState;
	idList< VkVertexInputBindingDescription > bindingDesc;
	idList< VkVertexInputAttributeDescription > attributeDesc;
};

idUniformBuffer emptyUBO;

static vertexLayout_t vertexLayouts[ NUM_VERTEX_LAYOUTS ];

static const char* renderProgBindingStrings[ BINDING_TYPE_MAX ] =
{
	"ubo",
	"sampler"
};

/*
=============
CreateVertexDescriptions
=============
*/
void CreateVertexDescriptions()
{
	VkPipelineVertexInputStateCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;

	VkVertexInputBindingDescription binding = {};
	VkVertexInputAttributeDescription attribute = {};

	{
		vertexLayout_t& layout = vertexLayouts[ LAYOUT_DRAW_VERT ];
		layout.inputState = createInfo;

		uint32 locationNo = 0;
		uint32 offset = 0;

		binding.stride = sizeof( idDrawVert );
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		layout.bindingDesc.Append( binding );

		// Position
		attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idDrawVert::xyz );

		// TexCoord
		attribute.format = VK_FORMAT_R16G16_SFLOAT;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idDrawVert::st );

		// Normal
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idDrawVert::normal );

		// Tangent
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idDrawVert::tangent );

		// Color1
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idDrawVert::color );

		// Color2
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
	}

	{
		vertexLayout_t& layout = vertexLayouts[ LAYOUT_DRAW_SHADOW_VERT_SKINNED ];
		layout.inputState = createInfo;

		uint32 locationNo = 0;
		uint32 offset = 0;

		binding.stride = sizeof( idShadowVertSkinned );
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		layout.bindingDesc.Append( binding );

		// Position
		attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idShadowVertSkinned::xyzw );

		// Color1
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idShadowVertSkinned::color );

		// Color2
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
	}

	{
		vertexLayout_t& layout = vertexLayouts[ LAYOUT_DRAW_SHADOW_VERT ];
		layout.inputState = createInfo;

		binding.stride = sizeof( idShadowVert );
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		layout.bindingDesc.Append( binding );

		// Position
		attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attribute.location = 0;
		attribute.offset = 0;
		layout.attributeDesc.Append( attribute );
	}
}

/*
========================
CreateDescriptorPools
========================
*/
void CreateDescriptorPools( VkDescriptorPool( &pools )[ NUM_FRAME_DATA ] )
{
	const int numPools = 2;
	VkDescriptorPoolSize poolSizes[ numPools ];
	poolSizes[ 0 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[ 0 ].descriptorCount = MAX_DESC_UNIFORM_BUFFERS;

	poolSizes[ 1 ].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[ 1 ].descriptorCount = MAX_DESC_IMAGE_SAMPLERS;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.pNext = NULL;
	poolCreateInfo.maxSets = MAX_DESC_SETS;
	poolCreateInfo.poolSizeCount = numPools;
	poolCreateInfo.pPoolSizes = poolSizes;

	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		ID_VK_CHECK( vkCreateDescriptorPool( vkcontext.device, &poolCreateInfo, NULL, &pools[ i ] ) );
	}
}

/*
========================
GetDescriptorType
========================
*/
static VkDescriptorType GetDescriptorType( rpBinding_t type )
{
	switch( type )
	{
		case BINDING_TYPE_UNIFORM_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		case BINDING_TYPE_SAMPLER:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		default:
			idLib::Error( "Unknown rpBinding_t %d", static_cast< int >( type ) );
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

/*
========================
CreateDescriptorSetLayout
========================
*/
void idRenderProgManager::CreateDescriptorSetLayout( const shader_t& vertexShader, const shader_t& fragmentShader, renderProg_t& renderProg )
{
	// Descriptor Set Layout
	{
		idList< VkDescriptorSetLayoutBinding > layoutBindings;
		VkDescriptorSetLayoutBinding binding = {};
		binding.descriptorCount = 1;

		uint32 bindingId = 0;

		binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		for( int i = 0; i < vertexShader.bindings.Num(); ++i )
		{
			binding.binding = bindingId++;
			binding.descriptorType = GetDescriptorType( vertexShader.bindings[ i ] );
			renderProg.bindings.Append( vertexShader.bindings[ i ] );

			layoutBindings.Append( binding );
		}

		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		for( int i = 0; i < fragmentShader.bindings.Num(); ++i )
		{
			binding.binding = bindingId++;
			binding.descriptorType = GetDescriptorType( fragmentShader.bindings[ i ] );
			renderProg.bindings.Append( fragmentShader.bindings[ i ] );

			layoutBindings.Append( binding );
		}

		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = layoutBindings.Num();
		createInfo.pBindings = layoutBindings.Ptr();

		vkCreateDescriptorSetLayout( vkcontext.device, &createInfo, NULL, &renderProg.descriptorSetLayout );
	}

	// Pipeline Layout
	{
		VkPipelineLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = 1;
		createInfo.pSetLayouts = &renderProg.descriptorSetLayout;

		vkCreatePipelineLayout( vkcontext.device, &createInfo, NULL, &renderProg.pipelineLayout );
	}
}

/*
========================
idRenderProgManager::StartFrame
========================
*/
void idRenderProgManager::StartFrame()
{
	counter++;
	currentData = counter % NUM_FRAME_DATA;
	currentDescSet = 0;
	currentParmBufferOffset = 0;

	vkResetDescriptorPool( vkcontext.device, descriptorPools[ currentData ], 0 );
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

	RENDERLOG_PRINTF( "Binding SPIR-V Program %s\n", renderProgs[ index ].name.c_str() );
}

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind()
{
	current = -1;
}

/*
================================================================================================
idRenderProgManager::LoadShader
================================================================================================
*/
void idRenderProgManager::LoadShader( int index, rpStage_t stage )
{
	if( shaders[ index ].module != VK_NULL_HANDLE )
	{
		return; // Already loaded
	}

	LoadShader( shaders[index] );
}


/*
================================================================================================
CompileGLSLtoSPIRV
================================================================================================
*/


#if defined(SPIRV_SHADERC)

#include <shaderc/shaderc.hpp>

static int CompileGLSLtoSPIRV( const char* filename, const idStr& dataGLSL, const rpStage_t stage, uint32** spirvBuffer )
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	// Like -DMY_DEFINE=1
	//options.AddMacroDefinition("MY_DEFINE", "1");

	//if (optimize)
	{
		options.SetOptimizationLevel( shaderc_optimization_level_size );
	}

	shaderc_shader_kind shaderKind;
	if( stage == SHADER_STAGE_VERTEX )
	{
		shaderKind = shaderc_glsl_vertex_shader;
	}
	else if( stage == SHADER_STAGE_COMPUTE )
	{
		shaderKind = shaderc_glsl_compute_shader;
	}
	else
	{
		shaderKind = shaderc_glsl_fragment_shader;
	}

	std::string source = dataGLSL.c_str();

	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv( source, shaderKind, filename, options );

	if( module.GetCompilationStatus() != shaderc_compilation_status_success )
	{
		idLib::Printf( "Comping GLSL to SPIR-V using shaderc failed for: %s\n", filename );
		idLib::Printf( "%s\n", module.GetErrorMessage().c_str() );
		return 0;
	}

	std::vector<uint32_t> spirV = { module.cbegin(), module.cend() };

	// copy to spirvBuffer
	int32 spirvLen = spirV.size() * sizeof( uint32_t );

	void* buffer = Mem_Alloc( spirvLen, TAG_RENDERPROG );
	memcpy( buffer, spirV.data(), spirvLen );

	*spirvBuffer = ( uint32_t* ) buffer;
	return spirvLen;


}
#else

//#include <glslang/Public/ShaderLang.h>
//#include <glslang/Include/ResourceLimits.h>
//#include <SPIRV/GlslangToSpv.h>
//#include <glslang/StandAlone/DirStackFileIncluder.h>

#include "../../extern/glslang/glslang/Public/ShaderLang.h"
#include "../../extern/glslang/glslang/Include/ResourceLimits.h"
#include "../../extern/glslang/SPIRV/GlslangToSpv.h"

static bool glslangInitialized = false;

static int CompileGLSLtoSPIRV( const char* filename, const idStr& dataGLSL, const rpStage_t stage, uint32** spirvBuffer )
{
	if( !glslangInitialized )
	{
		glslang::InitializeProcess();
		glslangInitialized = true;
	}

	const char* inputCString = dataGLSL.c_str();

	EShLanguage shaderType;
	if( stage == SHADER_STAGE_VERTEX )
	{
		shaderType = EShLangVertex;
	}
	else if( stage == SHADER_STAGE_COMPUTE )
	{
		shaderType = EShLangCompute;
	}
	else
	{
		shaderType = EShLangFragment;
	}

	glslang::TShader* shader = new glslang::TShader( shaderType );
	shader->setStrings( &inputCString, 1 );

	int clientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100
	glslang::EShTargetClientVersion vulkanClientVersion = glslang::EShTargetVulkan_1_0;
	glslang::EShTargetLanguageVersion targetVersion = glslang::EShTargetSpv_1_0;

	shader->setEnvInput( glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, clientInputSemanticsVersion );
	shader->setEnvClient( glslang::EShClientVulkan, vulkanClientVersion );
	shader->setEnvTarget( glslang::EShTargetSpv, targetVersion );

	// RB: see RBDOOM-3-BFG\neo\extern\glslang\StandAlone\ResourceLimits.cpp
	static TBuiltInResource resources;
	resources.maxLights = 32;
	resources.maxClipPlanes = 6;
	resources.maxTextureUnits = 32;
	resources.maxTextureCoords = 32;
	resources.maxVertexAttribs = 64;
	resources.maxVertexUniformComponents = 4096;
	resources.maxVaryingFloats = 64;
	resources.maxVertexTextureImageUnits = 32;
	resources.maxCombinedTextureImageUnits = 80;
	resources.maxTextureImageUnits = 32;
	resources.maxFragmentUniformComponents = 4096;
	resources.maxDrawBuffers = 32;
	resources.maxVertexUniformVectors = 128;
	resources.maxVaryingVectors = 8;
	resources.maxFragmentUniformVectors = 16;
	resources.maxVertexOutputVectors = 16;
	resources.maxFragmentInputVectors = 15;
	resources.minProgramTexelOffset = -8;
	resources.maxProgramTexelOffset = 7;
	resources.maxClipDistances = 8;
	resources.maxComputeWorkGroupCountX = 65535;
	resources.maxComputeWorkGroupCountY = 65535;
	resources.maxComputeWorkGroupCountZ = 65535;
	resources.maxComputeWorkGroupSizeX = 1024;
	resources.maxComputeWorkGroupSizeY = 1024;
	resources.maxComputeWorkGroupSizeZ = 64;
	resources.maxComputeUniformComponents = 1024;
	resources.maxComputeTextureImageUnits = 16;
	resources.maxComputeImageUniforms = 8;
	resources.maxComputeAtomicCounters = 8;
	resources.maxComputeAtomicCounterBuffers = 1;
	resources.maxVaryingComponents = 60;
	resources.maxVertexOutputComponents = 64;
	resources.maxGeometryInputComponents = 64;
	resources.maxGeometryOutputComponents = 128;
	resources.maxFragmentInputComponents = 128;
	resources.maxImageUnits = 8;
	resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	resources.maxCombinedShaderOutputResources = 8;
	resources.maxImageSamples = 0;
	resources.maxVertexImageUniforms = 0;
	resources.maxTessControlImageUniforms = 0;
	resources.maxTessEvaluationImageUniforms = 0;
	resources.maxGeometryImageUniforms = 0;
	resources.maxFragmentImageUniforms = 8;
	resources.maxCombinedImageUniforms = 8;
	resources.maxGeometryTextureImageUnits = 16;
	resources.maxGeometryOutputVertices = 256;
	resources.maxGeometryTotalOutputComponents = 1024;
	resources.maxGeometryUniformComponents = 1024;
	resources.maxGeometryVaryingComponents = 64;
	resources.maxTessControlInputComponents = 128;
	resources.maxTessControlOutputComponents = 128;
	resources.maxTessControlTextureImageUnits = 16;
	resources.maxTessControlUniformComponents = 1024;
	resources.maxTessControlTotalOutputComponents = 4096;
	resources.maxTessEvaluationInputComponents = 128;
	resources.maxTessEvaluationOutputComponents = 128;
	resources.maxTessEvaluationTextureImageUnits = 16;
	resources.maxTessEvaluationUniformComponents = 1024;
	resources.maxTessPatchComponents = 120;
	resources.maxPatchVertices = 32;
	resources.maxTessGenLevel = 64;
	resources.maxViewports = 16;
	resources.maxVertexAtomicCounters = 0;
	resources.maxTessControlAtomicCounters = 0;
	resources.maxTessEvaluationAtomicCounters = 0;
	resources.maxGeometryAtomicCounters = 0;
	resources.maxFragmentAtomicCounters = 8;
	resources.maxCombinedAtomicCounters = 8;
	resources.maxAtomicCounterBindings = 1;
	resources.maxVertexAtomicCounterBuffers = 0;
	resources.maxTessControlAtomicCounterBuffers = 0;
	resources.maxTessEvaluationAtomicCounterBuffers = 0;
	resources.maxGeometryAtomicCounterBuffers = 0;
	resources.maxFragmentAtomicCounterBuffers = 1;
	resources.maxCombinedAtomicCounterBuffers = 1;
	resources.maxAtomicCounterBufferSize = 16384;
	resources.maxTransformFeedbackBuffers = 4;
	resources.maxTransformFeedbackInterleavedComponents = 64;
	resources.maxCullDistances = 8;
	resources.maxCombinedClipAndCullDistances = 8;
	resources.maxSamples = 4;
	resources.maxMeshOutputVerticesNV = 256;
	resources.maxMeshOutputPrimitivesNV = 512;
	resources.maxMeshWorkGroupSizeX_NV = 32;
	resources.maxMeshWorkGroupSizeY_NV = 1;
	resources.maxMeshWorkGroupSizeZ_NV = 1;
	resources.maxTaskWorkGroupSizeX_NV = 32;
	resources.maxTaskWorkGroupSizeY_NV = 1;
	resources.maxTaskWorkGroupSizeZ_NV = 1;
	resources.maxMeshViewCountNV = 4;

	resources.limits.nonInductiveForLoops = true;
	resources.limits.whileLoops = true;
	resources.limits.doWhileLoops = true;
	resources.limits.generalUniformIndexing = true;
	resources.limits.generalAttributeMatrixVectorIndexing = true;
	resources.limits.generalVaryingIndexing = true;
	resources.limits.generalSamplerIndexing = true;
	resources.limits.generalVariableIndexing = true;
	resources.limits.generalConstantMatrixVectorIndexing = true;

	EShMessages messages = ( EShMessages )( EShMsgSpvRules | EShMsgVulkanRules );

	const int defaultVersion = 100;

	if( !shader->parse( &resources, 100, false, messages ) )
	{
		idLib::Printf( "GLSL parsing failed for: %s\n", filename );
		idLib::Printf( "%s\n", shader->getInfoLog() );
		idLib::Printf( "%s\n", shader->getInfoDebugLog() );

		//*spirvBuffer = NULL;
		//return 0;
	}

	glslang::TProgram* program = new glslang::TProgram();
	program->addShader( shader );

	if( !program->link( messages ) )
	{
		idLib::Printf( "GLSL linking failed for: %s\n", filename );
		idLib::Printf( "%s\n", shader->getInfoLog() );
		idLib::Printf( "%s\n", shader->getInfoDebugLog() );
	}

	// All that's left to do now is to convert the program's intermediate representation into SpirV:
	std::vector<unsigned int>	spirV;
	spv::SpvBuildLogger			logger;
	glslang::SpvOptions			spvOptions;

	glslang::GlslangToSpv( *program->getIntermediate( shaderType ), spirV, &logger, &spvOptions );

	int32 spirvLen = spirV.size() * sizeof( unsigned int );

	void* buffer = Mem_Alloc( spirvLen, TAG_RENDERPROG );
	memcpy( buffer, spirV.data(), spirvLen );

	*spirvBuffer = ( uint32* ) buffer;
	delete program;
	delete shader;
	return spirvLen;
}

#endif

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
	idStr outFileSPIRV;
	idStr outFileLayout;

	// RB: replaced backslashes
	inFile.Format( "renderprogs/%s", shader.name.c_str() );
	inFile.StripFileExtension();
	outFileHLSL.Format( "renderprogs/hlsl/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
	outFileHLSL.StripFileExtension();

	outFileGLSL.Format( "renderprogs/vkglsl/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
	outFileLayout.Format( "renderprogs/vkglsl/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );

	outFileGLSL.StripFileExtension();
	outFileLayout.StripFileExtension();

	if( shader.stage == SHADER_STAGE_FRAGMENT )
	{
		inFile += ".ps.hlsl";
		outFileHLSL += ".ps.hlsl";
		outFileGLSL += ".frag";
		outFileLayout += ".frag.layout";
		outFileSPIRV += ".fspv";
	}
	else
	{
		inFile += ".vs.hlsl";
		outFileHLSL += ".vs.hlsl";
		outFileGLSL += ".vert";
		outFileLayout += ".vert.layout";
		outFileSPIRV += ".vspv";
	}

	// first check whether we already have a valid GLSL file and compare it to the hlsl timestamp;
	ID_TIME_T hlslTimeStamp;
	int hlslFileLength = fileSystem->ReadFile( inFile.c_str(), NULL, &hlslTimeStamp );

	ID_TIME_T glslTimeStamp;
	int glslFileLength = fileSystem->ReadFile( outFileGLSL.c_str(), NULL, &glslTimeStamp );

	// if the glsl file doesn't exist or we have a newer HLSL file we need to recreate the glsl file.
	idStr programGLSL;
	idStr programLayout;
	//if( ( glslFileLength <= 0 ) || ( hlslTimeStamp != FILE_NOT_FOUND_TIMESTAMP && hlslTimeStamp > glslTimeStamp ) || r_alwaysExportGLSL.GetBool() )
	{
		const char* hlslFileBuffer = NULL;
		int len = 0;

		if( hlslFileLength <= 0 )
		{
			hlslFileBuffer = FindEmbeddedSourceShader( inFile.c_str() );
			if( hlslFileBuffer == NULL )
			{
				// hlsl file doesn't even exist bail out
				idLib::Error( "HLSL file %s could not be loaded and may be corrupt", inFile.c_str() );
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
		programGLSL = ConvertCG2GLSL( programHLSL, inFile, shader.stage, programLayout, true, hasGPUSkinning, shader.vertexLayout );

		fileSystem->WriteFile( outFileHLSL, programHLSL.c_str(), programHLSL.Length(), "fs_savepath" );
		fileSystem->WriteFile( outFileGLSL, programGLSL.c_str(), programGLSL.Length(), "fs_savepath" );
		fileSystem->WriteFile( outFileLayout, programLayout.c_str(), programLayout.Length(), "fs_savepath" );
	}
	/*
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
	*/

	// RB: find the uniforms locations in either the vertex or fragment uniform array
	// this uses the new layout structure
	{
		idLexer src( programLayout, programLayout.Length(), "layout" );
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
				shader.parmIndices.Append( index );
			}
		}

		if( src.ExpectTokenString( "bindings" ) )
		{
			src.ExpectTokenString( "[" );

			while( !src.CheckTokenString( "]" ) )
			{
				src.ReadToken( &token );

				int index = -1;
				for( int i = 0; i < BINDING_TYPE_MAX; ++i )
				{
					if( token == renderProgBindingStrings[ i ] )
					{
						index = i;
					}
				}

				if( index == -1 )
				{
					idLib::Error( "Invalid binding %s", token.c_str() );
				}

				shader.bindings.Append( static_cast< rpBinding_t >( index ) );
			}
		}
	}

	// TODO GLSL to SPIR-V compilation
	uint32* spirvBuffer = NULL;
	int spirvLen = CompileGLSLtoSPIRV( outFileGLSL.c_str(), programGLSL, shader.stage, &spirvBuffer );

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = spirvLen;
	shaderModuleCreateInfo.pCode = ( uint32* )spirvBuffer;

	ID_VK_CHECK( vkCreateShaderModule( vkcontext.device, &shaderModuleCreateInfo, NULL, &shader.module ) );

	Mem_Free( spirvBuffer );
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

	idStr programName = shaders[ vertexShaderIndex ].name;
	programName.StripFileExtension();
	prog.name = programName;
	//prog.progId = programIndex;
	prog.fragmentShaderIndex = fragmentShaderIndex;
	prog.vertexShaderIndex = vertexShaderIndex;

	CreateDescriptorSetLayout( shaders[ vertexShaderIndex ], shaders[ fragmentShaderIndex ], prog );


	// TODO

#if 1
	// RB: removed idStr::Icmp( name, "heatHaze.vfp" ) == 0  hack
	for( int i = 0; i < shaders[vertexShaderIndex].parmIndices.Num(); i++ )
	{
		if( shaders[vertexShaderIndex].parmIndices[i] == RENDERPARM_ENABLE_SKINNING )
		{
			prog.usesJoints = true;
			prog.optionalSkinning = true;
		}
	}
#endif
}

/*
================================================================================================
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders()
{
	Unbind();

	// destroy shaders
	for( int i = 0; i < shaders.Num(); ++i )
	{
		shader_t& shader = shaders[ i ];
		vkDestroyShaderModule( vkcontext.device, shader.module, NULL );
		shader.module = VK_NULL_HANDLE;
	}

	// destroy pipelines
	for( int i = 0; i < renderProgs.Num(); ++i )
	{
		renderProg_t& prog = renderProgs[ i ];

		for( int j = 0; j < prog.pipelines.Num(); ++j )
		{
			vkDestroyPipeline( vkcontext.device, prog.pipelines[ j ].pipeline, NULL );
		}
		prog.pipelines.Clear();

		vkDestroyDescriptorSetLayout( vkcontext.device, prog.descriptorSetLayout, NULL );
		vkDestroyPipelineLayout( vkcontext.device, prog.pipelineLayout, NULL );
	}
	renderProgs.Clear();

	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		parmBuffers[ i ]->FreeBufferObject();
		delete parmBuffers[ i ];
		parmBuffers[ i ] = NULL;
	}

	emptyUBO.FreeBufferObject();

	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		//vkFreeDescriptorSets( vkcontext.device, descriptorPools[ i ], MAX_DESC_SETS, descriptorSets[ i ] );
		vkResetDescriptorPool( vkcontext.device, descriptorPools[ i ], 0 );
		vkDestroyDescriptorPool( vkcontext.device, descriptorPools[ i ], NULL );
	}

	memset( descriptorSets, 0, sizeof( descriptorSets ) );
	memset( descriptorPools, 0, sizeof( descriptorPools ) );

	counter = 0;
	currentData = 0;
	currentDescSet = 0;
}

/*
========================
idRenderProgManager::AllocParmBlockBuffer
========================
*/
void idRenderProgManager::AllocParmBlockBuffer( const idList<int>& parmIndices, idUniformBuffer& ubo )
{
	// TODO support shadow matrices + 23 float4

	const int numParms = parmIndices.Num();
	const int bytes = ALIGN( numParms * sizeof( idVec4 ), vkcontext.gpu->props.limits.minUniformBufferOffsetAlignment );

	ubo.Reference( *parmBuffers[ currentData ], currentParmBufferOffset, bytes );

	idVec4* gpuUniforms = ( idVec4* )ubo.MapBuffer( BM_WRITE );

	for( int i = 0; i < numParms; ++i )
	{
		gpuUniforms[ i ] = uniforms[ static_cast< renderParm_t >( parmIndices[ i ] ) ];
	}

	ubo.UnmapBuffer();

	currentParmBufferOffset += bytes;
}

/*
========================
GetStencilOpState
========================
*/
static VkStencilOpState GetStencilOpState( uint64 stencilBits )
{
	VkStencilOpState state = {};

	switch( stencilBits & GLS_STENCIL_OP_FAIL_BITS )
	{
		case GLS_STENCIL_OP_FAIL_KEEP:
			state.failOp = VK_STENCIL_OP_KEEP;
			break;
		case GLS_STENCIL_OP_FAIL_ZERO:
			state.failOp = VK_STENCIL_OP_ZERO;
			break;
		case GLS_STENCIL_OP_FAIL_REPLACE:
			state.failOp = VK_STENCIL_OP_REPLACE;
			break;
		case GLS_STENCIL_OP_FAIL_INCR:
			state.failOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			break;
		case GLS_STENCIL_OP_FAIL_DECR:
			state.failOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			break;
		case GLS_STENCIL_OP_FAIL_INVERT:
			state.failOp = VK_STENCIL_OP_INVERT;
			break;
		case GLS_STENCIL_OP_FAIL_INCR_WRAP:
			state.failOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
			break;
		case GLS_STENCIL_OP_FAIL_DECR_WRAP:
			state.failOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
			break;
	}
	switch( stencilBits & GLS_STENCIL_OP_ZFAIL_BITS )
	{
		case GLS_STENCIL_OP_ZFAIL_KEEP:
			state.depthFailOp = VK_STENCIL_OP_KEEP;
			break;
		case GLS_STENCIL_OP_ZFAIL_ZERO:
			state.depthFailOp = VK_STENCIL_OP_ZERO;
			break;
		case GLS_STENCIL_OP_ZFAIL_REPLACE:
			state.depthFailOp = VK_STENCIL_OP_REPLACE;
			break;
		case GLS_STENCIL_OP_ZFAIL_INCR:
			state.depthFailOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			break;
		case GLS_STENCIL_OP_ZFAIL_DECR:
			state.depthFailOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			break;
		case GLS_STENCIL_OP_ZFAIL_INVERT:
			state.depthFailOp = VK_STENCIL_OP_INVERT;
			break;
		case GLS_STENCIL_OP_ZFAIL_INCR_WRAP:
			state.depthFailOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
			break;
		case GLS_STENCIL_OP_ZFAIL_DECR_WRAP:
			state.depthFailOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
			break;
	}
	switch( stencilBits & GLS_STENCIL_OP_PASS_BITS )
	{
		case GLS_STENCIL_OP_PASS_KEEP:
			state.passOp = VK_STENCIL_OP_KEEP;
			break;
		case GLS_STENCIL_OP_PASS_ZERO:
			state.passOp = VK_STENCIL_OP_ZERO;
			break;
		case GLS_STENCIL_OP_PASS_REPLACE:
			state.passOp = VK_STENCIL_OP_REPLACE;
			break;
		case GLS_STENCIL_OP_PASS_INCR:
			state.passOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			break;
		case GLS_STENCIL_OP_PASS_DECR:
			state.passOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			break;
		case GLS_STENCIL_OP_PASS_INVERT:
			state.passOp = VK_STENCIL_OP_INVERT;
			break;
		case GLS_STENCIL_OP_PASS_INCR_WRAP:
			state.passOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
			break;
		case GLS_STENCIL_OP_PASS_DECR_WRAP:
			state.passOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
			break;
	}

	return state;
}

/*
========================
CreateGraphicsPipeline
========================
*/
static VkPipeline CreateGraphicsPipeline(
	vertexLayoutType_t vertexLayoutType,
	VkShaderModule vertexShader,
	VkShaderModule fragmentShader,
	VkPipelineLayout pipelineLayout,
	uint64 stateBits )
{

	// Pipeline
	vertexLayout_t& vertexLayout = vertexLayouts[ vertexLayoutType ];

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInputState = vertexLayout.inputState;
	vertexInputState.vertexBindingDescriptionCount = vertexLayout.bindingDesc.Num();
	vertexInputState.pVertexBindingDescriptions = vertexLayout.bindingDesc.Ptr();
	vertexInputState.vertexAttributeDescriptionCount = vertexLayout.attributeDesc.Num();
	vertexInputState.pVertexAttributeDescriptions = vertexLayout.attributeDesc.Ptr();

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo assemblyInputState = {};
	assemblyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyInputState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = ( stateBits & GLS_POLYGON_OFFSET ) != 0;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.frontFace = ( stateBits & GLS_CLOCKWISE ) ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.lineWidth = 1.0f;
	rasterizationState.polygonMode = ( stateBits & GLS_POLYMODE_LINE ) ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;

	switch( stateBits & GLS_CULL_BITS )
	{
		case GLS_CULL_TWOSIDED:
			rasterizationState.cullMode = VK_CULL_MODE_NONE;
			break;

		case GLS_CULL_BACKSIDED:
			if( stateBits & GLS_MIRROR_VIEW )
			{
				rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
			}
			else
			{
				rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
			}
			break;

		case GLS_CULL_FRONTSIDED:
		default:
			if( stateBits & GLS_MIRROR_VIEW )
			{
				rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
			}
			else
			{
				rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
			}
			break;
	}

	// Color Blend Attachment
	VkPipelineColorBlendAttachmentState attachmentState = {};
	{
		VkBlendFactor srcFactor = VK_BLEND_FACTOR_ONE;
		switch( stateBits & GLS_SRCBLEND_BITS )
		{
			case GLS_SRCBLEND_ZERO:
				srcFactor = VK_BLEND_FACTOR_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = VK_BLEND_FACTOR_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = VK_BLEND_FACTOR_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = VK_BLEND_FACTOR_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
				break;
		}

		VkBlendFactor dstFactor = VK_BLEND_FACTOR_ZERO;
		switch( stateBits & GLS_DSTBLEND_BITS )
		{
			case GLS_DSTBLEND_ZERO:
				dstFactor = VK_BLEND_FACTOR_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = VK_BLEND_FACTOR_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = VK_BLEND_FACTOR_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = VK_BLEND_FACTOR_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
				break;
		}

		VkBlendOp blendOp = VK_BLEND_OP_ADD;
		switch( stateBits & GLS_BLENDOP_BITS )
		{
			case GLS_BLENDOP_MIN:
				blendOp = VK_BLEND_OP_MIN;
				break;
			case GLS_BLENDOP_MAX:
				blendOp = VK_BLEND_OP_MAX;
				break;
			case GLS_BLENDOP_ADD:
				blendOp = VK_BLEND_OP_ADD;
				break;
			case GLS_BLENDOP_SUB:
				blendOp = VK_BLEND_OP_SUBTRACT;
				break;
		}

		attachmentState.blendEnable = ( srcFactor != VK_BLEND_FACTOR_ONE || dstFactor != VK_BLEND_FACTOR_ZERO );
		attachmentState.colorBlendOp = blendOp;
		attachmentState.srcColorBlendFactor = srcFactor;
		attachmentState.dstColorBlendFactor = dstFactor;
		attachmentState.alphaBlendOp = blendOp;
		attachmentState.srcAlphaBlendFactor = srcFactor;
		attachmentState.dstAlphaBlendFactor = dstFactor;

		// Color Mask
		attachmentState.colorWriteMask = 0;
		attachmentState.colorWriteMask |= ( stateBits & GLS_REDMASK ) ?	0 : VK_COLOR_COMPONENT_R_BIT;
		attachmentState.colorWriteMask |= ( stateBits & GLS_GREENMASK ) ? 0 : VK_COLOR_COMPONENT_G_BIT;
		attachmentState.colorWriteMask |= ( stateBits & GLS_BLUEMASK ) ? 0 : VK_COLOR_COMPONENT_B_BIT;
		attachmentState.colorWriteMask |= ( stateBits & GLS_ALPHAMASK ) ? 0 : VK_COLOR_COMPONENT_A_BIT;
	}

	// Color Blend
	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &attachmentState;

	// Depth / Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	{
		VkCompareOp depthCompareOp = VK_COMPARE_OP_ALWAYS;
		switch( stateBits & GLS_DEPTHFUNC_BITS )
		{
			case GLS_DEPTHFUNC_EQUAL:
				depthCompareOp = VK_COMPARE_OP_EQUAL;
				break;
			case GLS_DEPTHFUNC_ALWAYS:
				depthCompareOp = VK_COMPARE_OP_ALWAYS;
				break;
			case GLS_DEPTHFUNC_LESS:
				depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
				break;
			case GLS_DEPTHFUNC_GREATER:
				depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
				break;
		}

		VkCompareOp stencilCompareOp = VK_COMPARE_OP_ALWAYS;
		switch( stateBits & GLS_STENCIL_FUNC_BITS )
		{
			case GLS_STENCIL_FUNC_NEVER:
				stencilCompareOp = VK_COMPARE_OP_NEVER;
				break;
			case GLS_STENCIL_FUNC_LESS:
				stencilCompareOp = VK_COMPARE_OP_LESS;
				break;
			case GLS_STENCIL_FUNC_EQUAL:
				stencilCompareOp = VK_COMPARE_OP_EQUAL;
				break;
			case GLS_STENCIL_FUNC_LEQUAL:
				stencilCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
				break;
			case GLS_STENCIL_FUNC_GREATER:
				stencilCompareOp = VK_COMPARE_OP_GREATER;
				break;
			case GLS_STENCIL_FUNC_NOTEQUAL:
				stencilCompareOp = VK_COMPARE_OP_NOT_EQUAL;
				break;
			case GLS_STENCIL_FUNC_GEQUAL:
				stencilCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
				break;
			case GLS_STENCIL_FUNC_ALWAYS:
				stencilCompareOp = VK_COMPARE_OP_ALWAYS;
				break;
		}

		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = ( stateBits & GLS_DEPTHMASK ) == 0;
		depthStencilState.depthCompareOp = depthCompareOp;
		depthStencilState.depthBoundsTestEnable = vkcontext.physicalDeviceFeatures.depthBounds && ( stateBits & GLS_DEPTH_TEST_MASK ) != 0;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = ( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS | GLS_SEPARATE_STENCIL ) ) != 0;

		uint32 ref = uint32( ( stateBits & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT );
		uint32 mask = uint32( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );

		if( stateBits & GLS_SEPARATE_STENCIL )
		{
			depthStencilState.front = GetStencilOpState( stateBits & GLS_STENCIL_FRONT_OPS );
			depthStencilState.front.writeMask = 0xFFFFFFFF;
			depthStencilState.front.compareOp = stencilCompareOp;
			depthStencilState.front.compareMask = mask;
			depthStencilState.front.reference = ref;

			depthStencilState.back = GetStencilOpState( ( stateBits & GLS_STENCIL_BACK_OPS ) >> 12 );
			depthStencilState.back.writeMask = 0xFFFFFFFF;
			depthStencilState.back.compareOp = stencilCompareOp;
			depthStencilState.back.compareMask = mask;
			depthStencilState.back.reference = ref;
		}
		else
		{
			depthStencilState.front = GetStencilOpState( stateBits );
			depthStencilState.front.writeMask = 0xFFFFFFFF;
			depthStencilState.front.compareOp = stencilCompareOp;
			depthStencilState.front.compareMask = mask;
			depthStencilState.front.reference = ref;
			depthStencilState.back = depthStencilState.front;
		}
	}

	// Multisample
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = vkcontext.sampleCount;
	if( vkcontext.supersampling )
	{
		multisampleState.sampleShadingEnable = VK_TRUE;
		multisampleState.minSampleShading = 1.0f;
	}

	// Shader Stages
	idList< VkPipelineShaderStageCreateInfo > stages;
	VkPipelineShaderStageCreateInfo stage = {};
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.pName = "main";

	{
		stage.module = vertexShader;
		stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages.Append( stage );
	}

	if( fragmentShader != VK_NULL_HANDLE )
	{
		stage.module = fragmentShader;
		stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages.Append( stage );
	}

	// Dynamic
	idList< VkDynamicState > dynamic;
	dynamic.Append( VK_DYNAMIC_STATE_SCISSOR );
	dynamic.Append( VK_DYNAMIC_STATE_VIEWPORT );

	//if( stateBits & GLS_POLYGON_OFFSET )
	{
		dynamic.Append( VK_DYNAMIC_STATE_DEPTH_BIAS );
	}

	//if( stateBits & GLS_DEPTH_TEST_MASK )
	{
		dynamic.Append( VK_DYNAMIC_STATE_DEPTH_BOUNDS );
	}

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = dynamic.Num();
	dynamicState.pDynamicStates = dynamic.Ptr();

	// Viewport / Scissor
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Pipeline Create
	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.layout = pipelineLayout;
	createInfo.renderPass = vkcontext.renderPass;
	createInfo.pVertexInputState = &vertexInputState;
	createInfo.pInputAssemblyState = &assemblyInputState;
	createInfo.pRasterizationState = &rasterizationState;
	createInfo.pColorBlendState = &colorBlendState;
	createInfo.pDepthStencilState = &depthStencilState;
	createInfo.pMultisampleState = &multisampleState;
	createInfo.pDynamicState = &dynamicState;
	createInfo.pViewportState = &viewportState;
	createInfo.stageCount = stages.Num();
	createInfo.pStages = stages.Ptr();

	VkPipeline pipeline = VK_NULL_HANDLE;

	ID_VK_CHECK( vkCreateGraphicsPipelines( vkcontext.device, vkcontext.pipelineCache, 1, &createInfo, NULL, &pipeline ) );

	return pipeline;
}

/*
========================
renderProg_t::GetPipeline
========================
*/
VkPipeline idRenderProgManager::renderProg_t::GetPipeline( uint64 stateBits, VkShaderModule vertexShader, VkShaderModule fragmentShader )
{
	for( int i = 0; i < pipelines.Num(); ++i )
	{
		if( stateBits == pipelines[ i ].stateBits )
		{
			return pipelines[ i ].pipeline;
		}
	}

	VkPipeline pipeline = CreateGraphicsPipeline( vertexLayout, vertexShader, fragmentShader, pipelineLayout, stateBits );

	pipelineState_t pipelineState;
	pipelineState.pipeline = pipeline;
	pipelineState.stateBits = stateBits;
	pipelines.Append( pipelineState );

	return pipeline;
}

/*
================================================================================================
idRenderProgManager::CommitUnforms
================================================================================================
*/
void idRenderProgManager::CommitUniforms( uint64 stateBits )
{
#if 0
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

#endif

	renderProg_t& prog = renderProgs[ current ];

	VkPipeline pipeline = prog.GetPipeline( stateBits, shaders[ prog.vertexShaderIndex ].module, shaders[ prog.fragmentShaderIndex ].module );

	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.pNext = NULL;
	setAllocInfo.descriptorPool = descriptorPools[ currentData ];
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &prog.descriptorSetLayout;

	ID_VK_CHECK( vkAllocateDescriptorSets( vkcontext.device, &setAllocInfo, &descriptorSets[ currentData ][ currentDescSet ] ) );

	VkDescriptorSet descSet = descriptorSets[ currentData ][ currentDescSet ];
	currentDescSet++;

	int writeIndex = 0;
	int bufferIndex = 0;
	int	imageIndex = 0;
	int bindingIndex = 0;

	VkWriteDescriptorSet writes[ MAX_DESC_SET_WRITES ];
	VkDescriptorBufferInfo bufferInfos[ MAX_DESC_SET_WRITES ];
	VkDescriptorImageInfo imageInfos[ MAX_DESC_SET_WRITES ];

	int uboIndex = 0;
	idUniformBuffer* ubos[ 3 ] = { NULL, NULL, NULL };

	idUniformBuffer vertParms;
	if( prog.vertexShaderIndex > -1 && shaders[ prog.vertexShaderIndex ].parmIndices.Num() > 0 )
	{
		AllocParmBlockBuffer( shaders[ prog.vertexShaderIndex ].parmIndices, vertParms );

		ubos[ uboIndex++ ] = &vertParms;
	}

	idUniformBuffer jointBuffer;
	if( prog.usesJoints && vkcontext.jointCacheHandle > 0 )
	{
		if( !vertexCache.GetJointBuffer( vkcontext.jointCacheHandle, &jointBuffer ) )
		{
			idLib::Error( "idRenderProgManager::CommitCurrent: jointBuffer == NULL" );
			return;
		}
		assert( ( jointBuffer.GetOffset() & ( vkcontext.gpu->props.limits.minUniformBufferOffsetAlignment - 1 ) ) == 0 );

		ubos[ uboIndex++ ] = &jointBuffer;
	}
	else if( prog.optionalSkinning )
	{
		ubos[ uboIndex++ ] = &emptyUBO;
	}

	idUniformBuffer fragParms;
	if( prog.fragmentShaderIndex > -1 && shaders[ prog.fragmentShaderIndex ].parmIndices.Num() > 0 )
	{
		AllocParmBlockBuffer( shaders[ prog.fragmentShaderIndex ].parmIndices, fragParms );

		ubos[ uboIndex++ ] = &fragParms;
	}

	for( int i = 0; i < prog.bindings.Num(); ++i )
	{
		rpBinding_t binding = prog.bindings[ i ];

		switch( binding )
		{
			case BINDING_TYPE_UNIFORM_BUFFER:
			{
				idUniformBuffer* ubo = ubos[ bufferIndex ];

				VkDescriptorBufferInfo& bufferInfo = bufferInfos[ bufferIndex++ ];
				memset( &bufferInfo, 0, sizeof( VkDescriptorBufferInfo ) );
				bufferInfo.buffer = ubo->GetAPIObject();
				bufferInfo.offset = ubo->GetOffset();
				bufferInfo.range = ubo->GetSize();

				VkWriteDescriptorSet& write = writes[ writeIndex++ ];
				memset( &write, 0, sizeof( VkWriteDescriptorSet ) );
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = descSet;
				write.dstBinding = bindingIndex++;
				write.descriptorCount = 1;
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write.pBufferInfo = &bufferInfo;

				break;
			}
			case BINDING_TYPE_SAMPLER:
			{
				idImage* image = vkcontext.imageParms[ imageIndex ];

				VkDescriptorImageInfo& imageInfo = imageInfos[ imageIndex++ ];
				memset( &imageInfo, 0, sizeof( VkDescriptorImageInfo ) );
				imageInfo.imageLayout = image->GetLayout();
				imageInfo.imageView = image->GetView();
				imageInfo.sampler = image->GetSampler();

				assert( image->GetView() != VK_NULL_HANDLE );

				VkWriteDescriptorSet& write = writes[ writeIndex++ ];
				memset( &write, 0, sizeof( VkWriteDescriptorSet ) );
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = descSet;
				write.dstBinding = bindingIndex++;
				write.descriptorCount = 1;
				write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				write.pImageInfo = &imageInfo;

				//imageIndex++;
				break;
			}
		}
	}

	vkUpdateDescriptorSets( vkcontext.device, writeIndex, writes, 0, NULL );

	vkCmdBindDescriptorSets(
		vkcontext.commandBuffer[ vkcontext.frameParity ],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		prog.pipelineLayout, 0, 1, &descSet,
		0, NULL );

	vkCmdBindPipeline(
		vkcontext.commandBuffer[ vkcontext.frameParity ],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline );
}

void idRenderProgManager::CachePipeline( uint64 stateBits )
{
	renderProg_t& prog = renderProgs[ current ];

	VkPipeline pipeline = prog.GetPipeline( stateBits, shaders[ prog.vertexShaderIndex ].module, shaders[ prog.fragmentShaderIndex ].module );
}


void idRenderProgManager::PrintPipelines()
{
	for( int i = 0; i < renderProgManager.renderProgs.Num(); ++i )
	{
		renderProg_t& prog = renderProgManager.renderProgs[ i ];
		for( int j = 0; j < prog.pipelines.Num(); ++j )
		{
			idLib::Printf( "%s: %llu\n", prog.name.c_str(), prog.pipelines[ j ].stateBits );
			idLib::Printf( "------------------------------------------\n" );
			RpPrintState( prog.pipelines[ j ].stateBits );
			idLib::Printf( "\n" );
		}
	}
}

void idRenderProgManager::ClearPipelines()
{
	for( int i = 0; i < renderProgManager.renderProgs.Num(); ++i )
	{
		renderProg_t& prog = renderProgManager.renderProgs[ i ];
		for( int j = 0; j < prog.pipelines.Num(); ++j )
		{
			vkDestroyPipeline( vkcontext.device, prog.pipelines[ j ].pipeline, NULL );
		}
		prog.pipelines.Clear();
	}
}


CONSOLE_COMMAND( Vulkan_PrintPipelineStates, "Print the GLState bits associated with each pipeline.", 0 )
{
	renderProgManager.PrintPipelines();
}

CONSOLE_COMMAND( Vulkan_ClearPipelines, "Clear all existing pipelines, forcing them to be recreated.", 0 )
{
	renderProgManager.ClearPipelines();
}

