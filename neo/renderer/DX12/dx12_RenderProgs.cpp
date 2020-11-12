#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../tr_local.h"

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

	if (vertexShader == NULL || fragmentShader == NULL || vertexShader->data == NULL || fragmentShader->data == NULL) {
		common->Warning("Could not build shader %s.", vertexShaders[vertexShaderIndex].name.c_str());
		return;
	}

	ComPtr<ID3D12PipelineState> renderProgram;
	dxRenderer.LoadPipelineState(vertexShader, fragmentShader, IID_PPV_ARGS(&renderProgram));

	// TODO: Implement the uniforms and binders.

	idStr programName = vertexShaders[vertexShaderIndex].name;
	programName.StripFileExtension();
	prog.name = programName;
	prog.shaderObject = renderProgram.GetAddressOf();
	prog.fragmentShaderIndex = fragmentShaderIndex;
	prog.vertexShaderIndex = vertexShaderIndex;
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