#pragma hdrstop

#include "../../idlib/precompiled.h"

#include <comdef.h>

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

void __stdcall OnDeviceRemoved(PVOID context, BOOLEAN) {
	ID3D12Device* removedDevice = (ID3D12Device*)context;
	HRESULT removedReason = removedDevice->GetDeviceRemovedReason();

	ComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
	removedDevice->QueryInterface(IID_PPV_ARGS(&pDred)); //TODO: Validate result
	D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput;
	D3D12_DRED_PAGE_FAULT_OUTPUT1 DredPageFaultOutput;
	pDred->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput); //TODO: Validate result
	pDred->GetPageFaultAllocationOutput1(&DredPageFaultOutput); //TODO: Validate result

	_com_error err(removedReason);
	FailMessage(err.ErrorMessage());
}

void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter) {
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; ; ++adapterIndex) {
		IDXGIAdapter1* pAdapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter)) {
			// No more adapters.
			break;
		}

		//TODO: Select the appropriate monitor.
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

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
	m_width(2),
	m_height(2),
	m_fullScreen(0)
{
}

DX12Renderer::~DX12Renderer() {
	OnDestroy();
}

void DX12Renderer::ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		_com_error err(hr);

		// Set a breakpoint on this line to catch DirectX API errors
		FailMessage(err.ErrorMessage());

		throw std::exception(err.ErrorMessage());
	}
}

bool DX12Renderer::WarnIfFailed(HRESULT hr)
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

		ComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings)))) {
			// Turn on auto-breadcrumbs and page fault reporting.
			pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
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

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_directCommandQueue)));

	// Describe and create the copy command queue
	D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_copyCommandQueue)));

	// Create Descriptor Heaps
	{
		// Describe and create the constant buffer view (CBV) descriptor for each frame
		for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex) {
			D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = MAX_DESCRIPTOR_COUNT * MAX_HEAP_OBJECT_COUNT; 
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap[frameIndex])));
		}

		m_cbvHeapIncrementor = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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
	swapChainDesc.Windowed = FALSE; //TODO: Change to option

	ComPtr<IDXGISwapChain> swapChain;
	ThrowIfFailed(factory->CreateSwapChain(m_directCommandQueue.Get(), &swapChainDesc, &swapChain));

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
			// Create the buffer size.
			UINT entrySize = (sizeof(m_constantBuffer) + 255) & ~255; // Size is required to be 256 byte aligned
			UINT resourceAlignment = (1024 * 64) - 1; // Resource must be a multible of 64KB
			UINT heapSize = ((entrySize * MAX_HEAP_OBJECT_COUNT) + resourceAlignment) & ~resourceAlignment;

			// Create the Constant buffer heap for each frame
			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(heapSize), 
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, // Currently not clear value needed
				IID_PPV_ARGS(&m_cbvUploadHeap[frameIndex])
			));

			WCHAR uploadHeapName[20];
			wsprintfW(uploadHeapName, L"CBV Upload Heap %d", frameIndex);
			m_cbvUploadHeap[frameIndex]->SetName(uploadHeapName);

			// Create the constant buffer view for each object
			UINT bufferLocation = m_cbvUploadHeap[frameIndex]->GetGPUVirtualAddress();
			for (UINT objectIndex = 0; objectIndex < MAX_HEAP_OBJECT_COUNT; ++objectIndex) {
				UINT descriptorIndex = objectIndex << MAX_DESCRIPTOR_TWO_POWER; // Descriptor Table Location
				CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_cbvHeap[frameIndex]->GetCPUDescriptorHandleForHeapStart(), descriptorIndex, m_cbvHeapIncrementor);

				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation =  bufferLocation;
				cbvDesc.SizeInBytes = entrySize; 
				m_device->CreateConstantBufferView(&cbvDesc, descriptorHandle);

				bufferLocation += entrySize;
			}

			WCHAR heapName[20];
			wsprintfW(heapName, L"CBV Heap %d", frameIndex);
			m_cbvHeap[frameIndex]->SetName(heapName);

			ZeroMemory(&m_constantBuffer, sizeof(m_constantBuffer));
		}
	}

	// TODO: Turn into a loop for multiple frames
	for (int frame = 0; frame < FrameCount; ++frame)  {
		WCHAR nameDest[25];
		wsprintfW(nameDest, L"Main Command Allocator %d", frame);
		
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_directCommandAllocator[frame])));
		m_directCommandAllocator[frame]->SetName(nameDest);
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_copyCommandAllocator)));
	m_copyCommandAllocator->SetName(L"Main Command Allocator");
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

		WCHAR nameDest[16];
		wsprintfW(nameDest, L"Render Target %d", frameIndex);

		m_renderTargets[frameIndex]->SetName(static_cast<LPCWSTR>(nameDest));
	}

	// Create the DSV
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil = { 1.0f, 128 };

	if (FAILED(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&m_depthBuffer)
	))) {
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
	dsv.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Texture2D.MipSlice = 0;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	m_device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void DX12Renderer::LoadPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc, ID3D12PipelineState** ppPipelineState) {
	assert(ppPipelineState != NULL);

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	psoDesc->pRootSignature = m_rootsSignature.Get();

	ThrowIfFailed(m_device->CreateGraphicsPipelineState(psoDesc, IID_PPV_ARGS(ppPipelineState)));	
}

