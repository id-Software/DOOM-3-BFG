#pragma once

#include <wrl.h>
#include "./d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <debugapi.h>
#include <dxcapi.h>
#include <DirectXMath.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

const UINT FrameCount = 2;


struct Vertex
{
	XMFLOAT4 position;
	XMFLOAT4 colour;
};

struct StoredModel
{
	XMMATRIX modelMat;
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	UINT indexCount;
};

class DX12Renderer {
public:
	DX12Renderer(HWND hwnd, UINT width, UINT height);
	~DX12Renderer();

	virtual void OnInit();
	virtual void OnResize(UINT width, UINT height);
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();

	void UpdateViewport(FLOAT topLeftX, FLOAT topLeftY, FLOAT width, FLOAT height, FLOAT minDepth = D3D12_MIN_DEPTH, FLOAT maxDepth = D3D12_MAX_DEPTH);
	void UpdateScissorRect(LONG left, LONG top, LONG right, LONG bottom);



private:
	UINT m_width = 0;
	UINT m_height = 0;
	HWND m_hwnd;
	FLOAT m_aspectRatio = 1.0f;
    FLOAT m_FoV = 90.0f;

	// Pipeline
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<ID3D12Device5> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	UINT m_rtvDescriptorSize;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12RootSignature> m_rootsSignature;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12Resource> m_depthBuffer;

	// Shaders
	ComPtr<ID3D12PipelineState> m_uvState;

	// Synchronization
	UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT16 m_fenceValue;

    // TODO: Temp objects
    XMMATRIX m_modelMat;
	XMMATRIX m_viewMat;
	XMMATRIX m_projMat;
	StoredModel m_testModel;

	void LoadPipeline();
	void LoadAssets();
	void LoadShader(const wchar_t* vsPath, const wchar_t* psPath, const IID& riid, void** ppPipelineStatee);

    void WaitForPreviousFrame();

	// Model functions
	void GenerateStoredModel(StoredModel* model, const Vertex* vertecies, const UINT vertexSize, const WORD* indecies, const UINT indexSize, UINT indexCount);

    // Heap functions
    void UpdateCommittedResource(ComPtr<ID3D12Resource> resource, const void* data, size_t size);

    // Render functions
    void PopulateCommandList();

};

extern DX12Renderer* dxRenderer;