#pragma hdrstop

#include "../../idlib/precompiled.h"

#include <comdef.h>

// TODO: Remove when we have the ID reader
/*#include "../ByteFileReader.h"*/

DX12Renderer dxRenderer;
extern idCommon* common;

static const char* HLSLParmNames[] = {
	"rpScreenCorrectionFactor",
	"rpWindowCoord",
	"rpDiffuseModifier",
	"rpSpecularModifier",

	"rpLocalLightOrigin",
	"rpLocalViewOrigin",

	"rpLightProjectionS",
	"rpLightProjectionT",
	"rpLightProjectionQ",
	"rpLightFalloffS",

	"rpBumpMatrixS",
	"rpBumpMatrixT",

	"rpDiffuseMatrixS",
	"rpDiffuseMatrixT",

	"rpSpecularMatrixS",
	"rpSpecularMatrixT",

	"rpVertexColorModulate",
	"rpVertexColorAdd",

	"rpColor",
	"rpViewOrigin",
	"rpGlobalEyePos",

	"rpMVPmatrixX",
	"rpMVPmatrixY",
	"rpMVPmatrixZ",
	"rpMVPmatrixW",

	"rpModelMatrixX",
	"rpModelMatrixY",
	"rpModelMatrixZ",
	"rpModelMatrixW",

	"rpProjectionMatrixX",
	"rpProjectionMatrixY",
	"rpProjectionMatrixZ",
	"rpProjectionMatrixW",

	"rpModelViewMatrixX",
	"rpModelViewMatrixY",
	"rpModelViewMatrixZ",
	"rpModelViewMatrixW",

	"rpTextureMatrixS",
	"rpTextureMatrixT",

	"rpTexGen0S",
	"rpTexGen0T",
	"rpTexGen0Q",
	"rpTexGen0Enabled",

	"rpTexGen1S",
	"rpTexGen1T",
	"rpTexGen1Q",
	"rpTexGen1Enabled",

	"rpWobbleSkyX",
	"rpWobbleSkyY",
	"rpWobbleSkyZ",

	"rpOverbright",
	"rpEnableSkinning",
	"rpAlphaTest"
};

void FailMessage(LPCSTR message) {
	OutputDebugString(message);
	common->Error(message);
}

void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		_com_error err(hr);
		// Set a breakpoint on this line to catch DirectX API errors
		FailMessage(err.ErrorMessage());
	}
}

bool WarnIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		_com_error err(hr);
		// Set a breakpoint on this line to catch DirectX API errors
		common->Warning(err.ErrorMessage());
		return false;
	}

	return true;
}

void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter) {
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; ; ++adapterIndex) {
		IDXGIAdapter1* pAdapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter)) {
			// No more adapters.
			break;
		}

		// Check to see if the adapter supports Direct3D 12
		if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr))) {
			*ppAdapter = pAdapter;
			return;
		}

		pAdapter->Release();
	}
}

DX12Renderer::DX12Renderer() :
	m_frameIndex(0),
	m_rtvDescriptorSize(0),
	m_width(512),
	m_height(512),
	m_fullScreen(0)
{
}

DX12Renderer::~DX12Renderer() {
	OnDestroy();
}

void DX12Renderer::Init(HWND hWnd) {
	LoadPipeline(hWnd);
	LoadAssets();

	m_initialized = true;
}

void DX12Renderer::LoadPipeline(HWND hWnd) {
#if defined(_DEBUG)
	{

		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

	// TODO: Try to enable a WARP adapter
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));
	}

	// Describe and create the command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	

	// Create Descriptor Heaps
	{
		// Describe and create the constant buffer view (CBV) descriptor for each frame
		for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex) {
			D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = 1;
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap[frameIndex])));
		}
	}

	// Describe and create the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.BufferDesc.Width = m_width;
	swapChainDesc.BufferDesc.Height = m_height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO: Look into changing this for HDR
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;

	ComPtr<IDXGISwapChain> swapChain;
	ThrowIfFailed(factory->CreateSwapChain(m_commandQueue.Get(), &swapChainDesc, &swapChain));

	ThrowIfFailed(swapChain.As(&m_swapChain));

	// Remove ALT+ENTER functionality.
	ThrowIfFailed(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	if (!CreateBackBuffer()) {
		common->FatalError("Could not initailze backbuffer.");
	}

	// Create Frame Resources
	{
		for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex) {
			// Create the Constant buffer heap for each frame
			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), // Resource must be a multible of 64KB
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, // Currently not clear value needed
				IID_PPV_ARGS(&m_cbvUploadHeap[frameIndex])
			));

			// Create the constant buffer view
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_cbvUploadHeap[frameIndex]->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = (sizeof(m_constantBuffer) + 255) & ~255; // Size is required to be 256 byte aligned
			m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap[frameIndex]->GetCPUDescriptorHandleForHeapStart());

			ZeroMemory(&m_constantBuffer, sizeof(m_constantBuffer));
		}
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

