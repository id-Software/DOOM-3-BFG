#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../tr_local.h"

#include <unordered_map>

D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDescriptors[53];
std::unordered_map<int32, ID3D12PipelineState*> pipelineStateMap(128);

D3D12_CULL_MODE CalculateCullMode(const int cullType) {
	switch (cullType) {
	case CT_FRONT_SIDED:
		return D3D12_CULL_MODE_BACK;
	case CT_BACK_SIDED:
		return D3D12_CULL_MODE_FRONT;
	}

	return D3D12_CULL_MODE_NONE;
}

D3D12_BLEND_DESC CalculateBlendMode(uint64 stateBits) {
	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	D3D12_BLEND srcFactor = D3D12_BLEND_ONE;
	D3D12_BLEND dstFactor = D3D12_BLEND_ZERO;

	switch (stateBits & GLS_SRCBLEND_BITS) {
	case GLS_SRCBLEND_ZERO:					srcFactor = D3D12_BLEND_ZERO; break;
	case GLS_SRCBLEND_ONE:					srcFactor = D3D12_BLEND_ONE; break;
	case GLS_SRCBLEND_DST_COLOR:			srcFactor = D3D12_BLEND_DEST_COLOR; break;
	case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:	srcFactor = D3D12_BLEND_INV_DEST_COLOR; break;
	case GLS_SRCBLEND_SRC_ALPHA:			srcFactor = D3D12_BLEND_SRC_ALPHA; break;
	case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	srcFactor = D3D12_BLEND_INV_SRC_ALPHA; break;
	case GLS_SRCBLEND_DST_ALPHA:			srcFactor = D3D12_BLEND_DEST_ALPHA; break;
	case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:	srcFactor = D3D12_BLEND_INV_DEST_ALPHA; break;
	default:
		assert(!"GL_State: invalid src blend state bits\n");
		break;
	}

	switch (stateBits & GLS_DSTBLEND_BITS) {
	case GLS_DSTBLEND_ZERO:					dstFactor = D3D12_BLEND_ZERO; break;
	case GLS_DSTBLEND_ONE:					dstFactor = D3D12_BLEND_ONE; break;
	case GLS_DSTBLEND_SRC_COLOR:			dstFactor = D3D12_BLEND_SRC_COLOR; break;
	case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:	dstFactor = D3D12_BLEND_INV_SRC_COLOR; break;
	case GLS_DSTBLEND_SRC_ALPHA:			dstFactor = D3D12_BLEND_SRC_ALPHA; break;
	case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	dstFactor = D3D12_BLEND_INV_SRC_ALPHA; break;
	case GLS_DSTBLEND_DST_ALPHA:			dstFactor = D3D12_BLEND_DEST_ALPHA; break;
	case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:  dstFactor = D3D12_BLEND_INV_DEST_ALPHA; break;
	default:
		assert(!"GL_State: invalid dst blend state bits\n");
		break;
	}

	blendDesc.RenderTarget[0].SrcBlend = srcFactor;
	blendDesc.RenderTarget[0].DestBlend = dstFactor;
		
	//blendDesc.AlphaToCoverageEnable = TRUE:

	return blendDesc;
}

void LoadStagePipelineState(int parentState, glstate_t state) {
	// Generate the state index
	// (Blend State) | (cullType << 6) | (shader index << 8)
	int32 stateIndex = (state.glStateBits & 0x00000003F) | (state.faceCulling << 6) | (parentState << 8);
	const auto result = pipelineStateMap.find(stateIndex);

	if (result == pipelineStateMap.end()) {
		// No PipelineState found. Create a new one.

		// Define the vertex input layout
		const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT , 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_B8G8R8A8_UNORM , 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_B8G8R8A8_UNORM , 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM , 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 1, DXGI_FORMAT_B8G8R8A8_UNORM , 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = pipelineDescriptors[parentState];
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };

		psoDesc.RasterizerState.CullMode = CalculateCullMode(state.faceCulling);
		psoDesc.BlendState = CalculateBlendMode(state.glStateBits);

		ID3D12PipelineState* renderState;
		dxRenderer.LoadPipelineState(&psoDesc, &renderState);

		wchar_t resourceName[64];
		wsprintfW(resourceName, L"Shader: %x", stateIndex);
		renderState->SetName(resourceName);

		pipelineStateMap.insert({ stateIndex, renderState });

		dxRenderer.SetActivePipelineState(renderState);
	}
	else {
		dxRenderer.SetActivePipelineState(result->second);
	}
}

void LoadHLSLShader(DX12CompiledShader* shader, const char* name, eShader shaderType) {
	idStr inFile;
	inFile.Format("renderprogs\\hlsl\\%s", name);
	inFile.StripFileExtension();

	switch (shaderType) {
	case VERTEX:
		inFile += ".vcso";
		break;

	case PIXEL:
		inFile += ".pcso";
		break;

	default:
		inFile += ".cso";
	}

	void* data = NULL;
	shader->size = fileSystem->ReadFile(inFile.c_str(), &data);
	shader->data = static_cast<byte*>(data);
}

void idRenderProgManager::SetUniformValue(const renderParm_t rp, const float* value) {
	dxRenderer.Uniform4f(rp, value);
}

