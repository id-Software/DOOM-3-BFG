#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../tr_local.h"

#include <unordered_map>

D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDescriptors[53];
std::unordered_map<int64, ID3D12PipelineState*> pipelineStateMap(128);

D3D12_CULL_MODE CalculateCullMode(const int cullType) {
	switch (cullType) {
	case CT_FRONT_SIDED:
		return D3D12_CULL_MODE_BACK;
	case CT_BACK_SIDED:
		return D3D12_CULL_MODE_FRONT;
	}

	return D3D12_CULL_MODE_NONE;
}

D3D12_DEPTH_STENCIL_DESC CalculateDepthStencilMode(uint64 stateBits) {
	D3D12_DEPTH_STENCIL_DESC dsDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	//TODO: FIX!!. For now we are disabling the depth test:
	dsDesc.DepthEnable = false;

	// Check if we should enable the depth mask
	if (stateBits & GLS_DEPTHMASK) {
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	}
	else {
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	}

	// Calculate the stencil
	if (stateBits & (GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS)) {
		dsDesc.StencilEnable = true;
	}
	else {
		dsDesc.StencilEnable = false;
	}

	if (stateBits & (GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS)) {
		uint8 ref = static_cast<uint8>((stateBits & GLS_STENCIL_FUNC_REF_BITS) >> GLS_STENCIL_FUNC_REF_SHIFT);
		uint8 mask = static_cast<uint8>((stateBits & GLS_STENCIL_FUNC_MASK_BITS) >> GLS_STENCIL_FUNC_MASK_SHIFT);
		D3D12_COMPARISON_FUNC func = D3D12_COMPARISON_FUNC_NEVER;
		
		// TODO: Double check that this is correct.
		dsDesc.StencilReadMask = mask & ref;
		dsDesc.StencilWriteMask = mask & ref;

		switch (stateBits & GLS_STENCIL_FUNC_BITS) {
			case GLS_STENCIL_FUNC_NEVER:		func = D3D12_COMPARISON_FUNC_NEVER; break;
			case GLS_STENCIL_FUNC_LESS:			func = D3D12_COMPARISON_FUNC_LESS; break;
			case GLS_STENCIL_FUNC_EQUAL:		func = D3D12_COMPARISON_FUNC_EQUAL; break;
			case GLS_STENCIL_FUNC_LEQUAL:		func = D3D12_COMPARISON_FUNC_LESS_EQUAL; break;
			case GLS_STENCIL_FUNC_GREATER:		func = D3D12_COMPARISON_FUNC_GREATER; break;
			case GLS_STENCIL_FUNC_NOTEQUAL:		func = D3D12_COMPARISON_FUNC_NOT_EQUAL; break;
			case GLS_STENCIL_FUNC_GEQUAL:		func = D3D12_COMPARISON_FUNC_GREATER_EQUAL; break;
			case GLS_STENCIL_FUNC_ALWAYS:		func = D3D12_COMPARISON_FUNC_ALWAYS; break;
		}

		dsDesc.FrontFace.StencilFunc = func;
		dsDesc.BackFace.StencilFunc = func;
	}

	if (stateBits & (GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS)) {
		D3D12_STENCIL_OP sFail = D3D12_STENCIL_OP_KEEP;
		D3D12_STENCIL_OP zFail = D3D12_STENCIL_OP_KEEP;
		D3D12_STENCIL_OP pass = D3D12_STENCIL_OP_KEEP;

		switch (stateBits & GLS_STENCIL_OP_FAIL_BITS) {
			case GLS_STENCIL_OP_FAIL_KEEP:		sFail = D3D12_STENCIL_OP_KEEP; break;
			case GLS_STENCIL_OP_FAIL_ZERO:		sFail = D3D12_STENCIL_OP_ZERO; break;
			case GLS_STENCIL_OP_FAIL_REPLACE:	sFail = D3D12_STENCIL_OP_REPLACE; break;
			case GLS_STENCIL_OP_FAIL_INCR:		sFail = D3D12_STENCIL_OP_INCR_SAT; break;
			case GLS_STENCIL_OP_FAIL_DECR:		sFail = D3D12_STENCIL_OP_DECR_SAT; break;
			case GLS_STENCIL_OP_FAIL_INVERT:	sFail = D3D12_STENCIL_OP_INVERT; break;
			case GLS_STENCIL_OP_FAIL_INCR_WRAP: sFail = D3D12_STENCIL_OP_INCR; break;
			case GLS_STENCIL_OP_FAIL_DECR_WRAP: sFail = D3D12_STENCIL_OP_DECR; break;
		}

		switch (stateBits & GLS_STENCIL_OP_ZFAIL_BITS) {
			case GLS_STENCIL_OP_ZFAIL_KEEP:		zFail = D3D12_STENCIL_OP_KEEP; break;
			case GLS_STENCIL_OP_ZFAIL_ZERO:		zFail = D3D12_STENCIL_OP_ZERO; break;
			case GLS_STENCIL_OP_ZFAIL_REPLACE:	zFail = D3D12_STENCIL_OP_REPLACE; break;
			case GLS_STENCIL_OP_ZFAIL_INCR:		zFail = D3D12_STENCIL_OP_INCR_SAT; break;
			case GLS_STENCIL_OP_ZFAIL_DECR:		zFail = D3D12_STENCIL_OP_DECR_SAT; break;
			case GLS_STENCIL_OP_ZFAIL_INVERT:	zFail = D3D12_STENCIL_OP_INVERT; break;
			case GLS_STENCIL_OP_ZFAIL_INCR_WRAP: zFail = D3D12_STENCIL_OP_INCR; break;
			case GLS_STENCIL_OP_ZFAIL_DECR_WRAP: zFail = D3D12_STENCIL_OP_DECR; break;
		}

		switch (stateBits & GLS_STENCIL_OP_PASS_BITS) {
			case GLS_STENCIL_OP_PASS_KEEP:		pass = D3D12_STENCIL_OP_KEEP; break;
			case GLS_STENCIL_OP_PASS_ZERO:		pass = D3D12_STENCIL_OP_ZERO; break;
			case GLS_STENCIL_OP_PASS_REPLACE:	pass = D3D12_STENCIL_OP_REPLACE; break;
			case GLS_STENCIL_OP_PASS_INCR:		pass = D3D12_STENCIL_OP_INCR_SAT; break;
			case GLS_STENCIL_OP_PASS_DECR:		pass = D3D12_STENCIL_OP_DECR_SAT; break;
			case GLS_STENCIL_OP_PASS_INVERT:	pass = D3D12_STENCIL_OP_INVERT; break;
			case GLS_STENCIL_OP_PASS_INCR_WRAP: pass = D3D12_STENCIL_OP_INCR; break;
			case GLS_STENCIL_OP_PASS_DECR_WRAP: pass = D3D12_STENCIL_OP_DECR; break;
		}

		dsDesc.FrontFace.StencilFailOp = dsDesc.BackFace.StencilFailOp = sFail;
		dsDesc.FrontFace.StencilDepthFailOp = dsDesc.BackFace.StencilDepthFailOp = zFail;
		dsDesc.FrontFace.StencilPassOp = dsDesc.BackFace.StencilPassOp = pass;
	}

	return dsDesc;
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
	blendDesc.RenderTarget[0].BlendEnable = !(srcFactor == D3D12_BLEND_ONE && dstFactor == D3D12_BLEND_ZERO);

	return blendDesc;
}

void LoadStagePipelineState(int parentState, glstate_t state) {
	// TODO: Add the stencil state to the index.
	// Generate the state index
	// (Blend State) | (cullType << 6) | (shader index << 8)
	int64 stateIndex = (state.glStateBits & 0x00000003F) | (state.faceCulling << 6) | (parentState << 8);
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
		psoDesc.DepthStencilState = CalculateDepthStencilMode(state.glStateBits);

		// TODO: Enable colour mask.

		ID3D12PipelineState* renderState;
		dxRenderer.LoadPipelineState(&psoDesc, &renderState);

		wchar_t resourceName[64];
		wsprintfW(resourceName, L"Shader: 0x%x", stateIndex);
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
	psoDesc->DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
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