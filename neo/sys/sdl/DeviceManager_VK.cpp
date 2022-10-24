/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
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

#include <precompiled.h>
#pragma hdrstop

#include <string>
#include <queue>
#include <unordered_set>

#include <sys/DeviceManager.h>

#include <nvrhi/vulkan.h>
#include <nvrhi/validation.h>

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
		{
#if defined(__APPLE__) && !defined( USE_MoltenVK )
			//"VK_LAYER_KHRONOS_synchronization2"		// sync2 not supported natively on MoltenVK, use layer implementation instead
#endif
		},
		// device
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#if defined(__APPLE__)
			VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
			VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
			VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
#if !defined( USE_MoltenVK )
			//VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
#endif
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
		{ },
		// device
		{
			VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_NV_MESH_SHADER_EXTENSION_NAME,
			VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME
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
	vk::Semaphore m_PresentSemaphore;

	std::queue<nvrhi::EventQueryHandle> m_FramesInFlight;
	std::vector<nvrhi::EventQueryHandle> m_QueryPool;


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
			const auto& ignored = manager->deviceParms.ignoredVulkanValidationMessageLocations;
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
	for( const std::string& name : deviceParms.requiredVulkanInstanceExtensions )
	{
		enabledExtensions.instance.insert( name );
	}
	for( const std::string& name : deviceParms.optionalVulkanInstanceExtensions )
	{
		optionalExtensions.instance.insert( name );
	}

	// add layers requested by the user
	for( const std::string& name : deviceParms.requiredVulkanLayers )
	{
		enabledExtensions.layers.insert( name );
	}
	for( const std::string& name : deviceParms.optionalVulkanLayers )
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

	vk::Result res = m_VulkanInstance.createDebugReportCallbackEXT( &info, nullptr, &m_DebugReportCallback );
	assert( res == vk::Result::eSuccess );
}

bool DeviceManager_VK::pickPhysicalDevice()
{
	vk::Format requestedFormat = nvrhi::vulkan::convertFormat( deviceParms.swapChainFormat );
	vk::Extent2D requestedExtent( deviceParms.backBufferWidth, deviceParms.backBufferHeight );

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

		if( surfaceCaps.minImageCount > deviceParms.swapChainBufferCount ||
				( surfaceCaps.maxImageCount < deviceParms.swapChainBufferCount && surfaceCaps.maxImageCount > 0 ) )
		{
			errorStream << std::endl << "  - cannot support the requested swap chain image count:";
			errorStream << " requested " << deviceParms.swapChainBufferCount << ", available " << surfaceCaps.minImageCount << " - " << surfaceCaps.maxImageCount;
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
					vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, i, surface, &presentSupported ) == VK_SUCCESS )
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
			( m_ComputeQueueFamily == -1 && deviceParms.enableComputeQueue ) ||
			( m_TransferQueueFamily == -1 && deviceParms.enableCopyQueue ) )
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

		if( deviceParms.enableRayTracingExtensions && m_RayTracingExtensions.find( name ) != m_RayTracingExtensions.end() )
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
	}

	std::unordered_set<int> uniqueQueueFamilies =
	{
		m_GraphicsQueueFamily,
		m_PresentQueueFamily
	};

	if( deviceParms.enableComputeQueue )
	{
		uniqueQueueFamilies.insert( m_ComputeQueueFamily );
	}

	if( deviceParms.enableCopyQueue )
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

	void* pNext = nullptr;
#define APPEND_EXTENSION(condition, desc) if (condition) { (desc).pNext = pNext; pNext = &(desc); }  // NOLINT(cppcoreguidelines-macro-usage)
	APPEND_EXTENSION( accelStructSupported, accelStructFeatures )
	APPEND_EXTENSION( bufferAddressSupported, bufferAddressFeatures )
	APPEND_EXTENSION( rayPipelineSupported, rayPipelineFeatures )
	APPEND_EXTENSION( rayQuerySupported, rayQueryFeatures )
	APPEND_EXTENSION( meshletsSupported, meshletFeatures )
	APPEND_EXTENSION( vrsSupported, vrsFeatures )
#undef APPEND_EXTENSION

	auto deviceFeatures = vk::PhysicalDeviceFeatures()
						  .setShaderImageGatherExtended( true )
						  .setSamplerAnisotropy( true )
						  .setTessellationShader( true )
						  .setTextureCompressionBC( true )
