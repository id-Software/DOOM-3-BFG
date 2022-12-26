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
	width = deviceParms.backBufferWidth;
	height = deviceParms.backBufferHeight;
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
	return deviceParms;
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

void DeviceManager::AddRenderPassToBack( IRenderPass* pRenderPass )
{
	renderPasses.Remove( pRenderPass );
	renderPasses.Append( pRenderPass );

	pRenderPass->BackBufferResizing();
	pRenderPass->BackBufferResized(
		deviceParms.backBufferWidth,
		deviceParms.backBufferHeight,
		deviceParms.swapChainSampleCount );
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