void DX12Renderer::SetActivePipelineState(ID3D12PipelineState* pPipelineState) {
	if (pPipelineState != NULL  && pPipelineState != m_activePipelineState) {
		m_activePipelineState = pPipelineState;

		if (m_isDrawing) {
			m_commandList->SetPipelineState(pPipelineState);
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

		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_copyFence)));
		m_copyFenceValue = 1;

		// Create an event handle to use for the frame synchronization
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Attach event for device removal
#if defined(_DEBUG)
		m_removeDeviceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_removeDeviceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
		m_fence->SetEventOnCompletion(UINT64_MAX, m_removeDeviceEvent); // This is done because all fence values are set the to  UINT64_MAX value when the device is removed.

		RegisterWaitForSingleObject(
			&m_deviceRemovedHandle,
			m_removeDeviceEvent,
			OnDeviceRemoved,
			m_device.Get(),
			INFINITE,
			0
		);
#endif

		// Wait for the command list to execute
		SignalNextFrame();
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

		CD3DX12_ROOT_PARAMETER1 rootParameters[1];

		// Setup the constant descriptors
		//rootParameters[0].InitAsConstantBufferView(0);

		// Setup the descriptor table
		CD3DX12_DESCRIPTOR_RANGE1 descriptorTableRanges[2];
		descriptorTableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
		descriptorTableRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 1/* m_cbvHeapIncrementor*/);
		rootParameters[0].InitAsDescriptorTable(2, &descriptorTableRanges[0]);

		CD3DX12_STATIC_SAMPLER_DESC staticSampler[1];
		staticSampler[0].Init(0, D3D12_FILTER_ANISOTROPIC);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(1, rootParameters, 1, &staticSampler[0], rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootsSignature)));
	}

	// Create the Command Lists
	{
		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_directCommandAllocator[m_frameIndex].Get(), NULL, IID_PPV_ARGS(&m_commandList)));
		ThrowIfFailed(m_commandList->Close());

		WCHAR nameDest[16];
		wsprintfW(nameDest, L"Command List %d", 0);

		m_commandList->SetName(static_cast<LPCWSTR>(nameDest));
	}

	// Create the Copy Command Lists
	{
		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_copyCommandAllocator.Get(), NULL, IID_PPV_ARGS(&m_copyCommandList)));
		ThrowIfFailed(m_copyCommandList->Close());

		WCHAR nameDest[20];
		wsprintfW(nameDest, L"Copy Command List %d", 0);

		m_copyCommandList->SetName(static_cast<LPCWSTR>(nameDest));
	}

	{
		// Create the texture upload heap.
		//TODO: Find a better way and size for textures
		// For now we will assume that the max texture resolution is 1024x1024 32bit pixels
		const UINT bWidth = 1920;
		const UINT bHeight = 1080;
		const UINT bBytesPerRow = bWidth * 4;
		const UINT64 textureUploadBufferSize = (((bBytesPerRow + 255) & ~256) * (bHeight - 1)) + bBytesPerRow;

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_textureBufferUploadHeap)));

		m_textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

		std::fill(m_activeTextures, m_activeTextures + TEXTURE_REGISTER_COUNT, static_cast<DX12TextureBuffer*>(nullptr));
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

