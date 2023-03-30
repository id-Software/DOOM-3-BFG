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

// SRS - Disable PCH here, otherwise get Vulkan header mismatch failures with USE_AMD_ALLOCATOR option
//#include <precompiled.h>
//#pragma hdrstop

#include <string>
#include <queue>
#include <unordered_set>

#include "renderer/RenderCommon.h"
#include <sys/DeviceManager.h>

#include <nvrhi/vulkan.h>
// SRS - optionally needed for VK_MVK_MOLTENVK_EXTENSION_NAME and MoltenVK runtime config visibility
#if defined(__APPLE__) && defined( USE_MoltenVK )
	#include <MoltenVK/vk_mvk_moltenvk.h>

	idCVar r_mvkSynchronousQueueSubmits( "r_mvkSynchronousQueueSubmits", "0", CVAR_BOOL | CVAR_INIT, "Use MoltenVK's synchronous queue submit option." );
#endif
#include <nvrhi/validation.h>

#if defined( USE_AMD_ALLOCATOR )
	#define VMA_IMPLEMENTATION
	#define VMA_STATIC_VULKAN_FUNCTIONS 0
	#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
	#include "vk_mem_alloc.h"

	VmaAllocator m_VmaAllocator = nullptr;

	idCVar r_vmaDeviceLocalMemoryMB( "r_vmaDeviceLocalMemoryMB", "256", CVAR_INTEGER | CVAR_INIT, "Size of VMA allocation block for gpu memory." );
#endif

// Define the Vulkan dynamic dispatcher - this needs to occur in exactly one cpp file in the program.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

class DeviceManager_VK : public DeviceManager
{
public:
	[[nodiscard]] nvrhi::IDevice* GetDevice() const override
	{
		if( m_ValidationLayer )
		{
			return m_ValidationLayer;
		}

		return m_NvrhiDevice;
	}

	[[nodiscard]] nvrhi::GraphicsAPI GetGraphicsAPI() const override
	{
		return nvrhi::GraphicsAPI::VULKAN;
	}

protected:
	bool CreateDeviceAndSwapChain() override;
	void DestroyDeviceAndSwapChain() override;

	void ResizeSwapChain() override
	{
		if( m_VulkanDevice )
		{
			destroySwapChain();
			createSwapChain();
		}
	}

	nvrhi::ITexture* GetCurrentBackBuffer() override
	{
		return m_SwapChainImages[m_SwapChainIndex].rhiHandle;
	}
	nvrhi::ITexture* GetBackBuffer( uint32_t index ) override
	{
		if( index < m_SwapChainImages.size() )
		{
			return m_SwapChainImages[index].rhiHandle;
		}
		return nullptr;
	}
	uint32_t GetCurrentBackBufferIndex() override
	{
		return m_SwapChainIndex;
	}
	uint32_t GetBackBufferCount() override
	{
		return uint32_t( m_SwapChainImages.size() );
	}

	void BeginFrame() override;
	void EndFrame() override;
	void Present() override;

	const char* GetRendererString() const override
	{
		return m_RendererString.c_str();
	}

	bool IsVulkanInstanceExtensionEnabled( const char* extensionName ) const override
	{
		return enabledExtensions.instance.find( extensionName ) != enabledExtensions.instance.end();
	}

	bool IsVulkanDeviceExtensionEnabled( const char* extensionName ) const override
	{
		return enabledExtensions.device.find( extensionName ) != enabledExtensions.device.end();
	}

	bool IsVulkanLayerEnabled( const char* layerName ) const override
	{
		return enabledExtensions.layers.find( layerName ) != enabledExtensions.layers.end();
	}

	void GetEnabledVulkanInstanceExtensions( std::vector<std::string>& extensions ) const override
	{
		for( const auto& ext : enabledExtensions.instance )
		{
			extensions.push_back( ext );
		}
	}

	void GetEnabledVulkanDeviceExtensions( std::vector<std::string>& extensions ) const override
	{
		for( const auto& ext : enabledExtensions.device )
		{
			extensions.push_back( ext );
		}
	}

	void GetEnabledVulkanLayers( std::vector<std::string>& layers ) const override
	{
		for( const auto& ext : enabledExtensions.layers )
		{
			layers.push_back( ext );
		}
	}

private:
	bool createInstance();
	bool createWindowSurface();
	void installDebugCallback();
	bool pickPhysicalDevice();
	bool findQueueFamilies( vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface );
	bool createDevice();
	bool createSwapChain();
	void destroySwapChain();

	struct VulkanExtensionSet
	{
		std::unordered_set<std::string> instance;
		std::unordered_set<std::string> layers;
		std::unordered_set<std::string> device;
	};