bool DX12Renderer::CreateBackBuffer() {
	// Describe and create a render target view (RTV) descriptor
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
		return false;
	}

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Describe and create a depth-stencil view (DSV) descriptor
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)))) {
		return false;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex) {
		// Create RTV for each frame
		if (FAILED(m_swapChain->GetBuffer(frameIndex, IID_PPV_ARGS(&m_renderTargets[frameIndex])))) {
			return false;
		}

		m_device->CreateRenderTargetView(m_renderTargets[frameIndex].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	// Create the DSV
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil = { 1.0f, 0 };

	if (FAILED(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&m_depthBuffer)
	))) {
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
	dsv.Format = DXGI_FORMAT_D32_FLOAT;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Texture2D.MipSlice = 0;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	m_device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void DX12Renderer::LoadPipelineState(const DX12CompiledShader* vertexShader, const DX12CompiledShader* pixelShader, const IID& riid, void** ppPipelineState) {
	// Define the vertex input layout
	const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT , 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_B8G8R8A8_UNORM , 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_B8G8R8A8_UNORM , 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM , 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 1, DXGI_FORMAT_B8G8R8A8_UNORM , 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT; // As we are reversed from opengl

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootsSignature.Get();
	psoDesc.VS = { vertexShader->data, vertexShader->size };
	psoDesc.PS = { pixelShader->data, pixelShader->size };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, riid, ppPipelineState));	

	// Create the DSV
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil = { 1.0f, 0 };

	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&m_depthBuffer)
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
	dsv.Format = DXGI_FORMAT_D32_FLOAT;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Texture2D.MipSlice = 0;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	m_device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void DX12Renderer::SetActivePipelineState(ID3D12PipelineState** pPipelineState) {
	if (pPipelineState != NULL  && *pPipelineState != NULL && *pPipelineState != m_activePipelineState) {
		m_activePipelineState = *pPipelineState;

		if (m_isDrawing && pPipelineState != NULL) {
			m_commandList->SetPipelineState(*pPipelineState);
		}
	}
}

void DX12Renderer::Uniform4f(UINT index, const float* uniform) {
	memcpy(&m_constantBuffer[index], uniform, sizeof(XMFLOAT4));
}

void DX12Renderer::LoadAssets() {
	// Create the synchronization objects
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		// Create an event handle to use for the frame synchronization
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute
		WaitForPreviousFrame();
	}

	// Create Empty Root Signature
	{
		// Generate the root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];

		// Setup the constant descriptors
		rootParameters[0].InitAsConstantBufferView(0);

		// Setup the descriptor table
		CD3DX12_DESCRIPTOR_RANGE1 descriptorTableRanges[1];
		descriptorTableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		rootParameters[1].InitAsDescriptorTable(1, &descriptorTableRanges[0]);

		CD3DX12_STATIC_SAMPLER_DESC staticSampler[1];
		staticSampler[0].Init(0, D3D12_FILTER_ANISOTROPIC);

		// Register all constants in global.inc
		//rootParameters[0].InitAsConstants((sizeof(XMMATRIX) / 4) * 2, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(2, rootParameters, 1, &staticSampler[0], rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootsSignature)));
	}

	// Create the Command List
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), NULL, IID_PPV_ARGS(&m_commandList)));
	ThrowIfFailed(m_commandList->Close());

	{
		// Create the texture upload heap.
		//TODO: Find a better way and size for textures
		// For now we will assume that the max texture resolution is 1024x1024 32bit pixels
		UINT64 textureUploadBufferSize = ((((1024 * 4) + 255) & ~256) * (1024 - 1)) + (1024 * 4);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_textureBufferUploadHeap)));
		m_textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");
	}
}

DX12VertexBuffer* DX12Renderer::AllocVertexBuffer(DX12VertexBuffer* buffer, UINT numBytes) {
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(numBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer->vertexBuffer)
	));

	buffer->vertexBufferView.BufferLocation = buffer->vertexBuffer->GetGPUVirtualAddress();
	buffer->vertexBufferView.StrideInBytes = sizeof(idDrawVert); // TODO: Change to Doom vertex structure
	buffer->vertexBufferView.SizeInBytes = numBytes;

	return buffer;
}

