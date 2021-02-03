#ifndef __DX12_RENDERER_H__
#define __DX12_RENDERER_H__

#include <wrl.h>
#include <initguid.h>
#include "./d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <debugapi.h>
#include <dxcapi.h>
#include <DirectXMath.h>

#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxcompiler.lib")

// Use D3D clip space.
#define CLIP_SPACE_D3D

#define BUFFER_RGB 0x01
#define BUFFER_STENCIL 0x02

#define COMMAND_LIST_COUNT 5

// TODO: We will separate the CBV and materials into two separate heap objects. This will allow us to define objects positional properties differently from the material properties.
#define TEXTURE_REGISTER_COUNT 5
#define MAX_DESCRIPTOR_COUNT 8 // 1 CBV and 5 Shader Resource View, 2 extra to keep this as a power of 2
#define MAX_DESCRIPTOR_TWO_POWER 3
#define MAX_HEAP_OBJECT_COUNT 10000

using namespace DirectX;
using namespace Microsoft::WRL;

const UINT FrameCount = 2;

struct Vertex
{
	XMFLOAT4 position;
	XMFLOAT4 colour;
};

struct DX12VertexBuffer
{
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
};

struct DX12IndexBuffer
{
	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	UINT indexCount;
};

struct DX12CompiledShader
{
	byte* data;
	size_t size;
};

struct DX12TextureBuffer
{
	ComPtr<ID3D12Resource> textureBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC textureView;
	D3D12_RESOURCE_STATES usageState;
};

struct DX12JointBuffer
{
	// TODO: Check if any of this is correct.
	ComPtr<ID3D12Resource> jointBuffer;
};

enum eShader {
	VERTEX,
	PIXEL
};

void DX12_ActivatePipelineState();

class DX12Renderer {
public:
	DX12Renderer();
	~DX12Renderer();

	virtual void Init(HWND hWnd);
	virtual bool SetScreenParams(UINT width, UINT height, int fullscreen);
	virtual void OnDestroy();

	void UpdateViewport(FLOAT topLeftX, FLOAT topLeftY, FLOAT width, FLOAT height, FLOAT minDepth = D3D12_DEFAULT_VIEWPORT_MIN_DEPTH, FLOAT maxDepth = D3D12_DEFAULT_VIEWPORT_MAX_DEPTH); // Used to put us into right hand depth space.
	void UpdateScissorRect(LONG left, LONG top, LONG right, LONG bottom);
	void UpdateStencilRef(UINT ref);

	void ReadPixels(int x, int y, int width, int height, UINT readBuffer, byte* buffer);

	// Shaders
	void LoadPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc, ID3D12PipelineState** ppPipelineState);
	void SetActivePipelineState(ID3D12PipelineState* pipelineState);

	void Uniform4f(UINT index, const float* uniform);

	// Buffers
	DX12VertexBuffer* AllocVertexBuffer(DX12VertexBuffer* buffer, UINT numBytes);
	void FreeVertexBuffer(DX12VertexBuffer* buffer);

	DX12IndexBuffer* AllocIndexBuffer(DX12IndexBuffer* buffer, UINT numBytes);
	void FreeIndexBuffer(DX12IndexBuffer* buffer);

	DX12JointBuffer* AllocJointBuffer(DX12JointBuffer* buffer, UINT numBytes);
	void FreeJointBuffer(DX12JointBuffer* buffer);

	// Textures
	void SetActiveTextureRegister(UINT8 index);
	DX12TextureBuffer* AllocTextureBuffer(DX12TextureBuffer* buffer, D3D12_RESOURCE_DESC* textureDesc, const idStr* name);
	void FreeTextureBuffer(DX12TextureBuffer* buffer);
	void SetTextureContent(DX12TextureBuffer* buffer, const UINT mipLevel, const UINT bytesPerRow, const size_t imageSize, const void* image);
	void SetTexture(const DX12TextureBuffer* buffer);

	// Draw commands
	void BeginDraw();
	void Clear(bool color, bool depth, bool stencil, byte stencilValue, float* colorRGBA);
	void EndDraw();
	void PresentBackbuffer();
	void ResetCommandList(bool waitForBackBuffer = false);
	void ExecuteCommandList();
	UINT StartSurfaceSettings(); // Starts a new heap entry for the surface.
	void EndSurfaceSettings(); // Records the the surface entry into the heap.
	void DrawModel(DX12VertexBuffer* vertexBuffer, UINT vertexOffset, DX12IndexBuffer* indexBuffer, UINT indexOffset, UINT indexCount);

private:
	UINT m_width;
	UINT m_height;
	int m_fullScreen; // 0 = windowed, otherwise 1 based monitor number to go full screen on
						// -1 = borderless window for spanning multiple displays

	FLOAT m_aspectRatio = 1.0f;
    FLOAT m_FoV = 90.0f;

	bool m_isDrawing = false;
	bool m_initialized = false;

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

	ComPtr<ID3D12DescriptorHeap> m_cbvHeap[FrameCount];
	ComPtr<ID3D12Resource> m_cbvUploadHeap[FrameCount];
	UINT m_cbvHeapIncrementor;
	UINT m_cbvHeapIndex;
	XMFLOAT4 m_constantBuffer[53];
	UINT8* m_constantBufferGPUAddress[FrameCount];
	ID3D12PipelineState* m_activePipelineState = nullptr;
	UINT m_stencilRef = 0;

	// Synchronization
	UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT16 m_fenceValue;

	// Textures
	ComPtr<ID3D12Resource> m_textureBufferUploadHeap;
	UINT8 m_activeTextureRegister;
	const DX12TextureBuffer* m_activeTextures[TEXTURE_REGISTER_COUNT];

	void ThrowIfFailed(HRESULT hr);
	bool WarnIfFailed(HRESULT hr);

	void LoadPipeline(HWND hWnd);
	void LoadAssets();

    void WaitForPreviousFrame();

	bool CreateBackBuffer();


};

extern DX12Renderer dxRenderer;

#endif