	// minimal set of required extensions
	VulkanExtensionSet enabledExtensions =
	{
		// instance
		{
#if defined(__APPLE__) && defined( USE_MoltenVK )
			// SRS - needed for using MoltenVK configuration on macOS (if USE_MoltenVK defined)
			VK_MVK_MOLTENVK_EXTENSION_NAME,
#endif
			VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
		},
		// layers
		{ },
		// device
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#if defined(__APPLE__)
#if defined( VK_KHR_portability_subset )
			VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif
			VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
			VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
#endif
		},
	};

	// optional extensions
	VulkanExtensionSet optionalExtensions =
	{
		// instance
		{
#if defined(__APPLE__) && defined( VK_KHR_portability_enumeration )
			// SRS - This is optional since it only became manadatory with Vulkan SDK 1.3.216.0 or later
			VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
			VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		},
		// layers
		{
#if defined(__APPLE__)
			// SRS - synchronization2 not supported natively on MoltenVK, use layer implementation instead
			"VK_LAYER_KHRONOS_synchronization2"
#endif
		},
		// device
		{
			VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_NV_MESH_SHADER_EXTENSION_NAME,
			VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
			VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
		},
	};

	std::unordered_set<std::string> m_RayTracingExtensions =
	{
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
		VK_KHR_RAY_QUERY_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
	};

	std::string m_RendererString;

	vk::Instance m_VulkanInstance;
	vk::DebugReportCallbackEXT m_DebugReportCallback;

	vk::PhysicalDevice m_VulkanPhysicalDevice;
	int m_GraphicsQueueFamily = -1;
	int m_ComputeQueueFamily = -1;
	int m_TransferQueueFamily = -1;
	int m_PresentQueueFamily = -1;

	vk::Device m_VulkanDevice;
	vk::Queue m_GraphicsQueue;
	vk::Queue m_ComputeQueue;
	vk::Queue m_TransferQueue;
	vk::Queue m_PresentQueue;

	vk::SurfaceKHR m_WindowSurface;

	vk::SurfaceFormatKHR m_SwapChainFormat;
	vk::SwapchainKHR m_SwapChain;

	struct SwapChainImage
	{
		vk::Image image;
		nvrhi::TextureHandle rhiHandle;
	};

	std::vector<SwapChainImage> m_SwapChainImages;
	uint32_t m_SwapChainIndex = uint32_t( -1 );

	nvrhi::vulkan::DeviceHandle m_NvrhiDevice;
	nvrhi::DeviceHandle m_ValidationLayer;

	nvrhi::CommandListHandle m_BarrierCommandList;
	std::queue<vk::Semaphore> m_PresentSemaphoreQueue;
	vk::Semaphore m_PresentSemaphore;

	nvrhi::EventQueryHandle m_FrameWaitQuery;

	// SRS - flag indicating support for eFifoRelaxed surface presentation (r_swapInterval = 1) mode
	bool enablePModeFifoRelaxed = false;