void DX12Renderer::FreeVertexBuffer(DX12VertexBuffer* buffer) {
	buffer->vertexBuffer->Release();
}

DX12IndexBuffer* DX12Renderer::AllocIndexBuffer(DX12IndexBuffer* buffer, UINT numBytes) {
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(numBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer->indexBuffer)
	));

	buffer->indexBufferView.BufferLocation = buffer->indexBuffer->GetGPUVirtualAddress();
	buffer->indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	buffer->indexBufferView.SizeInBytes = numBytes;

	return buffer;
}

void DX12Renderer::FreeIndexBuffer(DX12IndexBuffer* buffer) {
	buffer->indexBuffer->Release();
}

DX12JointBuffer* DX12Renderer::AllocJointBuffer(DX12JointBuffer* buffer, UINT numBytes) {
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(numBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer->jointBuffer)
	));

	return buffer;
}

void DX12Renderer::FreeJointBuffer(DX12JointBuffer* buffer) {
	buffer->jointBuffer->Release();
}

void DX12Renderer::WaitForPreviousFrame() {
	// TODO: Use a better version

	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	++m_fenceValue;

	// Wait for the frame to finish
	if (m_fence->GetCompletedValue() < fence) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void DX12Renderer::BeginDraw() {
	if (m_isDrawing || !m_initialized) {
		return;
	}

	m_isDrawing = true;
	ThrowIfFailed(m_commandAllocator->Reset()); //TODO: Change to warning
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_activePipelineState));
	m_commandList->SetGraphicsRootSignature(m_rootsSignature.Get());

	// Indicate that we will be rendering to the back buffer
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	m_commandList->SetDescriptorHeaps(1, m_cbvHeap[m_frameIndex].GetAddressOf());

	//m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap[m_frameIndex]->GetGPUDescriptorHandleForHeapStart());
}

void DX12Renderer::EndDraw() {
	if (!m_isDrawing) {
		return;
	}

	// present the backbuffer
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_commandList->Close());

	// Execute the command list
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	m_isDrawing = false;
}

void DX12Renderer::PresentBackbuffer() {
	// Present the frame
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void DX12Renderer::DrawModel(DX12VertexBuffer* vertexBuffer, UINT vertexOffset, DX12IndexBuffer* indexBuffer, UINT indexOffset, UINT indexCount) {
	D3D12_VERTEX_BUFFER_VIEW vertecies = vertexBuffer->vertexBufferView;

	D3D12_INDEX_BUFFER_VIEW indecies = indexBuffer->indexBufferView;

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &vertecies);
	m_commandList->IASetIndexBuffer(&indecies);

	// Draw the model
	m_commandList->DrawIndexedInstanced(indexCount, 1, indexOffset, vertexOffset, 0); // TODO: Multiply by 16 for index?
}

void DX12Renderer::UpdateConstantBuffer() {
	// TODO: Check for better way to do this.
	UINT8* buffer;
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_cbvUploadHeap[m_frameIndex]->Map(0, &readRange, reinterpret_cast<void**>(&buffer)));
	memcpy(buffer, &m_constantBuffer, sizeof(m_constantBuffer));
	m_cbvUploadHeap[m_frameIndex]->Unmap(0, nullptr);

	if (m_isDrawing) {
		m_commandList->SetGraphicsRootConstantBufferView(0, m_cbvUploadHeap[m_frameIndex]->GetGPUVirtualAddress());
	}
}

void DX12Renderer::Clear(bool color, bool depth, bool stencil, byte stencilValue, float* colorRGBA) {
	if (!m_isDrawing) {
		return;
	}

	uint8 clearFlags = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	

	if (color) {
		m_commandList->ClearRenderTargetView(rtvHandle, colorRGBA, 0, nullptr);
	}

	if (depth) {
		clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
	}

	if (stencil) {
		// TODO: Implement stencil first.
		clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
	}

	if (clearFlags > 0) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		m_commandList->ClearDepthStencilView(dsvHandle, static_cast<D3D12_CLEAR_FLAGS>(clearFlags), 0.0f, stencilValue, 0, nullptr);
	}
}

