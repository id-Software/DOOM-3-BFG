/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
* Copyright (C) 2022 Stephen Pridham (id Tech 4x integration)
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

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