#if !defined(__APPLE__)
						  .setGeometryShader( true )
#endif
						  .setImageCubeArray( true )
						  .setDualSrcBlend( true );

	auto vulkan12features = vk::PhysicalDeviceVulkan12Features()
							.setDescriptorIndexing( true )
							.setRuntimeDescriptorArray( true )
							.setDescriptorBindingPartiallyBound( true )
							.setDescriptorBindingVariableDescriptorCount( true )
							.setTimelineSemaphore( true )
							.setShaderSampledImageArrayNonUniformIndexing( true )
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
	if( deviceParms.enableComputeQueue )
	{
		m_VulkanDevice.getQueue( m_ComputeQueueFamily, 0, &m_ComputeQueue );
	}
	if( deviceParms.enableCopyQueue )
	{
		m_VulkanDevice.getQueue( m_TransferQueueFamily, 0, &m_TransferQueue );
	}
	m_VulkanDevice.getQueue( m_PresentQueueFamily, 0, &m_PresentQueue );

	VULKAN_HPP_DEFAULT_DISPATCHER.init( m_VulkanDevice );

	// stash the renderer string
	auto prop = m_VulkanPhysicalDevice.getProperties();
	m_RendererString = std::string( prop.deviceName.data() );

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
	VkResult err = VK_SUCCESS;

	// Create the platform-specific surface
#if defined( VULKAN_USE_PLATFORM_SDL )
	// SRS - Support generic SDL platform for linux and macOS
	if( !CreateSDLWindowSurface( m_VulkanInstance, ( VkSurfaceKHR* )&m_WindowSurface ) )
	{
		err = VK_ERROR_NATIVE_WINDOW_IN_USE_KHR;
	}
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = ( HINSTANCE )windowInstance;
	surfaceCreateInfo.hwnd = ( HWND )windowHandle;
	err = vkCreateWin32SurfaceKHR( m_VulkanInstance, &surfaceCreateInfo, nullptr, ( VkSurfaceKHR* )&m_WindowSurface );
#endif

	if( err != VK_SUCCESS )
	{
		common->FatalError( "Failed to create a Vulkan window surface, error code = %s", nvrhi::vulkan::resultToString( err ) );
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

	if( m_SwapChain )
	{
		m_VulkanDevice.destroySwapchainKHR( m_SwapChain );
		m_SwapChain = nullptr;
	}

	m_SwapChainImages.clear();
}