void DX12Renderer::OnDestroy() {
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

bool DX12Renderer::SetScreenParams(UINT width, UINT height, int fullscreen)
{
	// TODO: Resize buffers as needed.

	m_width = width;
	m_height = height;
	m_fullScreen = fullscreen;
	m_aspectRatio = static_cast<FLOAT>(width) / static_cast<FLOAT>(height);

	// TODO: Find the correct window.
	//m_swapChain->SetFullscreenState(true, NULL);

	if (m_device && m_swapChain && m_commandAllocator) {
		WaitForPreviousFrame();

		if (FAILED(m_commandAllocator->Reset())) {
			return false;
		}

		if (FAILED(m_commandList->Reset(m_commandAllocator.Get(), nullptr))) {
			return false;
		}

		for (int frameIndex = 0; frameIndex < FrameCount; ++frameIndex) {
			m_renderTargets[frameIndex].Reset();
		}

		if (FAILED(m_swapChain->ResizeBuffers(FrameCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, NULL))) {
			return false;
		}

		m_frameIndex = 0;
		if (!CreateBackBuffer()) {
			return false;
		}

		UpdateViewport(0.0f, 0.0f, width, height);
		UpdateScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

		if (FAILED(m_commandList->Close())) {
			return false;
		}

		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}
	else {
		UpdateViewport(0.0f, 0.0f, width, height);
		UpdateScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
	}

	

	return true;
}

void DX12Renderer::UpdateViewport(FLOAT topLeftX, FLOAT topLeftY, FLOAT width, FLOAT height, FLOAT minDepth, FLOAT maxDepth) {
	
	m_viewport.TopLeftX = topLeftX;
	m_viewport.TopLeftY = topLeftY;
	m_viewport.Width = width;
	m_viewport.Height = height;
	m_viewport.MinDepth = minDepth;
	m_viewport.MaxDepth = maxDepth;
}

void DX12Renderer::UpdateScissorRect(LONG left, LONG top, LONG right, LONG bottom) {
	m_scissorRect.left = left;
	m_scissorRect.top = top;
	m_scissorRect.right = right;
	m_scissorRect.bottom = bottom;

	if (m_isDrawing) {
		m_commandList->RSSetScissorRects(1, &m_scissorRect);
	}
}

void DX12Renderer::ReadPixels(int x, int y, int width, int height, UINT readBuffer, byte* buffer) {
	// TODO: Implement
	common->Warning("Read Pixels not yet implemented.");
}

void DX12Renderer::SetCullMode(int cullType) {
	// TODO: implement
}

// Texture functions
void DX12Renderer::SetActiveTextureRegister(UINT index) {
	if (index < 5) {
		m_activeTextureRegister = index;
	}
}

DX12TextureBuffer* DX12Renderer::AllocTextureBuffer(DX12TextureBuffer* buffer, const idStr* name) {
	// Create the buffer object.
	if (!WarnIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&buffer->textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&buffer->textureBuffer)))) {
		return nullptr;
	}

	
	wchar_t wname[256];
	mbstowcs(wname, name->c_str(), name->Length() + 1);

	buffer->textureBuffer->SetName(wname);

	// Create the Shader Resource View
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = buffer->textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = buffer->textureDesc.MipLevels;

	for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex) {
		m_device->CreateShaderResourceView(buffer->textureBuffer.Get(), &srvDesc, m_cbvHeap[frameIndex]->GetCPUDescriptorHandleForHeapStart());
	}


	return buffer;
}

void DX12Renderer::SetTextureContent(const DX12TextureBuffer* buffer, const UINT bytesPerRow, const size_t imageSize, const void* image) {
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = image;
	textureData.RowPitch = bytesPerRow;
	textureData.SlicePitch = bytesPerRow * buffer->textureDesc.Height;

	bool runCommandList = !m_isDrawing;

	if (runCommandList) {
		WaitForPreviousFrame();

		if (FAILED(m_commandAllocator->Reset())) {
			common->Warning("Could not reset command allocator.");
			return;
		}

		if (FAILED(m_commandList->Reset(m_commandAllocator.Get(), nullptr))) {
			common->Warning("Could not reset command list.");
			return;
		}
	}

	UpdateSubresources(m_commandList.Get(), buffer->textureBuffer.Get(), m_textureBufferUploadHeap.Get(), 0, 0, 1, &textureData);

	if (runCommandList) {
		if (FAILED(m_commandList->Close())) {
			common->Warning("Could not close command list.");
		}
	}
}

void DX12Renderer::SetTexture(const DX12TextureBuffer* buffer) {
	if (m_isDrawing) {
		D3D12_GPU_VIRTUAL_ADDRESS addr = buffer->textureBuffer->GetGPUVirtualAddress();

		if (addr != NULL) {
			m_commandList->SetComputeRootShaderResourceView(m_activeTextureRegister, addr);
		}
	}
	//m_commandList->
}