void DX12Renderer::SignalNextFrame() {
	ThrowIfFailed(m_directCommandQueue->Signal(m_fence.Get(), m_fenceValue));
}

void DX12Renderer::WaitForPreviousFrame() {
	const UINT16 fence = m_fenceValue;

	// Wait for the previous frame to finish
	if (m_fence->GetCompletedValue() < fence) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void DX12Renderer::WaitForCopyToComplete() {
	const UINT64 fence = m_copyFenceValue;
	ThrowIfFailed(m_copyCommandQueue->Signal(m_copyFence.Get(), fence));
	++m_copyFenceValue;

	// Wait for the frame to finish
	if (m_copyFence->GetCompletedValue() < fence) {
		ThrowIfFailed(m_copyFence->SetEventOnCompletion(fence, m_copyFenceEvent));
		WaitForSingleObject(m_copyFenceEvent, INFINITE);
	}
}

void DX12Renderer::ResetCommandList(bool waitForBackBuffer) {
	if (!m_isDrawing) {
		return;
	}

	ThrowIfFailed(m_commandList->Reset(m_directCommandAllocator[m_frameIndex].Get(), m_activePipelineState));
	m_commandList->SetGraphicsRootSignature(m_rootsSignature.Get());

	if (waitForBackBuffer) {
		// Indicate that we will be rendering to the back buffer
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	m_commandList->OMSetStencilRef(m_stencilRef);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Setup the initial heap location
	m_commandList->SetDescriptorHeaps(1, m_cbvHeap[m_frameIndex].GetAddressOf());
}

void DX12Renderer::ExecuteCommandList() {
	if (!m_isDrawing) {
		return;
	}

	//TODO: Implement version for multiple command lists
	WarnIfFailed(m_commandList->Close());

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_directCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void DX12Renderer::BeginDraw() {
	if (m_isDrawing || !m_initialized) {
		return;
	}

	WaitForPreviousFrame();
	++m_fenceValue;

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	m_cbvHeapIndex = 0;
	m_isDrawing = true;
	ThrowIfFailed(m_directCommandAllocator[m_frameIndex]->Reset()); //TODO: Change to warning

	ResetCommandList(true);
}

void DX12Renderer::EndDraw() {
	if (!m_isDrawing) {
		return;
	}

	// present the backbuffer
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ExecuteCommandList();

	m_isDrawing = false;

	SignalNextFrame();
}

UINT DX12Renderer::StartSurfaceSettings() {
	assert(m_isDrawing);

	++m_cbvHeapIndex;

	assert(m_cbvHeapIndex < MAX_HEAP_OBJECT_COUNT);

	return m_cbvHeapIndex;
}

bool DX12Renderer::EndSurfaceSettings() {
	// TODO: Define separate CBV for location data and Textures
	// TODO: add a check if we need to update tehCBV and Texture data.

	assert(m_isDrawing);

	if (!DX12_ActivatePipelineState()) {
		// We cant draw the object, so return.
		return false;
	}

	const UINT descriptorIndex = m_cbvHeapIndex << MAX_DESCRIPTOR_TWO_POWER;
	UINT currentDescriptorIndex = descriptorIndex;
	
	// Copy the CBV value to the upload heap
	UINT8* buffer;
	CD3DX12_RANGE readRange(0, 0);
	UINT offset = ((sizeof(m_constantBuffer) + 255) & ~255)* m_cbvHeapIndex; // Each entry is 256 byte aligned.
	ThrowIfFailed(m_cbvUploadHeap[m_frameIndex]->Map(0, &readRange, reinterpret_cast<void**>(&buffer)));
	memcpy(&buffer[offset], &m_constantBuffer, sizeof(m_constantBuffer));
	m_cbvUploadHeap[m_frameIndex]->Unmap(0, nullptr);

	// Copy the Textures
	DX12TextureBuffer* currentTexture;
	for (UINT index = 0; index < TEXTURE_REGISTER_COUNT && (currentTexture = m_activeTextures[index]) != nullptr; ++index) {
		++currentDescriptorIndex;
		
		
		//UINT descriptorIndex = (m_cbvHeapIndex << MAX_DESCRIPTOR_TWO_POWER) + 1 + index; // (Descriptor Table Location) + (CBV Location) + (Texture Register Offset)
		SetTexturePixelShaderState(currentTexture);

		CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle(m_cbvHeap[m_frameIndex]->GetCPUDescriptorHandleForHeapStart(), currentDescriptorIndex, m_cbvHeapIncrementor);
		m_device->CreateShaderResourceView(currentTexture->textureBuffer.Get(), &currentTexture->textureView, textureHandle);

		//m_copyCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentTexture->textureBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON));
	}

	// Define the Descriptor Table to use.
	const CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorTableHandle(m_cbvHeap[m_frameIndex]->GetGPUDescriptorHandleForHeapStart(), descriptorIndex, m_cbvHeapIncrementor);
	m_commandList->SetGraphicsRootDescriptorTable(0, descriptorTableHandle);

	++m_cbvHeapIndex;

	return true;
}

bool DX12Renderer::IsScissorWindowValid() {
	return m_scissorRect.right > m_scissorRect.left && m_scissorRect.bottom > m_scissorRect.top;
}

void DX12Renderer::PresentBackbuffer() {
	// Present the frame
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void DX12Renderer::DrawModel(DX12VertexBuffer* vertexBuffer, UINT vertexOffset, DX12IndexBuffer* indexBuffer, UINT indexOffset, UINT indexCount) {
	if (!IsScissorWindowValid()) {
		return;
	}

	D3D12_VERTEX_BUFFER_VIEW vertecies = vertexBuffer->vertexBufferView;

	D3D12_INDEX_BUFFER_VIEW indecies = indexBuffer->indexBufferView;

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &vertecies);
	m_commandList->IASetIndexBuffer(&indecies);

	// Draw the model
	m_commandList->DrawIndexedInstanced(indexCount, 1, indexOffset, vertexOffset, 0); // TODO: Multiply by 16 for index?
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
		m_commandList->ClearDepthStencilView(dsvHandle, static_cast<D3D12_CLEAR_FLAGS>(clearFlags), 1.0f, stencilValue, 0, nullptr);
	}
}

void DX12Renderer::OnDestroy() {
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
	m_initialized = false;
}

bool DX12Renderer::SetScreenParams(UINT width, UINT height, int fullscreen)
{
	/*if (m_width == width && m_height == height && m_fullScreen == fullscreen) {
		return true;
	}*/
	// TODO: Resize buffers as needed.

	m_width = width;
	m_height = height;
	m_fullScreen = fullscreen;
	m_aspectRatio = static_cast<FLOAT>(width) / static_cast<FLOAT>(height);

	// TODO: Find the correct window.
	//m_swapChain->SetFullscreenState(true, NULL);

	// TODO: HANDLE THIS WHILE DRAWING.
	if (m_device && m_swapChain && m_directCommandAllocator[m_frameIndex]) {
		if (!m_isDrawing) {
			WaitForPreviousFrame();
			++m_fenceValue;

			if (FAILED(m_directCommandAllocator[m_frameIndex]->Reset())) {
				common->Warning("DX12Renderer::SetScreenParams: Error resetting command allocator.");
				return false;
			}

			if (FAILED(m_commandList->Reset(m_directCommandAllocator[m_frameIndex].Get(), nullptr))) {
				common->Warning("DX12Renderer::SetScreenParams: Error resetting command list.");
				return false;
			}
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

		if (!m_isDrawing) {
			if (FAILED(m_commandList->Close())) {
				return false;
			}

			ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
			m_directCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		}

		SignalNextFrame();
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

void DX12Renderer::UpdateStencilRef(UINT ref) {
	if (m_stencilRef != ref) {
		m_stencilRef = ref;

		if (m_isDrawing) {
			m_commandList->OMSetStencilRef(ref);
		}
	}
}

void DX12Renderer::ReadPixels(int x, int y, int width, int height, UINT readBuffer, byte* buffer) {
	// TODO: Implement
	common->Warning("Read Pixels not yet implemented.");
}

// Texture functions
void DX12Renderer::SetActiveTextureRegister(UINT8 index) {
	if (index < 5) {
		m_activeTextureRegister = index;
	}
}

DX12TextureBuffer* DX12Renderer::AllocTextureBuffer(DX12TextureBuffer* buffer, D3D12_RESOURCE_DESC* textureDesc, const idStr* name) {
	// Create the buffer object.
	if (!WarnIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&buffer->textureBuffer)))) {
		return nullptr;
	}

	// Add a name to the property.
	wchar_t wname[256];
	wsprintfW(wname, L"Texture: %hs", name->c_str());
	buffer->textureBuffer->SetName(wname);

	// Create the Shader Resource View
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc->Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = textureDesc->MipLevels;

	buffer->textureView = srvDesc;
	buffer->usageState = D3D12_RESOURCE_STATE_COPY_DEST;
	buffer->name = name;

	return buffer;
}

void DX12Renderer::FreeTextureBuffer(DX12TextureBuffer* buffer) {
	if (buffer != nullptr) {
		WaitForCopyToComplete();
		WaitForPreviousFrame();
		delete(buffer);
	}
}

void DX12Renderer::SetTextureContent(DX12TextureBuffer* buffer, const UINT mipLevel, const UINT bytesPerRow, const size_t imageSize, const void* image) {
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = image;
	textureData.RowPitch = bytesPerRow;
	textureData.SlicePitch = imageSize;

	int intermediateOffset = 0;
	if (mipLevel > 0) {
		UINT mipCheck = mipLevel;
		size_t lastSize = imageSize << 2;

		for (; mipCheck > 0; --mipCheck) {
			intermediateOffset += lastSize;
			lastSize = lastSize << 2;
		}

		intermediateOffset = (intermediateOffset + 511) & ~511;
	}

	SetTextureCopyState(buffer, mipLevel);
	UpdateSubresources(m_copyCommandList.Get(), buffer->textureBuffer.Get(), m_textureBufferUploadHeap.Get(), intermediateOffset, mipLevel, 1, &textureData);
}

void DX12Renderer::SetTexture(DX12TextureBuffer* buffer) {
	m_activeTextures[m_activeTextureRegister] = buffer;
}

bool DX12Renderer::SetTextureCopyState(DX12TextureBuffer* buffer, const UINT mipLevel) {
	return SetTextureState(buffer, D3D12_RESOURCE_STATE_COPY_DEST, m_copyCommandList.Get(), mipLevel);
}

bool DX12Renderer::SetTexturePixelShaderState(DX12TextureBuffer* buffer, const UINT mipLevel) {
	return SetTextureState(buffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, m_commandList.Get());
}

bool DX12Renderer::SetTextureState(DX12TextureBuffer* buffer, const D3D12_RESOURCE_STATES usageState, ID3D12GraphicsCommandList* commandList, const UINT mipLevel) {
	if (buffer == nullptr) {
		return false;
	}

	if (buffer->usageState == usageState) {
		return false;
	}
	
	// TODO: Check for valid state transitions.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer->textureBuffer.Get(), buffer->usageState, usageState, mipLevel));
	buffer->usageState = usageState;

	return true; // STate has changed.
}

void DX12Renderer::StartTextureWrite(DX12TextureBuffer* buffer) {
	WaitForCopyToComplete();

	if (FAILED(m_copyCommandAllocator->Reset())) {
		common->Warning("Could not reset the copy command allocator.");
		return;
	}

	if (FAILED(m_copyCommandList->Reset(m_copyCommandAllocator.Get(), nullptr))) {
		common->Warning("Could not reset the copy command list.");
		return;
	}
}

void DX12Renderer::EndTextureWrite(DX12TextureBuffer* buffer) {
	if (FAILED(m_copyCommandList->Close())) {
		common->Warning("Could not close copy command list.");
	}

	// Execute the command list
	ID3D12CommandList* ppCommandLists[] = { m_copyCommandList.Get() };
	m_copyCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	WaitForCopyToComplete();
}