bool DeviceManager_VK::createSwapChain()
{
	destroySwapChain();

	m_SwapChainFormat =
	{
		vk::Format( nvrhi::vulkan::convertFormat( deviceParms.swapChainFormat ) ),
		vk::ColorSpaceKHR::eSrgbNonlinear
	};

	vk::Extent2D extent = vk::Extent2D( deviceParms.backBufferWidth, deviceParms.backBufferHeight );

	std::unordered_set<uint32_t> uniqueQueues =
	{
		uint32_t( m_GraphicsQueueFamily ),
		uint32_t( m_PresentQueueFamily )
	};

	std::vector<uint32_t> queues = setToVector( uniqueQueues );

	const bool enableSwapChainSharing = queues.size() > 1;

	auto desc = vk::SwapchainCreateInfoKHR()
				.setSurface( m_WindowSurface )
				.setMinImageCount( deviceParms.swapChainBufferCount )
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
				.setPresentMode( deviceParms.vsyncEnabled ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate )
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
		textureDesc.width = deviceParms.backBufferWidth;
		textureDesc.height = deviceParms.backBufferHeight;
		textureDesc.format = deviceParms.swapChainFormat;
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
	deviceParms.enableNvrhiValidationLayer = r_useValidationLayers.GetInteger() > 0;
	deviceParms.enableDebugRuntime = r_useValidationLayers.GetInteger() > 1;

	if( deviceParms.enableDebugRuntime )
	{
		enabledExtensions.instance.insert( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
#if defined(__APPLE__) && defined( USE_MoltenVK )
		enabledExtensions.layers.insert( "MoltenVK" );
#else
		enabledExtensions.layers.insert( "VK_LAYER_KHRONOS_validation" );
#endif
	}

	const vk::DynamicLoader dl;
	const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =   // NOLINT(misc-misplaced-const)
		dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
	VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

#define CHECK(a) if (!(a)) { return false; }

	CHECK( createInstance() );

	if( deviceParms.enableDebugRuntime )
	{
		installDebugCallback();
	}

	if( deviceParms.swapChainFormat == nvrhi::Format::SRGBA8_UNORM )
	{
		deviceParms.swapChainFormat = nvrhi::Format::SBGRA8_UNORM;
	}
	else if( deviceParms.swapChainFormat == nvrhi::Format::RGBA8_UNORM )
	{
		deviceParms.swapChainFormat = nvrhi::Format::BGRA8_UNORM;
	}

	// add device extensions requested by the user
	for( const std::string& name : deviceParms.requiredVulkanDeviceExtensions )
	{
		enabledExtensions.device.insert( name );
	}
	for( const std::string& name : deviceParms.optionalVulkanDeviceExtensions )
	{
		optionalExtensions.device.insert( name );
	}

	CHECK( createWindowSurface() );
	CHECK( pickPhysicalDevice() );
	CHECK( findQueueFamilies( m_VulkanPhysicalDevice, m_WindowSurface ) );
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
	if( deviceParms.enableComputeQueue )
	{
		deviceDesc.computeQueue = m_ComputeQueue;
		deviceDesc.computeQueueIndex = m_ComputeQueueFamily;
	}
	if( deviceParms.enableCopyQueue )
	{
		deviceDesc.transferQueue = m_TransferQueue;
		deviceDesc.transferQueueIndex = m_TransferQueueFamily;
	}
	deviceDesc.instanceExtensions = vecInstanceExt.data();
	deviceDesc.numInstanceExtensions = vecInstanceExt.size();
	deviceDesc.deviceExtensions = vecDeviceExt.data();
	deviceDesc.numDeviceExtensions = vecDeviceExt.size();

	m_NvrhiDevice = nvrhi::vulkan::createDevice( deviceDesc );

	if( deviceParms.enableNvrhiValidationLayer )
	{
		m_ValidationLayer = nvrhi::validation::createValidationLayer( m_NvrhiDevice );
	}

	CHECK( createSwapChain() );

	m_BarrierCommandList = m_NvrhiDevice->createCommandList();

	m_PresentSemaphore = m_VulkanDevice.createSemaphore( vk::SemaphoreCreateInfo() );

#undef CHECK

	return true;
}

void DeviceManager_VK::DestroyDeviceAndSwapChain()
{
	destroySwapChain();

	m_VulkanDevice.destroySemaphore( m_PresentSemaphore );
	m_PresentSemaphore = vk::Semaphore();

	m_BarrierCommandList = nullptr;

	m_NvrhiDevice = nullptr;
	m_ValidationLayer = nullptr;
	m_RendererString.clear();

	if( m_DebugReportCallback )
	{
		m_VulkanInstance.destroyDebugReportCallbackEXT( m_DebugReportCallback );
	}

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

	assert( res == vk::Result::eSuccess );

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
	assert( res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR );

	if( deviceParms.enableDebugRuntime )
	{
		// according to vulkan-tutorial.com, "the validation layer implementation expects
		// the application to explicitly synchronize with the GPU"
		m_PresentQueue.waitIdle();
	}
	else
	{
#ifndef _WIN32
		if( deviceParms.vsyncEnabled )
		{
			m_PresentQueue.waitIdle();
		}
#endif

		while( m_FramesInFlight.size() > deviceParms.maxFramesInFlight )
		{
			auto query = m_FramesInFlight.front();
			m_FramesInFlight.pop();

			m_NvrhiDevice->waitEventQuery( query );

			m_QueryPool.push_back( query );
		}

		nvrhi::EventQueryHandle query;
		if( !m_QueryPool.empty() )
		{
			query = m_QueryPool.back();
			m_QueryPool.pop_back();
		}
		else
		{
			query = m_NvrhiDevice->createEventQuery();
		}

		m_NvrhiDevice->resetEventQuery( query );
		m_NvrhiDevice->setEventQuery( query, nvrhi::CommandQueue::Graphics );
		m_FramesInFlight.push( query );
	}
}

DeviceManager* DeviceManager::CreateVK()
{
	return new DeviceManager_VK();
}