private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData )
	{
		const DeviceManager_VK* manager = ( const DeviceManager_VK* )userData;

		if( manager )
		{
			const auto& ignored = manager->m_DeviceParams.ignoredVulkanValidationMessageLocations;
			const auto found = std::find( ignored.begin(), ignored.end(), location );
			if( found != ignored.end() )
			{
				return VK_FALSE;
			}
		}

		if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
		{
			idLib::Printf( "[Vulkan] ERROR location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg );
		}
		else if( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT )
		{
			idLib::Printf( "[Vulkan] WARNING location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg );
		}
		else if( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT )
		{
			idLib::Printf( "[Vulkan] PERFORMANCE WARNING location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg );
		}
		else if( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT )
		{
			idLib::Printf( "[Vulkan] INFO location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg );
		}
		else if( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT )
		{
			idLib::Printf( "[Vulkan] DEBUG location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg );
		}

		return VK_FALSE;
	}
};

static std::vector<const char*> stringSetToVector( const std::unordered_set<std::string>& set )
{
	std::vector<const char*> ret;
	for( const auto& s : set )
	{
		ret.push_back( s.c_str() );
	}

	return ret;
}

template <typename T>
static std::vector<T> setToVector( const std::unordered_set<T>& set )
{
	std::vector<T> ret;
	for( const auto& s : set )
	{
		ret.push_back( s );
	}

	return ret;
}

bool DeviceManager_VK::createInstance()
{
#if defined( VULKAN_USE_PLATFORM_SDL )
	// SRS - Populate enabledExtensions with required SDL instance extensions
	auto sdl_instanceExtensions = get_required_extensions();
	for( auto instanceExtension : sdl_instanceExtensions )
	{
		enabledExtensions.instance.insert( instanceExtension );
	}
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
	enabledExtensions.instance.insert( VK_KHR_SURFACE_EXTENSION_NAME );
	enabledExtensions.instance.insert( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#endif

	// add instance extensions requested by the user
	for( const std::string& name : m_DeviceParams.requiredVulkanInstanceExtensions )
	{
		enabledExtensions.instance.insert( name );
	}
	for( const std::string& name : m_DeviceParams.optionalVulkanInstanceExtensions )
	{
		optionalExtensions.instance.insert( name );
	}

	// add layers requested by the user
	for( const std::string& name : m_DeviceParams.requiredVulkanLayers )
	{
		enabledExtensions.layers.insert( name );
	}
	for( const std::string& name : m_DeviceParams.optionalVulkanLayers )
	{
		optionalExtensions.layers.insert( name );
	}

	std::unordered_set<std::string> requiredExtensions = enabledExtensions.instance;

	// figure out which optional extensions are supported
	for( const auto& instanceExt : vk::enumerateInstanceExtensionProperties() )
	{
		const std::string name = instanceExt.extensionName;
		if( optionalExtensions.instance.find( name ) != optionalExtensions.instance.end() )
		{
			enabledExtensions.instance.insert( name );
		}

		requiredExtensions.erase( name );
	}

	if( !requiredExtensions.empty() )
	{
		std::stringstream ss;
		ss << "Cannot create a Vulkan instance because the following required extension(s) are not supported:";
		for( const auto& ext : requiredExtensions )
		{
			ss << std::endl << "  - " << ext;
		}

		common->FatalError( "%s", ss.str().c_str() );
		return false;
	}

	common->Printf( "Enabled Vulkan instance extensions:\n" );
	for( const auto& ext : enabledExtensions.instance )
	{
		common->Printf( "    %s\n", ext.c_str() );
	}

	std::unordered_set<std::string> requiredLayers = enabledExtensions.layers;

	for( const auto& layer : vk::enumerateInstanceLayerProperties() )
	{
		const std::string name = layer.layerName;
		if( optionalExtensions.layers.find( name ) != optionalExtensions.layers.end() )
		{
			enabledExtensions.layers.insert( name );
		}

		requiredLayers.erase( name );
	}

	if( !requiredLayers.empty() )
	{
		std::stringstream ss;
		ss << "Cannot create a Vulkan instance because the following required layer(s) are not supported:";
		for( const auto& ext : requiredLayers )
		{
			ss << std::endl << "  - " << ext;
		}

		common->FatalError( "%s", ss.str().c_str() );
		return false;
	}

	common->Printf( "Enabled Vulkan layers:\n" );
	for( const auto& layer : enabledExtensions.layers )
	{
		common->Printf( "    %s\n", layer.c_str() );
	}

	auto instanceExtVec = stringSetToVector( enabledExtensions.instance );
	auto layerVec = stringSetToVector( enabledExtensions.layers );

	auto applicationInfo = vk::ApplicationInfo()
						   .setApiVersion( VK_MAKE_VERSION( 1, 2, 0 ) );

	// create the vulkan instance
	vk::InstanceCreateInfo info = vk::InstanceCreateInfo()
								  .setEnabledLayerCount( uint32_t( layerVec.size() ) )
								  .setPpEnabledLayerNames( layerVec.data() )
								  .setEnabledExtensionCount( uint32_t( instanceExtVec.size() ) )
								  .setPpEnabledExtensionNames( instanceExtVec.data() )
								  .setPApplicationInfo( &applicationInfo );

#if defined(__APPLE__) && defined( VK_KHR_portability_enumeration )
	if( enabledExtensions.instance.find( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME ) != enabledExtensions.instance.end() )
	{
		info.setFlags( vk::InstanceCreateFlagBits( VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR ) );
	}
#endif

	const vk::Result res = vk::createInstance( &info, nullptr, &m_VulkanInstance );
	if( res != vk::Result::eSuccess )
	{
		common->FatalError( "Failed to create a Vulkan instance, error code = %s", nvrhi::vulkan::resultToString( res ) );
		return false;
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init( m_VulkanInstance );

	return true;
}

void DeviceManager_VK::installDebugCallback()
{
	auto info = vk::DebugReportCallbackCreateInfoEXT()
				.setFlags( vk::DebugReportFlagBitsEXT::eError |
						   vk::DebugReportFlagBitsEXT::eWarning |
						   //   vk::DebugReportFlagBitsEXT::eInformation |
						   vk::DebugReportFlagBitsEXT::ePerformanceWarning )
				.setPfnCallback( vulkanDebugCallback )
				.setPUserData( this );

	const vk::Result res = m_VulkanInstance.createDebugReportCallbackEXT( &info, nullptr, &m_DebugReportCallback );
	assert( res == vk::Result::eSuccess );
}

bool DeviceManager_VK::pickPhysicalDevice()
{
	vk::Format requestedFormat = nvrhi::vulkan::convertFormat( m_DeviceParams.swapChainFormat );
	vk::Extent2D requestedExtent( m_DeviceParams.backBufferWidth, m_DeviceParams.backBufferHeight );

	auto devices = m_VulkanInstance.enumeratePhysicalDevices();

	// Start building an error message in case we cannot find a device.
	std::stringstream errorStream;
	errorStream << "Cannot find a Vulkan device that supports all the required extensions and properties.";

	// build a list of GPUs
	std::vector<vk::PhysicalDevice> discreteGPUs;
	std::vector<vk::PhysicalDevice> otherGPUs;
	for( const auto& dev : devices )
	{
		auto prop = dev.getProperties();

		errorStream << std::endl << prop.deviceName.data() << ":";

		// check that all required device extensions are present
		std::unordered_set<std::string> requiredExtensions = enabledExtensions.device;
		auto deviceExtensions = dev.enumerateDeviceExtensionProperties();
		for( const auto& ext : deviceExtensions )
		{
			requiredExtensions.erase( std::string( ext.extensionName.data() ) );
		}

		bool deviceIsGood = true;

		if( !requiredExtensions.empty() )
		{
			// device is missing one or more required extensions
			for( const auto& ext : requiredExtensions )
			{
				errorStream << std::endl << "  - missing " << ext;
			}
			deviceIsGood = false;
		}

		auto deviceFeatures = dev.getFeatures();
		if( !deviceFeatures.samplerAnisotropy )
		{
			// device is a toaster oven
			errorStream << std::endl << "  - does not support samplerAnisotropy";
			deviceIsGood = false;
		}
		if( !deviceFeatures.textureCompressionBC )
		{
			errorStream << std::endl << "  - does not support textureCompressionBC";
			deviceIsGood = false;
		}

		// check that this device supports our intended swap chain creation parameters
		auto surfaceCaps = dev.getSurfaceCapabilitiesKHR( m_WindowSurface );
		auto surfaceFmts = dev.getSurfaceFormatsKHR( m_WindowSurface );
		auto surfacePModes = dev.getSurfacePresentModesKHR( m_WindowSurface );

		if( surfaceCaps.minImageCount > m_DeviceParams.swapChainBufferCount ||
				( surfaceCaps.maxImageCount < m_DeviceParams.swapChainBufferCount && surfaceCaps.maxImageCount > 0 ) )
		{
			errorStream << std::endl << "  - cannot support the requested swap chain image count:";
			errorStream << " requested " << m_DeviceParams.swapChainBufferCount << ", available " << surfaceCaps.minImageCount << " - " << surfaceCaps.maxImageCount;
			deviceIsGood = false;
		}

		if( surfaceCaps.minImageExtent.width > requestedExtent.width ||
				surfaceCaps.minImageExtent.height > requestedExtent.height ||
				surfaceCaps.maxImageExtent.width < requestedExtent.width ||
				surfaceCaps.maxImageExtent.height < requestedExtent.height )
		{
			errorStream << std::endl << "  - cannot support the requested swap chain size:";
			errorStream << " requested " << requestedExtent.width << "x" << requestedExtent.height << ", ";
			errorStream << " available " << surfaceCaps.minImageExtent.width << "x" << surfaceCaps.minImageExtent.height;
			errorStream << " - " << surfaceCaps.maxImageExtent.width << "x" << surfaceCaps.maxImageExtent.height;
			deviceIsGood = false;
		}

		bool surfaceFormatPresent = false;
		for( const vk::SurfaceFormatKHR& surfaceFmt : surfaceFmts )
		{
			if( surfaceFmt.format == requestedFormat )
			{
				surfaceFormatPresent = true;
				break;
			}
		}

		if( !surfaceFormatPresent )
		{
			// can't create a swap chain using the format requested
			errorStream << std::endl << "  - does not support the requested swap chain format";
			deviceIsGood = false;
		}

		if( ( find( surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eImmediate ) == surfacePModes.end() ) ||
				( find( surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eFifo ) == surfacePModes.end() ) )
		{
			// can't find the required surface present modes
			errorStream << std::endl << "  - does not support the requested surface present modes";
			deviceIsGood = false;
		}

		if( !findQueueFamilies( dev, m_WindowSurface ) )
		{
			// device doesn't have all the queue families we need
			errorStream << std::endl << "  - does not support the necessary queue types";
			deviceIsGood = false;
		}

		// check that we can present from the graphics queue
		uint32_t canPresent = dev.getSurfaceSupportKHR( m_GraphicsQueueFamily, m_WindowSurface );
		if( !canPresent )
		{
			errorStream << std::endl << "  - cannot present";
			deviceIsGood = false;
		}

		if( !deviceIsGood )
		{
			continue;
		}

		if( prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
		{
			discreteGPUs.push_back( dev );
		}
		else
		{
			otherGPUs.push_back( dev );
		}
	}

	// pick the first discrete GPU if it exists, otherwise the first integrated GPU
	if( !discreteGPUs.empty() )
	{
		m_VulkanPhysicalDevice = discreteGPUs[0];
		return true;
	}

	if( !otherGPUs.empty() )
	{
		m_VulkanPhysicalDevice = otherGPUs[0];
		return true;
	}

	common->FatalError( "%s", errorStream.str().c_str() );

	return false;
}

bool DeviceManager_VK::findQueueFamilies( vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface )
{
	auto props = physicalDevice.getQueueFamilyProperties();

	for( int i = 0; i < int( props.size() ); i++ )
	{
		const auto& queueFamily = props[i];

		if( m_GraphicsQueueFamily == -1 )
		{
			if( queueFamily.queueCount > 0 &&
					( queueFamily.queueFlags & vk::QueueFlagBits::eGraphics ) )
			{
				m_GraphicsQueueFamily = i;
			}
		}

		if( m_ComputeQueueFamily == -1 )
		{
			if( queueFamily.queueCount > 0 &&
					( queueFamily.queueFlags & vk::QueueFlagBits::eCompute ) &&
					!( queueFamily.queueFlags & vk::QueueFlagBits::eGraphics ) )
			{
				m_ComputeQueueFamily = i;
			}
		}

		if( m_TransferQueueFamily == -1 )
		{
			if( queueFamily.queueCount > 0 &&
					( queueFamily.queueFlags & vk::QueueFlagBits::eTransfer ) &&
					!( queueFamily.queueFlags & vk::QueueFlagBits::eCompute ) &&
					!( queueFamily.queueFlags & vk::QueueFlagBits::eGraphics ) )
			{
				m_TransferQueueFamily = i;
			}
		}

		if( m_PresentQueueFamily == -1 )
		{
			vk::Bool32 presentSupported;
			// SRS - Use portable implmentation for detecting presentation support vs. Windows-specific Vulkan call
			if( queueFamily.queueCount > 0 &&
					physicalDevice.getSurfaceSupportKHR( i, surface, &presentSupported ) == vk::Result::eSuccess )
			{
				if( presentSupported )
				{
					m_PresentQueueFamily = i;
				}
			}
		}
	}

	if( m_GraphicsQueueFamily == -1 ||
			m_PresentQueueFamily == -1 ||
			( m_ComputeQueueFamily == -1 && m_DeviceParams.enableComputeQueue ) ||
			( m_TransferQueueFamily == -1 && m_DeviceParams.enableCopyQueue ) )
	{
		return false;
	}

	return true;
}

bool DeviceManager_VK::createDevice()
{
	// figure out which optional extensions are supported
	auto deviceExtensions = m_VulkanPhysicalDevice.enumerateDeviceExtensionProperties();
	for( const auto& ext : deviceExtensions )
	{
		const std::string name = ext.extensionName;
		if( optionalExtensions.device.find( name ) != optionalExtensions.device.end() )
		{
			enabledExtensions.device.insert( name );
		}

		if( m_DeviceParams.enableRayTracingExtensions && m_RayTracingExtensions.find( name ) != m_RayTracingExtensions.end() )
		{
			enabledExtensions.device.insert( name );
		}
	}

	bool accelStructSupported = false;
	bool bufferAddressSupported = false;
	bool rayPipelineSupported = false;
	bool rayQuerySupported = false;
	bool meshletsSupported = false;
	bool vrsSupported = false;
	bool sync2Supported = false;

	common->Printf( "Enabled Vulkan device extensions:\n" );
	for( const auto& ext : enabledExtensions.device )
	{
		common->Printf( "    %s\n", ext.c_str() );

		if( ext == VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME )
		{
			accelStructSupported = true;
		}
		else if( ext == VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME )
		{
			// RB: only makes problems at the moment
			bufferAddressSupported = true;
		}
		else if( ext == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME )
		{
			rayPipelineSupported = true;
		}
		else if( ext == VK_KHR_RAY_QUERY_EXTENSION_NAME )
		{
			rayQuerySupported = true;
		}
		else if( ext == VK_NV_MESH_SHADER_EXTENSION_NAME )
		{
			meshletsSupported = true;
		}
		else if( ext == VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME )
		{
			vrsSupported = true;
		}
		else if( ext == VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME )
		{
			sync2Supported = true;
		}
	}

	std::unordered_set<int> uniqueQueueFamilies =
	{
		m_GraphicsQueueFamily,
		m_PresentQueueFamily
	};

	if( m_DeviceParams.enableComputeQueue )
	{
		uniqueQueueFamilies.insert( m_ComputeQueueFamily );
	}

	if( m_DeviceParams.enableCopyQueue )
	{
		uniqueQueueFamilies.insert( m_TransferQueueFamily );
	}

	float priority = 1.f;
	std::vector<vk::DeviceQueueCreateInfo> queueDesc;
	for( int queueFamily : uniqueQueueFamilies )
	{
		queueDesc.push_back( vk::DeviceQueueCreateInfo()
							 .setQueueFamilyIndex( queueFamily )
							 .setQueueCount( 1 )
							 .setPQueuePriorities( &priority ) );
	}

	auto accelStructFeatures = vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
							   .setAccelerationStructure( true );
	auto bufferAddressFeatures = vk::PhysicalDeviceBufferAddressFeaturesEXT()
								 .setBufferDeviceAddress( true );
	auto rayPipelineFeatures = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
							   .setRayTracingPipeline( true )
							   .setRayTraversalPrimitiveCulling( true );
	auto rayQueryFeatures = vk::PhysicalDeviceRayQueryFeaturesKHR()
							.setRayQuery( true );
	auto meshletFeatures = vk::PhysicalDeviceMeshShaderFeaturesNV()
						   .setTaskShader( true )
						   .setMeshShader( true );
	auto vrsFeatures = vk::PhysicalDeviceFragmentShadingRateFeaturesKHR()
					   .setPipelineFragmentShadingRate( true )
					   .setPrimitiveFragmentShadingRate( true )
					   .setAttachmentFragmentShadingRate( true );

	auto sync2Features = vk::PhysicalDeviceSynchronization2FeaturesKHR()
						 .setSynchronization2( true );

#if defined(__APPLE__) && defined( VK_KHR_portability_subset )
	auto portabilityFeatures = vk::PhysicalDevicePortabilitySubsetFeaturesKHR()
							   .setImageViewFormatSwizzle( true );

	void* pNext = &portabilityFeatures;
#else
	void* pNext = nullptr;
#endif
#define APPEND_EXTENSION(condition, desc) if (condition) { (desc).pNext = pNext; pNext = &(desc); }  // NOLINT(cppcoreguidelines-macro-usage)
	APPEND_EXTENSION( accelStructSupported, accelStructFeatures )
	APPEND_EXTENSION( bufferAddressSupported, bufferAddressFeatures )
	APPEND_EXTENSION( rayPipelineSupported, rayPipelineFeatures )
	APPEND_EXTENSION( rayQuerySupported, rayQueryFeatures )
	APPEND_EXTENSION( meshletsSupported, meshletFeatures )
	APPEND_EXTENSION( vrsSupported, vrsFeatures )
	APPEND_EXTENSION( sync2Supported, sync2Features )
#undef APPEND_EXTENSION

	auto deviceFeatures = vk::PhysicalDeviceFeatures()
						  .setShaderImageGatherExtended( true )
						  .setShaderStorageImageReadWithoutFormat( true )
						  .setSamplerAnisotropy( true )
						  .setTessellationShader( true )
						  .setTextureCompressionBC( true )
#if !defined(__APPLE__)
						  .setGeometryShader( true )
#endif
						  .setFillModeNonSolid( true )
						  .setImageCubeArray( true )
						  .setDualSrcBlend( true );

	auto vulkan12features = vk::PhysicalDeviceVulkan12Features()
							.setDescriptorIndexing( true )
							.setRuntimeDescriptorArray( true )
							.setDescriptorBindingPartiallyBound( true )
							.setDescriptorBindingVariableDescriptorCount( true )
							.setTimelineSemaphore( true )
							.setShaderSampledImageArrayNonUniformIndexing( true )
							.setBufferDeviceAddress( bufferAddressSupported )
							.setPNext( pNext );

	auto layerVec = stringSetToVector( enabledExtensions.layers );
	auto extVec = stringSetToVector( enabledExtensions.device );

	auto deviceDesc = vk::DeviceCreateInfo()
					  .setPQueueCreateInfos( queueDesc.data() )
					  .setQueueCreateInfoCount( uint32_t( queueDesc.size() ) )
					  .setPEnabledFeatures( &deviceFeatures )
					  .setEnabledExtensionCount( uint32_t( extVec.size() ) )
					  .setPpEnabledExtensionNames( extVec.data() )
					  .setEnabledLayerCount( uint32_t( layerVec.size() ) )
					  .setPpEnabledLayerNames( layerVec.data() )
					  .setPNext( &vulkan12features );

	const vk::Result res = m_VulkanPhysicalDevice.createDevice( &deviceDesc, nullptr, &m_VulkanDevice );
	if( res != vk::Result::eSuccess )
	{
		common->FatalError( "Failed to create a Vulkan physical device, error code = %s", nvrhi::vulkan::resultToString( res ) );
		return false;
	}

	m_VulkanDevice.getQueue( m_GraphicsQueueFamily, 0, &m_GraphicsQueue );
	if( m_DeviceParams.enableComputeQueue )
	{
		m_VulkanDevice.getQueue( m_ComputeQueueFamily, 0, &m_ComputeQueue );
	}
	if( m_DeviceParams.enableCopyQueue )
	{
		m_VulkanDevice.getQueue( m_TransferQueueFamily, 0, &m_TransferQueue );
	}
	m_VulkanDevice.getQueue( m_PresentQueueFamily, 0, &m_PresentQueue );

	VULKAN_HPP_DEFAULT_DISPATCHER.init( m_VulkanDevice );

	// SRS - Determine if preferred image depth/stencil format D24S8 is supported (issue with Vulkan on AMD GPUs)
	vk::ImageFormatProperties imageFormatProperties;
	const vk::Result ret = m_VulkanPhysicalDevice.getImageFormatProperties( vk::Format::eD24UnormS8Uint,
						   vk::ImageType::e2D,
						   vk::ImageTiling::eOptimal,
						   vk::ImageUsageFlags( vk::ImageUsageFlagBits::eDepthStencilAttachment ),
						   vk::ImageCreateFlags( 0 ),
						   &imageFormatProperties );
	m_DeviceParams.enableImageFormatD24S8 = ( ret == vk::Result::eSuccess );

	// SRS - Determine if "smart" (r_swapInterval = 1) vsync mode eFifoRelaxed is supported by device and surface
	auto surfacePModes = m_VulkanPhysicalDevice.getSurfacePresentModesKHR( m_WindowSurface );
	enablePModeFifoRelaxed = find( surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eFifoRelaxed ) != surfacePModes.end();

	// stash the renderer string
	auto prop = m_VulkanPhysicalDevice.getProperties();
	m_RendererString = std::string( prop.deviceName.data() );

#if defined( USE_AMD_ALLOCATOR )
	// SRS - initialize the vma allocator
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorCreateInfo.physicalDevice = m_VulkanPhysicalDevice;
	allocatorCreateInfo.device = m_VulkanDevice;
	allocatorCreateInfo.instance = m_VulkanInstance;
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
	allocatorCreateInfo.flags = bufferAddressSupported ? VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT : 0;
	allocatorCreateInfo.preferredLargeHeapBlockSize = r_vmaDeviceLocalMemoryMB.GetInteger() * 1024 * 1024;
	vmaCreateAllocator( &allocatorCreateInfo, &m_VmaAllocator );
#endif

	common->Printf( "Created Vulkan device: %s\n", m_RendererString.c_str() );

	return true;
}

/*
* Vulkan Example base class
*
* Copyright (C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
bool DeviceManager_VK::createWindowSurface()
{
	// Create the platform-specific surface
#if defined( VULKAN_USE_PLATFORM_SDL )
	// SRS - Support generic SDL platform for linux and macOS
	const vk::Result res = CreateSDLWindowSurface( m_VulkanInstance, &m_WindowSurface );

#elif defined( VK_USE_PLATFORM_WIN32_KHR )
	auto surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR()
							 .setHinstance( ( HINSTANCE )windowInstance )
							 .setHwnd( ( HWND )windowHandle );

	const vk::Result res = m_VulkanInstance.createWin32SurfaceKHR( &surfaceCreateInfo, nullptr, &m_WindowSurface );
#endif

	if( res != vk::Result::eSuccess )
	{
		common->FatalError( "Failed to create a Vulkan window surface, error code = %s", nvrhi::vulkan::resultToString( res ) );
		return false;
	}

	return true;
}

void DeviceManager_VK::destroySwapChain()
{
	if( m_VulkanDevice )
	{
		m_VulkanDevice.waitIdle();
	}

	while( !m_SwapChainImages.empty() )
	{
		auto sci = m_SwapChainImages.back();
		m_SwapChainImages.pop_back();
		sci.rhiHandle = nullptr;
	}

	if( m_SwapChain )
	{
		m_VulkanDevice.destroySwapchainKHR( m_SwapChain );
		m_SwapChain = nullptr;
	}
}

bool DeviceManager_VK::createSwapChain()
{
	m_SwapChainFormat =
	{
		vk::Format( nvrhi::vulkan::convertFormat( m_DeviceParams.swapChainFormat ) ),
		vk::ColorSpaceKHR::eSrgbNonlinear
	};

	vk::Extent2D extent = vk::Extent2D( m_DeviceParams.backBufferWidth, m_DeviceParams.backBufferHeight );

	std::unordered_set<uint32_t> uniqueQueues =
	{
		uint32_t( m_GraphicsQueueFamily ),
		uint32_t( m_PresentQueueFamily )
	};

	std::vector<uint32_t> queues = setToVector( uniqueQueues );

	const bool enableSwapChainSharing = queues.size() > 1;

	auto desc = vk::SwapchainCreateInfoKHR()
				.setSurface( m_WindowSurface )
				.setMinImageCount( m_DeviceParams.swapChainBufferCount )
				.setImageFormat( m_SwapChainFormat.format )
				.setImageColorSpace( m_SwapChainFormat.colorSpace )
				.setImageExtent( extent )
				.setImageArrayLayers( 1 )
				.setImageUsage( vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled )
				.setImageSharingMode( enableSwapChainSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive )
				.setQueueFamilyIndexCount( enableSwapChainSharing ? uint32_t( queues.size() ) : 0 )
				.setPQueueFamilyIndices( enableSwapChainSharing ? queues.data() : nullptr )
				.setPreTransform( vk::SurfaceTransformFlagBitsKHR::eIdentity )
				.setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
				.setPresentMode( m_DeviceParams.vsyncEnabled > 0 ? ( m_DeviceParams.vsyncEnabled == 2 || !enablePModeFifoRelaxed ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eFifoRelaxed ) : vk::PresentModeKHR::eImmediate )
				.setClipped( true )
				.setOldSwapchain( nullptr );

	const vk::Result res = m_VulkanDevice.createSwapchainKHR( &desc, nullptr, &m_SwapChain );
	if( res != vk::Result::eSuccess )
	{
		common->FatalError( "Failed to create a Vulkan swap chain, error code = %s", nvrhi::vulkan::resultToString( res ) );
		return false;
	}

	// retrieve swap chain images
	auto images = m_VulkanDevice.getSwapchainImagesKHR( m_SwapChain );
	for( auto image : images )
	{
		SwapChainImage sci;
		sci.image = image;

		nvrhi::TextureDesc textureDesc;
		textureDesc.width = m_DeviceParams.backBufferWidth;
		textureDesc.height = m_DeviceParams.backBufferHeight;
		textureDesc.format = m_DeviceParams.swapChainFormat;
		textureDesc.debugName = "Swap chain image";
		textureDesc.initialState = nvrhi::ResourceStates::Present;
		textureDesc.keepInitialState = true;
		textureDesc.isRenderTarget = true;

		sci.rhiHandle = m_NvrhiDevice->createHandleForNativeTexture( nvrhi::ObjectTypes::VK_Image, nvrhi::Object( sci.image ), textureDesc );
		m_SwapChainImages.push_back( sci );
	}

	m_SwapChainIndex = 0;

	return true;
}

bool DeviceManager_VK::CreateDeviceAndSwapChain()
{
	// RB: control these through the cmdline
	m_DeviceParams.enableNvrhiValidationLayer = r_useValidationLayers.GetInteger() > 0;
	m_DeviceParams.enableDebugRuntime = r_useValidationLayers.GetInteger() > 1;

	if( m_DeviceParams.enableDebugRuntime )
	{
		enabledExtensions.instance.insert( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
#if defined(__APPLE__) && defined( USE_MoltenVK )
		enabledExtensions.layers.insert( "MoltenVK" );
	}

	// SRS - when USE_MoltenVK defined, load libMoltenVK vs. the default libvulkan
	static const vk::DynamicLoader dl( "libMoltenVK.dylib" );
#else
		enabledExtensions.layers.insert( "VK_LAYER_KHRONOS_validation" );
	}

	// SRS - make static so ~DynamicLoader() does not prematurely unload vulkan dynamic lib
	static const vk::DynamicLoader dl;
#endif
	const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =   // NOLINT(misc-misplaced-const)
		dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
	VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

#define CHECK(a) if (!(a)) { return false; }

	CHECK( createInstance() );

	if( m_DeviceParams.enableDebugRuntime )
	{
		installDebugCallback();
	}

	if( m_DeviceParams.swapChainFormat == nvrhi::Format::SRGBA8_UNORM )
	{
		m_DeviceParams.swapChainFormat = nvrhi::Format::SBGRA8_UNORM;
	}
	else if( m_DeviceParams.swapChainFormat == nvrhi::Format::RGBA8_UNORM )
	{
		m_DeviceParams.swapChainFormat = nvrhi::Format::BGRA8_UNORM;
	}

	// add device extensions requested by the user
	for( const std::string& name : m_DeviceParams.requiredVulkanDeviceExtensions )
	{
		enabledExtensions.device.insert( name );
	}
	for( const std::string& name : m_DeviceParams.optionalVulkanDeviceExtensions )
	{
		optionalExtensions.device.insert( name );
	}

	CHECK( createWindowSurface() );
	CHECK( pickPhysicalDevice() );
	CHECK( findQueueFamilies( m_VulkanPhysicalDevice, m_WindowSurface ) );

	// SRS - when USE_MoltenVK defined, set MoltenVK runtime configuration parameters on macOS
#if defined(__APPLE__) && defined( USE_MoltenVK )
	vk::PhysicalDeviceFeatures2 deviceFeatures2;
	vk::PhysicalDevicePortabilitySubsetFeaturesKHR portabilityFeatures;
	deviceFeatures2.setPNext( &portabilityFeatures );
	m_VulkanPhysicalDevice.getFeatures2( &deviceFeatures2 );

	MVKConfiguration    pConfig;
	size_t              pConfigSize = sizeof( pConfig );

	vkGetMoltenVKConfigurationMVK( m_VulkanInstance, &pConfig, &pConfigSize );

	// SRS - Set MoltenVK's synchronous queue submit option for vkQueueSubmit() & vkQueuePresentKHR()
	pConfig.synchronousQueueSubmits = r_mvkSynchronousQueueSubmits.GetBool() ? VK_TRUE : VK_FALSE;
	vkSetMoltenVKConfigurationMVK( m_VulkanInstance, &pConfig, &pConfigSize );

	// SRS - If we don't have native image view swizzle, enable MoltenVK's image view swizzle feature
	if( portabilityFeatures.imageViewFormatSwizzle == VK_FALSE )
	{
		idLib::Printf( "Enabling MoltenVK's image view swizzle...\n" );
		pConfig.fullImageViewSwizzle = VK_TRUE;
		vkSetMoltenVKConfigurationMVK( m_VulkanInstance, &pConfig, &pConfigSize );
	}

	// SRS - Turn MoltenVK's Metal argument buffer feature on for descriptor indexing only
	if( pConfig.useMetalArgumentBuffers == MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS_NEVER )
	{
		idLib::Printf( "Enabling MoltenVK's Metal argument buffers for descriptor indexing...\n" );
		pConfig.useMetalArgumentBuffers = MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS_DESCRIPTOR_INDEXING;
		vkSetMoltenVKConfigurationMVK( m_VulkanInstance, &pConfig, &pConfigSize );
	}
#endif

	CHECK( createDevice() );

	auto vecInstanceExt = stringSetToVector( enabledExtensions.instance );
	auto vecLayers = stringSetToVector( enabledExtensions.layers );
	auto vecDeviceExt = stringSetToVector( enabledExtensions.device );

	nvrhi::vulkan::DeviceDesc deviceDesc;
	deviceDesc.errorCB = &DefaultMessageCallback::GetInstance();
	deviceDesc.instance = m_VulkanInstance;
	deviceDesc.physicalDevice = m_VulkanPhysicalDevice;
	deviceDesc.device = m_VulkanDevice;
	deviceDesc.graphicsQueue = m_GraphicsQueue;
	deviceDesc.graphicsQueueIndex = m_GraphicsQueueFamily;
	if( m_DeviceParams.enableComputeQueue )
	{
		deviceDesc.computeQueue = m_ComputeQueue;
		deviceDesc.computeQueueIndex = m_ComputeQueueFamily;
	}
	if( m_DeviceParams.enableCopyQueue )
	{
		deviceDesc.transferQueue = m_TransferQueue;
		deviceDesc.transferQueueIndex = m_TransferQueueFamily;
	}
	deviceDesc.instanceExtensions = vecInstanceExt.data();
	deviceDesc.numInstanceExtensions = vecInstanceExt.size();
	deviceDesc.deviceExtensions = vecDeviceExt.data();
	deviceDesc.numDeviceExtensions = vecDeviceExt.size();

	m_NvrhiDevice = nvrhi::vulkan::createDevice( deviceDesc );

	if( m_DeviceParams.enableNvrhiValidationLayer )
	{
		m_ValidationLayer = nvrhi::validation::createValidationLayer( m_NvrhiDevice );
	}

	CHECK( createSwapChain() );

	m_BarrierCommandList = m_NvrhiDevice->createCommandList();

	// SRS - Give each swapchain image its own semaphore in case of overlap (e.g. MoltenVK async queue submit)
	for( int i = 0; i < m_SwapChainImages.size(); i++ )
	{
		m_PresentSemaphoreQueue.push( m_VulkanDevice.createSemaphore( vk::SemaphoreCreateInfo() ) );
	}
	m_PresentSemaphore = m_PresentSemaphoreQueue.front();

	m_FrameWaitQuery = m_NvrhiDevice->createEventQuery();
	m_NvrhiDevice->setEventQuery( m_FrameWaitQuery, nvrhi::CommandQueue::Graphics );

#undef CHECK

	return true;
}

void DeviceManager_VK::DestroyDeviceAndSwapChain()
{
	m_FrameWaitQuery = nullptr;

	for( int i = 0; i < m_SwapChainImages.size(); i++ )
	{
		m_VulkanDevice.destroySemaphore( m_PresentSemaphoreQueue.front() );
		m_PresentSemaphoreQueue.pop();
	}
	m_PresentSemaphore = vk::Semaphore();

	m_BarrierCommandList = nullptr;

	destroySwapChain();

	m_NvrhiDevice = nullptr;
	m_ValidationLayer = nullptr;
	m_RendererString.clear();

	if( m_DebugReportCallback )
	{
		m_VulkanInstance.destroyDebugReportCallbackEXT( m_DebugReportCallback );
	}

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator )
	{
		// SRS - make sure image allocation garbage is emptied for all frames
		for( int i = 0; i < NUM_FRAME_DATA; i++ )
		{
			idImage::EmptyGarbage();
		}
		vmaDestroyAllocator( m_VmaAllocator );
		m_VmaAllocator = nullptr;
	}
#endif

	if( m_VulkanDevice )
	{
		m_VulkanDevice.destroy();
		m_VulkanDevice = nullptr;
	}

	if( m_WindowSurface )
	{
		assert( m_VulkanInstance );
		m_VulkanInstance.destroySurfaceKHR( m_WindowSurface );
		m_WindowSurface = nullptr;
	}

	if( m_VulkanInstance )
	{
		m_VulkanInstance.destroy();
		m_VulkanInstance = nullptr;
	}
}

void DeviceManager_VK::BeginFrame()
{
	const vk::Result res = m_VulkanDevice.acquireNextImageKHR( m_SwapChain,
						   std::numeric_limits<uint64_t>::max(), // timeout
						   m_PresentSemaphore,
						   vk::Fence(),
						   &m_SwapChainIndex );

	assert( res == vk::Result::eSuccess || res == vk::Result::eSuboptimalKHR );

	m_NvrhiDevice->queueWaitForSemaphore( nvrhi::CommandQueue::Graphics, m_PresentSemaphore, 0 );
}

void DeviceManager_VK::EndFrame()
{
	m_NvrhiDevice->queueSignalSemaphore( nvrhi::CommandQueue::Graphics, m_PresentSemaphore, 0 );

	m_BarrierCommandList->open(); // umm...
	m_BarrierCommandList->close();
	m_NvrhiDevice->executeCommandList( m_BarrierCommandList );
}

void DeviceManager_VK::Present()
{
	vk::PresentInfoKHR info = vk::PresentInfoKHR()
							  .setWaitSemaphoreCount( 1 )
							  .setPWaitSemaphores( &m_PresentSemaphore )
							  .setSwapchainCount( 1 )
							  .setPSwapchains( &m_SwapChain )
							  .setPImageIndices( &m_SwapChainIndex );

	const vk::Result res = m_PresentQueue.presentKHR( &info );
	assert( res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR );

	// SRS - Cycle the semaphore queue and setup m_PresentSemaphore for the next swapchain image
	m_PresentSemaphoreQueue.pop();
	m_PresentSemaphoreQueue.push( m_PresentSemaphore );
	m_PresentSemaphore = m_PresentSemaphoreQueue.front();

#if !defined(__APPLE__) || !defined( USE_MoltenVK )
	// SRS - validation layer is present only when the vulkan loader + layers are enabled (i.e. not MoltenVK standalone)
	if( m_DeviceParams.enableDebugRuntime )
	{
		// according to vulkan-tutorial.com, "the validation layer implementation expects
		// the application to explicitly synchronize with the GPU"
		m_PresentQueue.waitIdle();
	}
	else
#endif
	{
		if constexpr( NUM_FRAME_DATA > 2 )
		{
			// SRS - For triple buffering, sync on previous frame's command queue completion
			m_NvrhiDevice->waitEventQuery( m_FrameWaitQuery );
		}

		m_NvrhiDevice->resetEventQuery( m_FrameWaitQuery );
		m_NvrhiDevice->setEventQuery( m_FrameWaitQuery, nvrhi::CommandQueue::Graphics );

		if constexpr( NUM_FRAME_DATA < 3 )
		{
			// SRS - For double buffering, sync on current frame's command queue completion
			m_NvrhiDevice->waitEventQuery( m_FrameWaitQuery );
		}
	}
}

DeviceManager* DeviceManager::CreateVK()
{
	return new DeviceManager_VK();
}
