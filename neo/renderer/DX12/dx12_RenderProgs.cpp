#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../tr_local.h"

void idRenderProgManager::SetUniformValue(const renderParm_t rp, const float* value) {
	// TODO: SetUniformValue
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

void idRenderProgManager::LoadProgram(const int programIndex, const int vertexShaderIndex, const int fragmentShaderIndex) {
	// TODO: Implment
	hlslProgram_t & prog = shaderPrograms[programIndex];

	if (prog.shaderObject != NULL) {
		return; // Already loaded.
	}

	DX12CompiledShader* vertexShader = (vertexShaderIndex != -1) ? static_cast<DX12CompiledShader*>(vertexShaders[vertexShaderIndex].apiObject) : NULL;
	DX12CompiledShader* fragmentShader = (fragmentShaderIndex != -1) ? static_cast<DX12CompiledShader*>(fragmentShaders[fragmentShaderIndex].apiObject) : NULL;

	ComPtr<ID3D12PipelineState>* renderProgram; // Change this to list pointer
	dxRenderer.LoadPipelineState(vertexShader, fragmentShader, IID_PPV_ARGS(&renderProgram));

	// TODO: Implement the uniforms and binders.

	idStr programName = vertexShaders[vertexShaderIndex].name;
	programName.StripFileExtension();
	prog.name = programName;
	prog.shaderObject = renderProgram;
	prog.fragmentShaderIndex = fragmentShaderIndex;
	prog.vertexShaderIndex = vertexShaderIndex;
}

/*
================================================================================================
idRenderProgManager::LoadVertexShader
================================================================================================
*/
void idRenderProgManager::LoadVertexShader(int index) {
	// TODO: Change to load the vertex shader binary.

	/*if ( vertexShaders[index].progId != INVALID_PROGID ) {
		return; // Already loaded
	}
	vertexShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_VERTEX_SHADER, vertexShaders[index].name, vertexShaders[index].uniforms );*/
}

/*
================================================================================================
idRenderProgManager::LoadFragmentShader
================================================================================================
*/
void idRenderProgManager::LoadFragmentShader(int index) {
	// TODO: Change to load the fragmentshader binary.

	/*if ( fragmentShaders[index].progId != INVALID_PROGID ) {
		return; // Already loaded
	}
	fragmentShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_FRAGMENT_SHADER, fragmentShaders[index].name, fragmentShaders[index].uniforms );*/
}