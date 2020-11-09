#pragma hdrstop

#include "../../idlib/precompiled.h"

#include <comdef.h>

// TODO: Remove when we have the ID reader
/*#include "../ByteFileReader.h"*/

DX12Renderer dxRenderer;
extern idCommon* common;

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
	//LoadAssets();
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
		// Describe and create a render target view (RTV) descriptor
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Describe and create a depth-stencil view (DSV) descriptor
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

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

	// Create Frame Resources
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create RTV for each frame
		for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex) {
			ThrowIfFailed(m_swapChain->GetBuffer(frameIndex, IID_PPV_ARGS(&m_renderTargets[frameIndex])));
			m_device->CreateRenderTargetView(m_renderTargets[frameIndex].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}

		// TODO: Add the ability to resize this buffer.

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

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

void DX12Renderer::LoadShader(const wchar_t* vsPath, const wchar_t* psPath, const IID &riid, void** ppPipelineState) {
	//TODO: FIX
	/*auto vsFile = ReadBinaryFile(vsPath);
	auto psFile = ReadBinaryFile(psPath);

	// Define the vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootsSignature.Get();
	psoDesc.VS = { vsFile.data(), vsFile.size() };
	psoDesc.PS = { psFile.data(), psFile.size() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, riid, ppPipelineState));*/
}

void DX12Renderer::LoadAssets() {
	// Create Empty Root Signature
	{
		// Generate the root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsConstants((sizeof(XMMATRIX) / 4) * 2, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootsSignature)));
	}

	// Create Shaders
	LoadShader(L"./test_vert.cso", L"./test_frag.cso", IID_PPV_ARGS(&m_uvState));

	// Create the Command List
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_uvState.Get(), IID_PPV_ARGS(&m_commandList)));
	ThrowIfFailed(m_commandList->Close());

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

void DX12Renderer::OnRender() {
	PopulateCommandList();

	// Execute the command list
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void DX12Renderer::PopulateCommandList() {
	ThrowIfFailed(m_commandAllocator->Reset());

	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_uvState.Get()));

	m_commandList->SetGraphicsRootSignature(m_rootsSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that we will be rendering to the back buffer
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// Record commands
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);

	// Update the MVP matrix
	XMMATRIX mvpMatrix = (m_modelMat * m_viewMat) * m_projMat;
	m_commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

	// TODO: CHANGE TO ITEMS
	/*m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_testModel.vertexBufferView);
	m_commandList->IASetIndexBuffer(&m_testModel.indexBufferView);

	// Draw the model
	m_commandList->DrawIndexedInstanced(m_testModel.indexCount, 1, 0, 0, 0);*/

	// present the backbuffer
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_commandList->Close());
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

	UpdateViewport(0.0f, 0.0f, width, height);
	UpdateScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

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
}

void DX12Renderer::OnUpdate() {
	// Update the model matrix.
	float angle = static_cast<float>(0.0);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
	m_modelMat = DirectX::XMMatrixIdentity();// DirectX::XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	// Update the view matrix.
	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	m_viewMat = DirectX::XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update the projection matrix.
	m_projMat = DirectX::XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), m_aspectRatio, 0.1f, 100.0f);
}

void DX12Renderer::ReadPixels(int x, int y, int width, int height, UINT readBuffer, byte* buffer) {
	// TODO: Implement
	common->Warning("Read Pixels not yet implemented.");
}