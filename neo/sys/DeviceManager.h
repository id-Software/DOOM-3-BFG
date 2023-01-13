/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Stephen Pridham

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef SYS_DEVICE_MANAGER_H_
#define SYS_DEVICE_MANAGER_H_

#if USE_DX11 || USE_DX12
	#include <DXGI.h>
#endif

#if USE_DX11
	#include <d3d11.h>
#endif

#if USE_DX12
	#include <d3d12.h>
#endif

#if USE_VK
	#include <nvrhi/vulkan.h>
#endif

struct DeviceCreationParameters
{
	bool startMaximized = false;
	bool startFullscreen = false;
	bool allowModeSwitch = true;
	int windowPosX = -1;            // -1 means use default placement
	int windowPosY = -1;
	uint32_t backBufferWidth = 1280;
	uint32_t backBufferHeight = 720;
	uint32_t backBufferSampleCount = 1;  // optional HDR Framebuffer MSAA
	uint32_t refreshRate = 0;
	uint32_t swapChainBufferCount = 3;
	nvrhi::Format swapChainFormat = nvrhi::Format::RGBA8_UNORM; // RB: don't do the sRGB gamma ramp with the swapchain
	uint32_t swapChainSampleCount = 1;
	uint32_t swapChainSampleQuality = 0;
	uint32_t maxFramesInFlight = 2;
	bool enableDebugRuntime = false;
	bool enableNvrhiValidationLayer = false;
	bool vsyncEnabled = false;
	bool enableRayTracingExtensions = false; // for vulkan
	bool enableComputeQueue = false;
	bool enableCopyQueue = false;

#if _WIN32
	HINSTANCE	hInstance;
	HWND		hWnd;
#endif

#if USE_DX11 || USE_DX12
	// Adapter to create the device on. Setting this to non-null overrides adapterNameSubstring.
	// If device creation fails on the specified adapter, it will *not* try any other adapters.
	IDXGIAdapter* adapter = nullptr;
#endif

	// For use in the case of multiple adapters; only effective if 'adapter' is null. If this is non-null, device creation will try to match
	// the given string against an adapter name.  If the specified string exists as a sub-string of the
	// adapter name, the device and window will be created on that adapter.  Case sensitive.
	std::wstring adapterNameSubstring = L"";

	// set to true to enable DPI scale factors to be computed per monitor
	// this will keep the on-screen window size in pixels constant
	//
	// if set to false, the DPI scale factors will be constant but the system
	// may scale the contents of the window based on DPI
	//
	// note that the backbuffer size is never updated automatically; if the app
	// wishes to scale up rendering based on DPI, then it must set this to true
	// and respond to DPI scale factor changes by resizing the backbuffer explicitly
	bool enablePerMonitorDPI = false;

#if USE_DX11 || USE_DX12
	DXGI_USAGE swapChainUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
#endif

#if USE_VK
	std::vector<std::string> requiredVulkanInstanceExtensions;
	std::vector<std::string> requiredVulkanDeviceExtensions;
	std::vector<std::string> requiredVulkanLayers;
	std::vector<std::string> optionalVulkanInstanceExtensions;
	std::vector<std::string> optionalVulkanDeviceExtensions;
	std::vector<std::string> optionalVulkanLayers;
	std::vector<size_t> ignoredVulkanValidationMessageLocations;
#endif

	// SRS - Used by idImage::AllocImage() to determine if format D24S8 is supported by device (default = true)
	bool enableImageFormatD24S8 = true;
};

struct DefaultMessageCallback : public nvrhi::IMessageCallback
{
	static DefaultMessageCallback& GetInstance();

	void message( nvrhi::MessageSeverity severity, const char* messageText ) override;
};

class IRenderPass;

class idRenderBackend;

/// Initialize the window and graphics device.
class DeviceManager
{
public:
	static DeviceManager* Create( nvrhi::GraphicsAPI api );

#if USE_VK && defined( VULKAN_USE_PLATFORM_SDL )
	// SRS - Helper method for creating SDL Vulkan surface within DeviceManager_VK()
	vk::Result CreateSDLWindowSurface( vk::Instance instance, vk::SurfaceKHR* surface );
#endif

	bool CreateWindowDeviceAndSwapChain( const glimpParms_t& params, const char* windowTitle );

	// returns the size of the window in screen coordinates
	void GetWindowDimensions( int& width, int& height );

	// returns the screen coordinate to pixel coordinate scale factor
	void GetDPIScaleInfo( float& x, float& y ) const
	{
		x = dpiScaleFactorX;
		y = dpiScaleFactorY;
	}

	void UpdateWindowSize( const glimpParms_t& params );

protected:
	friend class idRenderBackend;
	friend class idImage;

	void* windowInstance;
	void* windowHandle;
	bool windowVisible = false;
	bool isNvidia = false;

	DeviceCreationParameters deviceParms;

	float dpiScaleFactorX = 1.f;
	float dpiScaleFactorY = 1.f;
	bool requestedVSync = false;

	uint32_t m_FrameIndex = 0;

	idList<IRenderPass*> renderPasses;

	DeviceManager() = default;

	void BackBufferResizing();
	void BackBufferResized();

	// device-specific methods
	virtual bool CreateDeviceAndSwapChain() = 0;
	virtual void DestroyDeviceAndSwapChain() = 0;
	virtual void ResizeSwapChain() = 0;
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0; // RB: added for BFG edition
	virtual void Present() = 0;

public:
	[[nodiscard]] virtual nvrhi::IDevice* GetDevice() const = 0;
	[[nodiscard]] virtual const char* GetRendererString() const = 0;
	[[nodiscard]] virtual nvrhi::GraphicsAPI GetGraphicsAPI() const = 0;

	const DeviceCreationParameters& GetDeviceParams();
	virtual void SetVsyncEnabled( bool enabled )
	{
		requestedVSync = enabled; /* will be processed later */
	}
	virtual void ReportLiveObjects() {}

	[[nodiscard]] uint32_t GetFrameIndex() const
	{
		return m_FrameIndex;
	}

	virtual nvrhi::ITexture* GetCurrentBackBuffer() = 0;
	virtual nvrhi::ITexture* GetBackBuffer( uint32_t index ) = 0;
	virtual uint32_t GetCurrentBackBufferIndex() = 0;
	virtual uint32_t GetBackBufferCount() = 0;

	nvrhi::IFramebuffer* GetCurrentFramebuffer();
	nvrhi::IFramebuffer* GetFramebuffer( uint32_t index );

	void AddRenderPassToBack( IRenderPass* pRenderPass );

	void Shutdown();
	virtual ~DeviceManager() = default;

	void SetWindowTitle( const char* title );

	virtual bool IsVulkanInstanceExtensionEnabled( const char* extensionName ) const
	{
		return false;
	}
	virtual bool IsVulkanDeviceExtensionEnabled( const char* extensionName ) const
	{
		return false;
	}
	virtual bool IsVulkanLayerEnabled( const char* layerName ) const
	{
		return false;
	}
	virtual void GetEnabledVulkanInstanceExtensions( std::vector<std::string>& extensions ) const { }
	virtual void GetEnabledVulkanDeviceExtensions( std::vector<std::string>& extensions ) const { }
	virtual void GetEnabledVulkanLayers( std::vector<std::string>& layers ) const { }

private:
	static DeviceManager* CreateD3D11();
	static DeviceManager* CreateD3D12();
	static DeviceManager* CreateVK();

	std::string m_WindowTitle;
};

#endif
