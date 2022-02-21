#ifndef RENDER_PASS_H_
#define RENDER_PASS_H_

#include "sys/DeviceManager.h"

class IRenderPass
{
private:
	DeviceManager* deviceManager;

public:
	explicit IRenderPass( DeviceManager* deviceManager )
		: deviceManager( deviceManager )
	{ }

	virtual ~IRenderPass() = default;

	virtual void RenderFrontend() { }
	virtual void Render( nvrhi::IFramebuffer* framebuffer ) { }
	virtual void Animate( float fElapsedTimeSeconds ) { }
	virtual void BackBufferResizing() { }
	virtual void BackBufferResized( const uint32_t width, const uint32_t height, const uint32_t sampleCount ) { }

	[[nodiscard]] DeviceManager* GetDeviceManager() const
	{
		return deviceManager;
	}
	[[nodiscard]] nvrhi::IDevice* GetDevice() const
	{
		return deviceManager->GetDevice();
	}
	[[nodiscard]] uint32_t GetFrameIndex() const
	{
		return deviceManager->GetFrameIndex();
	}
};


class BasicTriangle : public IRenderPass
{
private:
	nvrhi::ShaderHandle             vertexShader;
	nvrhi::ShaderHandle             pixelShader;
	nvrhi::GraphicsPipelineHandle   pipeline;
	nvrhi::CommandListHandle        commandList;
	nvrhi::InputLayoutHandle        inputLayout;
	nvrhi::BindingLayoutHandle      bindingLayout;
	nvrhi::BindingSetHandle         bindingSet;

	vertCacheHandle_t			    vertexBlock;
	vertCacheHandle_t			    indexBlock;
	idDrawVert*                     vertexPointer;
	triIndex_t*                     indexPointer;
	const idMaterial*               material;

	int							    numVerts;
	int							    numIndexes;

	idDrawVert* AllocVerts( int vertCount, triIndex_t* tempIndexes, int indexCount );

public:
	using IRenderPass::IRenderPass;

	bool Init();
	void BackBufferResizing() override;
	void Animate( float fElapsedTimeSeconds ) override;
	void RenderFrontend() override;
	void Render( nvrhi::IFramebuffer* framebuffer ) override;
};

#endif