int idRenderProgManager::FindProgram(const char* name, int vIndex, int fIndex) {
	for (int i = 0; i < shaderPrograms.Num(); ++i) {
		if ((shaderPrograms[i].vertexShaderIndex == vIndex) && (shaderPrograms[i].fragmentShaderIndex == fIndex)) {
			LoadProgram(i, vIndex, fIndex);
			return i;
		}
	}

	hlslProgram_t program;
	program.name = name;
	int index = shaderPrograms.Append(program);
	LoadProgram(index, vIndex, fIndex);
	return index;
}

/// <summary>
/// Creates a base pipeline state descriptor. This will be used later to generate the material pipeline states.
/// </summary>
/// <param name="programIndex"></param>
/// <param name="vertexShaderIndex"></param>
/// <param name="fragmentShaderIndex"></param>
void idRenderProgManager::LoadProgram(const int programIndex, const int vertexShaderIndex, const int fragmentShaderIndex) {
	// TODO: Implment
	hlslProgram_t & prog = shaderPrograms[programIndex];

	if (prog.shaderObject != NULL) {
		return; // Already loaded.
	}

	DX12CompiledShader* vertexShader = (vertexShaderIndex != -1) ? static_cast<DX12CompiledShader*>(vertexShaders[vertexShaderIndex].apiObject) : NULL;
	DX12CompiledShader* fragmentShader = (fragmentShaderIndex != -1) ? static_cast<DX12CompiledShader*>(fragmentShaders[fragmentShaderIndex].apiObject) : NULL;

	if (vertexShader == NULL || fragmentShader == NULL || vertexShader->data == NULL || fragmentShader->data == NULL) {
		common->Warning("Could not build shader %s.", vertexShaders[vertexShaderIndex].name.c_str());
		return;
	}

	CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);
	rasterizerState.FrontCounterClockwise = TRUE; // This is done to match the opengl direction.
	rasterizerState.CullMode = D3D12_CULL_MODE_NONE; // Default front and back.

	CD3DX12_BLEND_DESC blendState(D3D12_DEFAULT); // TODO: set the blend state. We need to load these pipeline states for entire materials and not just programs.

	D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc = &pipelineDescriptors[programIndex];
	psoDesc->VS = { vertexShader->data, vertexShader->size };
	psoDesc->PS = { fragmentShader->data, fragmentShader->size };
	psoDesc->RasterizerState = rasterizerState;
	psoDesc->BlendState = blendState;
	psoDesc->SampleMask = UINT_MAX;
	psoDesc->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc->NumRenderTargets = 1;
	psoDesc->RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc->DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc->SampleDesc.Count = 1;

	prog.shaderObject = psoDesc;
}

/*
================================================================================================
idRenderProgManager::LoadVertexShader
================================================================================================
*/
void idRenderProgManager::LoadVertexShader(int index) {
	if ( vertexShaders[index].apiObject != NULL ) {
		return; // Already loaded
	}

	DX12CompiledShader* shader = (DX12CompiledShader*)malloc(sizeof(DX12CompiledShader));

	LoadHLSLShader(shader, vertexShaders[index].name, VERTEX);

	vertexShaders[index].apiObject = shader;
	//vertexShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_VERTEX_SHADER, vertexShaders[index].name, vertexShaders[index].uniforms );*/
}

/*
================================================================================================
idRenderProgManager::LoadFragmentShader
================================================================================================
*/
void idRenderProgManager::LoadFragmentShader(int index) {
	if (fragmentShaders[index].apiObject != NULL) {
		return; // Already loaded
	}

	DX12CompiledShader* shader = (DX12CompiledShader*)malloc(sizeof(DX12CompiledShader));

	LoadHLSLShader(shader, fragmentShaders[index].name, PIXEL);

	fragmentShaders[index].apiObject = shader;

	//fragmentShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_FRAGMENT_SHADER, fragmentShaders[index].name, fragmentShaders[index].uniforms );
}

/*
================================================================================================
idRenderProgManager::BindShader
================================================================================================
*/
void idRenderProgManager::BindShader(int vIndex, int fIndex) {
	if (currentVertexShader == vIndex && currentFragmentShader == fIndex) {
		return;
	}

	currentVertexShader = vIndex;
	currentFragmentShader = fIndex;

	// vIndex denotes the GLSL program
	if (vIndex >= 0 && vIndex < shaderPrograms.Num()) {
		if (shaderPrograms[vIndex].shaderObject == NULL) {
			common->Warning("RenderState %s has not been loaded.", shaderPrograms[vIndex].name.c_str());
			return;
		}

		currentRenderProgram = vIndex;
		RENDERLOG_PRINTF("Binding RenderState %s\n", shaderPrograms[vIndex].name.c_str());

		LoadStagePipelineState(vIndex, backEnd.glState);
	}
	else {
		common->Warning("Shader index is out of range.");
	}
}

/*
================================================================================================
idRenderProgManager::SetRenderParm
================================================================================================
*/
void idRenderProgManager::SetRenderParm(renderParm_t rp, const float* value) {
	// TODO: Set the property.
	dxRenderer.Uniform4f(rp, value);
}