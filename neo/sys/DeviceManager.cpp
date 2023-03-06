/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
* Copyright (C) 2022 Stephen Pridham (id Tech 4x integration)
* Copyright (C) 2023 Stephen Saunders (id Tech 4x integration)
* Copyright (C) 2023 Robert Beckebans (id Tech 4x integration)
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

#include "precompiled.h"
#pragma hdrstop

#include "DeviceManager.h"

// Either move RenderPass to sys or move window resizing logic
#include "renderer/RenderPass.h"

DeviceManager* DeviceManager::Create( nvrhi::GraphicsAPI api )
{
	switch( api )
	{
#if USE_DX11
		case nvrhi::GraphicsAPI::D3D11:
			return CreateD3D11();
#endif
#if USE_DX12
		case nvrhi::GraphicsAPI::D3D12:
			return CreateD3D12();
#endif
#if USE_VK
		case nvrhi::GraphicsAPI::VULKAN:
			return CreateVK();
#endif
		default:
			common->Error( "DeviceManager::Create: Unsupported Graphics API (%u)", ( unsigned int )api );
			return nullptr;
	}
}

void DeviceManager::GetWindowDimensions( int& width, int& height )
{
	width = m_DeviceParams.backBufferWidth;
	height = m_DeviceParams.backBufferHeight;
}

void DeviceManager::BackBufferResizing()
{
	Framebuffer::Shutdown();
}

void DeviceManager::BackBufferResized()
{
	if( tr.IsInitialized() )
	{
		Framebuffer::ResizeFramebuffers();
	}
}

const DeviceCreationParameters& DeviceManager::GetDeviceParams()
{
	return m_DeviceParams;
}

nvrhi::IFramebuffer* DeviceManager::GetCurrentFramebuffer()
{
	return GetFramebuffer( GetCurrentBackBufferIndex() );
}

nvrhi::IFramebuffer* DeviceManager::GetFramebuffer( uint32_t index )
{
	if( index < globalFramebuffers.swapFramebuffers.Num() )
	{
		return globalFramebuffers.swapFramebuffers[index]->GetApiObject();
	}

	return nullptr;
}

DeviceManager* DeviceManager::CreateD3D11()
{
	return nullptr;
}

DefaultMessageCallback& DefaultMessageCallback::GetInstance()
{
	static DefaultMessageCallback instance;
	return instance;
}

void DefaultMessageCallback::message( nvrhi::MessageSeverity severity, const char* messageText )
{
	switch( severity )
	{
		case nvrhi::MessageSeverity::Info:
			common->Printf( messageText );
			break;
		case nvrhi::MessageSeverity::Warning:
			common->Warning( messageText );
			break;
		case nvrhi::MessageSeverity::Error:
			common->FatalError( "%s", messageText );
			break;
		case nvrhi::MessageSeverity::Fatal:
			common->FatalError( "%s", messageText );
			break;
	}
}
