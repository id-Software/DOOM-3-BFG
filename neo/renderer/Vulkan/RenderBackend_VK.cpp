/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2018 Robert Beckebans
Copyright (C) 2016-2017 Dustin Land

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

#pragma hdrstop
#include "precompiled.h"

#include "../RenderCommon.h"
#include "../RenderBackend.h"
#include "Staging_VK.h"
//#include "../../framework/Common_local.h"

idCVar r_drawFlickerBox( "r_drawFlickerBox", "0", CVAR_RENDERER | CVAR_BOOL, "visual test for dropping frames" );
idCVar stereoRender_warp( "stereoRender_warp", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use the optical warping renderprog instead of stereoDeGhost" );

idCVar r_showSwapBuffers( "r_showSwapBuffers", "0", CVAR_BOOL, "Show timings from GL_BlockingSwapBuffers" );
idCVar r_syncEveryFrame( "r_syncEveryFrame", "1", CVAR_BOOL, "Don't let the GPU buffer execution past swapbuffers" );


// NEW VULKAN STUFF

idCVar r_vkEnableValidationLayers( "r_vkEnableValidationLayers", "1", CVAR_BOOL, "" );

vulkanContext_t vkcontext;

static const int g_numInstanceExtensions = 2;
static const char* g_instanceExtensions[ g_numInstanceExtensions ] =
{
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME
};

static const int g_numDebugInstanceExtensions = 1;
static const char* g_debugInstanceExtensions[ g_numDebugInstanceExtensions ] =
{
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};

static const int g_numDeviceExtensions = 1;
static const char* g_deviceExtensions[ g_numDeviceExtensions ] =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const int g_numValidationLayers = 1;
static const char* g_validationLayers[ g_numValidationLayers ] =
{
	"VK_LAYER_LUNARG_standard_validation"
};

#define ID_VK_ERROR_STRING( x ) case static_cast< int >( x ): return #x

/*
=============
VK_ErrorToString
=============
*/
const char* VK_ErrorToString( VkResult result )
{
	switch( result )
	{
			ID_VK_ERROR_STRING( VK_SUCCESS );
			ID_VK_ERROR_STRING( VK_NOT_READY );
			ID_VK_ERROR_STRING( VK_TIMEOUT );
			ID_VK_ERROR_STRING( VK_EVENT_SET );
			ID_VK_ERROR_STRING( VK_EVENT_RESET );
			ID_VK_ERROR_STRING( VK_INCOMPLETE );
			ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_HOST_MEMORY );
			ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_DEVICE_MEMORY );
			ID_VK_ERROR_STRING( VK_ERROR_INITIALIZATION_FAILED );
			ID_VK_ERROR_STRING( VK_ERROR_DEVICE_LOST );
			ID_VK_ERROR_STRING( VK_ERROR_MEMORY_MAP_FAILED );
			ID_VK_ERROR_STRING( VK_ERROR_LAYER_NOT_PRESENT );
			ID_VK_ERROR_STRING( VK_ERROR_EXTENSION_NOT_PRESENT );
			ID_VK_ERROR_STRING( VK_ERROR_FEATURE_NOT_PRESENT );
			ID_VK_ERROR_STRING( VK_ERROR_INCOMPATIBLE_DRIVER );
			ID_VK_ERROR_STRING( VK_ERROR_TOO_MANY_OBJECTS );
			ID_VK_ERROR_STRING( VK_ERROR_FORMAT_NOT_SUPPORTED );
			ID_VK_ERROR_STRING( VK_ERROR_SURFACE_LOST_KHR );
			ID_VK_ERROR_STRING( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR );
			ID_VK_ERROR_STRING( VK_SUBOPTIMAL_KHR );
			ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_DATE_KHR );
			ID_VK_ERROR_STRING( VK_ERROR_INCOMPATIBLE_DISPLAY_KHR );
			ID_VK_ERROR_STRING( VK_ERROR_VALIDATION_FAILED_EXT );
			ID_VK_ERROR_STRING( VK_ERROR_INVALID_SHADER_NV );
			ID_VK_ERROR_STRING( VK_RESULT_BEGIN_RANGE );
			ID_VK_ERROR_STRING( VK_RESULT_RANGE_SIZE );
		default:
			return "UNKNOWN";
	};
}




/*
=========================================================================================================

DEBUGGING AND VALIDATION

=========================================================================================================
*/

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
		size_t location, int32_t msgCode, const char* layerPrefix, const char* msg,
		void* userData )
{
	if( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
	{
		idLib::Printf( "[Vulkan] ERROR: [ %s ] Code %d : '%s'\n", layerPrefix, msgCode, msg );
	}
	else if( msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT )
	{
		idLib::Printf( "[Vulkan] WARNING: [ %s ] Code %d : '%s'\n", layerPrefix, msgCode, msg );
	}
	else if( msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT )
	{
		idLib::Printf( "[Vulkan] PERFORMANCE WARNING: [ %s ] Code %d : '%s'\n", layerPrefix, msgCode, msg );
	}
	else if( msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT )
	{
		idLib::Printf( "[Vulkan] INFO: [ %s ] Code %d : '%s'\n", layerPrefix, msgCode, msg );
	}
	else if( msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT )
	{
		idLib::Printf( "[Vulkan] DEBUG: [ %s ] Code %d : '%s'\n", layerPrefix, msgCode, msg );
	}
	
	/*
	 * false indicates that layer should not bail-out of an
	 * API call that had validation failures. This may mean that the
	 * app dies inside the driver due to invalid parameter(s).
	 * That's what would happen without validation layers, so we'll
	 * keep that behavior here.
	 */
	return VK_FALSE;
}

/*
=============
CreateDebugReportCallback
=============
*/
static void CreateDebugReportCallback()
{
	VkDebugReportCallbackCreateInfoEXT callbackInfo = {};
	callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	callbackInfo.flags = VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT; // VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
	callbackInfo.pfnCallback = ( PFN_vkDebugReportCallbackEXT ) DebugCallback;
	
	PFN_vkCreateDebugReportCallbackEXT func = ( PFN_vkCreateDebugReportCallbackEXT ) vkGetInstanceProcAddr( vkcontext.instance, "vkCreateDebugReportCallbackEXT" );
	ID_VK_VALIDATE( func != NULL, "Could not find vkCreateDebugReportCallbackEXT" );
	ID_VK_CHECK( func( vkcontext.instance, &callbackInfo, NULL, &vkcontext.callback ) );
}

/*
=============
DestroyDebugReportCallback
=============
*/
static void DestroyDebugReportCallback()
{
	PFN_vkDestroyDebugReportCallbackEXT func = ( PFN_vkDestroyDebugReportCallbackEXT ) vkGetInstanceProcAddr( vkcontext.instance, "vkDestroyDebugReportCallbackEXT" );
	ID_VK_VALIDATE( func != NULL, "Could not find vkDestroyDebugReportCallbackEXT" );
	func( vkcontext.instance, vkcontext.callback, NULL );
}

/*
=============
ValidateValidationLayers
=============
*/
static void ValidateValidationLayers()
{
	uint32 instanceLayerCount = 0;
	vkEnumerateInstanceLayerProperties( &instanceLayerCount, NULL );
	
	idList< VkLayerProperties > instanceLayers;
	instanceLayers.SetNum( instanceLayerCount );
	vkEnumerateInstanceLayerProperties( &instanceLayerCount, instanceLayers.Ptr() );
	
	bool found = false;
	for( uint32 i = 0; i < g_numValidationLayers; ++i )
	{
		for( uint32 j = 0; j < instanceLayerCount; ++j )
		{
			if( i == 0 )
			{
				idLib::Printf( "Found Vulkan Validation Layer '%s'\n", instanceLayers[j].layerName );
			}
			
			if( idStr::Icmp( g_validationLayers[i], instanceLayers[j].layerName ) == 0 )
			{
				found = true;
				break;
			}
		}
		
		if( !found )
		{
			idLib::FatalError( "Cannot find validation layer: %s.\n", g_validationLayers[ i ] );
		}
	}
}
/*
=============
CreateVulkanInstance
=============
*/
static void CreateVulkanInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = GAME_NAME;
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "idTech 4.5x";
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_MAKE_VERSION( 1, 0, VK_HEADER_VERSION );
	
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	
	const bool enableLayers = r_vkEnableValidationLayers.GetBool();
	
	vkcontext.instanceExtensions.Clear();
	vkcontext.deviceExtensions.Clear();
	vkcontext.validationLayers.Clear();
	
	for( int i = 0; i < g_numInstanceExtensions; ++i )
	{
		vkcontext.instanceExtensions.Append( g_instanceExtensions[ i ] );
	}
	
	for( int i = 0; i < g_numDeviceExtensions; ++i )
	{
		vkcontext.deviceExtensions.Append( g_deviceExtensions[ i ] );
	}
	
	if( enableLayers )
	{
		for( int i = 0; i < g_numDebugInstanceExtensions; ++i )
		{
			vkcontext.instanceExtensions.Append( g_debugInstanceExtensions[ i ] );
		}
		
		for( int i = 0; i < g_numValidationLayers; ++i )
		{
			vkcontext.validationLayers.Append( g_validationLayers[ i ] );
		}
		
		ValidateValidationLayers();
	}
	
	createInfo.enabledExtensionCount = vkcontext.instanceExtensions.Num();
	createInfo.ppEnabledExtensionNames = vkcontext.instanceExtensions.Ptr();
	createInfo.enabledLayerCount = vkcontext.validationLayers.Num();
	createInfo.ppEnabledLayerNames = vkcontext.validationLayers.Ptr();
	
	ID_VK_CHECK( vkCreateInstance( &createInfo, NULL, &vkcontext.instance ) );
	
	if( enableLayers )
	{
		CreateDebugReportCallback();
	}
}

/*
=============
EnumeratePhysicalDevices
=============
*/
static void EnumeratePhysicalDevices()
{
	uint32 numDevices = 0;
	ID_VK_CHECK( vkEnumeratePhysicalDevices( vkcontext.instance, &numDevices, NULL ) );
	ID_VK_VALIDATE( numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices." );
	
	idList< VkPhysicalDevice > devices;
	devices.SetNum( numDevices );
	
	ID_VK_CHECK( vkEnumeratePhysicalDevices( vkcontext.instance, &numDevices, devices.Ptr() ) );
	ID_VK_VALIDATE( numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices." );
	
	vkcontext.gpus.SetNum( numDevices );
	
	for( uint32 i = 0; i < numDevices; ++i )
	{
		gpuInfo_t& gpu = vkcontext.gpus[ i ];
		gpu.device = devices[ i ];
		
		// get Queue family properties
		{
			uint32 numQueues = 0;
			vkGetPhysicalDeviceQueueFamilyProperties( gpu.device, &numQueues, NULL );
			ID_VK_VALIDATE( numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues." );
			
			gpu.queueFamilyProps.SetNum( numQueues );
			vkGetPhysicalDeviceQueueFamilyProperties( gpu.device, &numQueues, gpu.queueFamilyProps.Ptr() );
			ID_VK_VALIDATE( numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues." );
		}
		
		// grab available Vulkan extensions
		{
			uint32 numExtension;
			ID_VK_CHECK( vkEnumerateDeviceExtensionProperties( gpu.device, NULL, &numExtension, NULL ) );
			ID_VK_VALIDATE( numExtension > 0, "vkEnumerateDeviceExtensionProperties returned zero extensions." );
			
			gpu.extensionProps.SetNum( numExtension );
			ID_VK_CHECK( vkEnumerateDeviceExtensionProperties( gpu.device, NULL, &numExtension, gpu.extensionProps.Ptr() ) );
			ID_VK_VALIDATE( numExtension > 0, "vkEnumerateDeviceExtensionProperties returned zero extensions." );
			
#if 0
			for( uint32 j = 0; j < numExtension; j++ )
			{
				idLib::Printf( "Found Vulkan Extension '%s' on device %d\n", gpu.extensionProps[j].extensionName, i );
			}
#endif
		}
		
		// grab surface specific information
		ID_VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gpu.device, vkcontext.surface, &gpu.surfaceCaps ) );
		
		{
			uint32 numFormats;
			ID_VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( gpu.device, vkcontext.surface, &numFormats, NULL ) );
			ID_VK_VALIDATE( numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats." );
			
			gpu.surfaceFormats.SetNum( numFormats );
			ID_VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( gpu.device, vkcontext.surface, &numFormats, gpu.surfaceFormats.Ptr() ) );
			ID_VK_VALIDATE( numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats." );
		}
		
		{
			uint32 numPresentModes;
			ID_VK_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR( gpu.device, vkcontext.surface, &numPresentModes, NULL ) );
			ID_VK_VALIDATE( numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes." );
			
			gpu.presentModes.SetNum( numPresentModes );
			ID_VK_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR( gpu.device, vkcontext.surface, &numPresentModes, gpu.presentModes.Ptr() ) );
			ID_VK_VALIDATE( numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes." );
		}
		
		vkGetPhysicalDeviceMemoryProperties( gpu.device, &gpu.memProps );
		vkGetPhysicalDeviceProperties( gpu.device, &gpu.props );
		
		switch( gpu.props.vendorID )
		{
			case 0x8086:
				idLib::Printf( "Found device[%i] Vendor: Intel\n", i );
				break;
				
			case 0x10DE:
				idLib::Printf( "Found device[%i] Vendor: NVIDIA\n", i );
				break;
				
			case 0x1002:
				idLib::Printf( "Found device[%i] Vendor: AMD\n", i );
				break;
				
			default:
				idLib::Printf( "Found device[%i] Vendor: Unknown (0x%x)\n", i, gpu.props.vendorID );
		}
	}
}

/*
=============
CreateSurface
=============
*/
#ifdef _WIN32
#include "../../sys/win32/win_local.h"
#endif

static void CreateSurface()
{
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hinstance = win32.hInstance;
	createInfo.hwnd = win32.hWnd;
	
	ID_VK_CHECK( vkCreateWin32SurfaceKHR( vkcontext.instance, &createInfo, NULL, &vkcontext.surface ) );
	
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	VkWaylandSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.display = info.display;
	createInfo.surface = info.window;
	
	ID_VK_CHECK( vkCreateWaylandSurfaceKHR( info.inst, &createInfo, NULL, &info.surface ) );
	
#else
	VkXcbSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.connection = info.connection;
	createInfo.window = info.window;
	
	ID_VK_CHECK( vkCreateXcbSurfaceKHR( info.inst, &createInfo, NULL, &info.surface ) );
#endif  // _WIN32
	
	
}


/*
=============
CheckPhysicalDeviceExtensionSupport
=============
*/
static bool CheckPhysicalDeviceExtensionSupport( gpuInfo_t& gpu, idList< const char* >& requiredExt )
{
	int required = requiredExt.Num();
	int available = 0;
	
	for( int i = 0; i < requiredExt.Num(); ++i )
	{
		for( int j = 0; j < gpu.extensionProps.Num(); ++j )
		{
			if( idStr::Icmp( requiredExt[ i ], gpu.extensionProps[ j ].extensionName ) == 0 )
			{
				available++;
				break;
			}
		}
	}
	
	return available == required;
}

/*
=============
SelectPhysicalDevice
=============
*/
static void SelectPhysicalDevice()
{
	//idLib::Printf( "Selecting physical device:\n" );
	
	for( int i = 0; i < vkcontext.gpus.Num(); ++i )
	{
		gpuInfo_t& gpu = vkcontext.gpus[ i ];
		
		int graphicsIdx = -1;
		int presentIdx = -1;
		
		if( !CheckPhysicalDeviceExtensionSupport( gpu, vkcontext.deviceExtensions ) )
		{
			continue;
		}
		
		if( gpu.surfaceFormats.Num() == 0 )
		{
			continue;
		}
		
		if( gpu.presentModes.Num() == 0 )
		{
			continue;
		}
		
		// Find graphics queue family
		for( int j = 0; j < gpu.queueFamilyProps.Num(); ++j )
		{
			VkQueueFamilyProperties& props = gpu.queueFamilyProps[ j ];
			
			if( props.queueCount == 0 )
			{
				continue;
			}
			
			if( props.queueFlags & VK_QUEUE_GRAPHICS_BIT )
			{
				graphicsIdx = j;
				break;
			}
		}
		
		// Find present queue family
		for( int j = 0; j < gpu.queueFamilyProps.Num(); ++j )
		{
			VkQueueFamilyProperties& props = gpu.queueFamilyProps[ j ];
			
			if( props.queueCount == 0 )
			{
				continue;
			}
			
			VkBool32 supportsPresent = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR( gpu.device, j, vkcontext.surface, &supportsPresent );
			if( supportsPresent )
			{
				presentIdx = j;
				break;
			}
		}
		
		// Did we find a device supporting both graphics and present.
		if( graphicsIdx >= 0 && presentIdx >= 0 )
		{
			vkcontext.graphicsFamilyIdx = graphicsIdx;
			vkcontext.presentFamilyIdx = presentIdx;
			vkcontext.physicalDevice = gpu.device;
			vkcontext.gpu = &gpu;
			
			vkGetPhysicalDeviceFeatures( vkcontext.physicalDevice, &vkcontext.physicalDeviceFeatures );
			
			idLib::Printf( "Selected device '%s'\n", gpu.props.deviceName );
			
			// RB: found vendor IDs in nvQuake
			switch( gpu.props.vendorID )
			{
				case 0x8086:
					idLib::Printf( "Vendor: Intel\n", i );
					glConfig.vendor = VENDOR_INTEL;
					break;
					
				case 0x10DE:
					idLib::Printf( "Vendor: NVIDIA\n", i );
					glConfig.vendor = VENDOR_NVIDIA;
					break;
					
				case 0x1002:
					idLib::Printf( "Vendor: AMD\n", i );
					glConfig.vendor = VENDOR_AMD;
					break;
					
				default:
					idLib::Printf( "Vendor: Unknown (0x%x)\n", i, gpu.props.vendorID );
			}
			
			return;
		}
	}
	
	// If we can't render or present, just bail.
	idLib::FatalError( "Could not find a physical device which fits our desired profile" );
}

/*
=============
CreateLogicalDeviceAndQueues
=============
*/
static void CreateLogicalDeviceAndQueues()
{
	idList< int > uniqueIdx;
	uniqueIdx.AddUnique( vkcontext.graphicsFamilyIdx );
	uniqueIdx.AddUnique( vkcontext.presentFamilyIdx );
	
	idList< VkDeviceQueueCreateInfo > devqInfo;
	
	const float priority = 1.0f;
	for( int i = 0; i < uniqueIdx.Num(); ++i )
	{
		VkDeviceQueueCreateInfo qinfo = {};
		qinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qinfo.queueFamilyIndex = uniqueIdx[ i ];
		qinfo.queueCount = 1;
		qinfo.pQueuePriorities = &priority;
		
		devqInfo.Append( qinfo );
	}
	
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.textureCompressionBC = VK_TRUE;
	deviceFeatures.imageCubeArray = VK_TRUE;
	deviceFeatures.depthClamp = VK_TRUE;
	deviceFeatures.depthBiasClamp = VK_TRUE;
	deviceFeatures.depthBounds = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	//deviceFeatures.samplerAnisotropy = vkcontext.physicalDeviceFeatures.samplerAnisotropy; // RB
	
	VkDeviceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info.queueCreateInfoCount = devqInfo.Num();
	info.pQueueCreateInfos = devqInfo.Ptr();
	info.pEnabledFeatures = &deviceFeatures;
	info.enabledExtensionCount = vkcontext.deviceExtensions.Num();
	info.ppEnabledExtensionNames = vkcontext.deviceExtensions.Ptr();
	
	if( r_vkEnableValidationLayers.GetBool() )
	{
		info.enabledLayerCount = vkcontext.validationLayers.Num();
		info.ppEnabledLayerNames = vkcontext.validationLayers.Ptr();
	}
	else
	{
		info.enabledLayerCount = 0;
	}
	
	ID_VK_CHECK( vkCreateDevice( vkcontext.physicalDevice, &info, NULL, &vkcontext.device ) );
	
	vkGetDeviceQueue( vkcontext.device, vkcontext.graphicsFamilyIdx, 0, &vkcontext.graphicsQueue );
	vkGetDeviceQueue( vkcontext.device, vkcontext.presentFamilyIdx, 0, &vkcontext.presentQueue );
}

/*
=============
ChooseSurfaceFormat
=============
*/
static VkSurfaceFormatKHR ChooseSurfaceFormat( idList< VkSurfaceFormatKHR >& formats )
{
	VkSurfaceFormatKHR result;
	
	if( formats.Num() == 1 && formats[ 0 ].format == VK_FORMAT_UNDEFINED )
	{
		result.format = VK_FORMAT_B8G8R8A8_UNORM;
		result.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		return result;
	}
	
	for( int i = 0; i < formats.Num(); ++i )
	{
		VkSurfaceFormatKHR& fmt = formats[ i ];
		if( fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
		{
			return fmt;
		}
	}
	
	return formats[ 0 ];
}

/*
=============
ChoosePresentMode
=============
*/
static VkPresentModeKHR ChoosePresentMode( idList< VkPresentModeKHR >& modes )
{
	VkPresentModeKHR desiredMode = VK_PRESENT_MODE_FIFO_KHR;
	
	if( r_swapInterval.GetInteger() < 1 )
	{
		for( int i = 0; i < modes.Num(); i++ )
		{
			if( modes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
			{
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
			if( ( modes[i] != VK_PRESENT_MODE_MAILBOX_KHR ) && ( modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR ) )
			{
				return VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	}
	
	for( int i = 0; i < modes.Num(); ++i )
	{
		if( modes[i] == desiredMode )
		{
			return desiredMode;
		}
	}
	
	return VK_PRESENT_MODE_FIFO_KHR;
}

/*
=============
ChooseSurfaceExtent
=============
*/
static VkExtent2D ChooseSurfaceExtent( VkSurfaceCapabilitiesKHR& caps )
{
	VkExtent2D extent;
	
	if( caps.currentExtent.width == -1 )
	{
		extent.width = glConfig.nativeScreenWidth;
		extent.height = glConfig.nativeScreenHeight;
	}
	else
	{
		extent = caps.currentExtent;
	}
	
	return extent;
}

/*
=============
CreateSwapChain
=============
*/
static void CreateSwapChain()
{
	gpuInfo_t& gpu = *vkcontext.gpu;
	
	VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat( gpu.surfaceFormats );
	VkPresentModeKHR presentMode = ChoosePresentMode( gpu.presentModes );
	VkExtent2D extent = ChooseSurfaceExtent( gpu.surfaceCaps );
	
	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = vkcontext.surface;
	info.minImageCount = NUM_FRAME_DATA;
	info.imageFormat = surfaceFormat.format;
	info.imageColorSpace = surfaceFormat.colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	
	if( vkcontext.graphicsFamilyIdx != vkcontext.presentFamilyIdx )
	{
		uint32 indices[] = { ( uint32 )vkcontext.graphicsFamilyIdx, ( uint32 )vkcontext.presentFamilyIdx };
		
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = indices;
	}
	else
	{
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	
	info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = presentMode;
	info.clipped = VK_TRUE;
	
	ID_VK_CHECK( vkCreateSwapchainKHR( vkcontext.device, &info, NULL, &vkcontext.swapchain ) );
	
	vkcontext.swapchainFormat = surfaceFormat.format;
	vkcontext.presentMode = presentMode;
	vkcontext.swapchainExtent = extent;
	vkcontext.fullscreen = glConfig.isFullscreen;
	
	uint32 numImages = 0;
	ID_VK_CHECK( vkGetSwapchainImagesKHR( vkcontext.device, vkcontext.swapchain, &numImages, NULL ) );
	ID_VK_VALIDATE( numImages > 0, "vkGetSwapchainImagesKHR returned a zero image count." );
	
	ID_VK_CHECK( vkGetSwapchainImagesKHR( vkcontext.device, vkcontext.swapchain, &numImages, vkcontext.swapchainImages.Ptr() ) );
	ID_VK_VALIDATE( numImages > 0, "vkGetSwapchainImagesKHR returned a zero image count." );
	
	for( uint32 i = 0; i < NUM_FRAME_DATA; ++i )
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = vkcontext.swapchainImages[ i ];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = vkcontext.swapchainFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.flags = 0;
		
		ID_VK_CHECK( vkCreateImageView( vkcontext.device, &imageViewCreateInfo, NULL, &vkcontext.swapchainViews[ i ] ) );
	}
}

/*
=============
DestroySwapChain
=============
*/
static void DestroySwapChain()
{
	for( uint32 i = 0; i < NUM_FRAME_DATA; ++i )
	{
		vkDestroyImageView( vkcontext.device, vkcontext.swapchainViews[ i ], NULL );
	}
	vkcontext.swapchainImages.Zero();
	vkcontext.swapchainViews.Zero();
	
	vkDestroySwapchainKHR( vkcontext.device, vkcontext.swapchain, NULL );
}

/*
=============
CreateCommandPool
=============
*/
static void CreateCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = vkcontext.graphicsFamilyIdx;
	
	ID_VK_CHECK( vkCreateCommandPool( vkcontext.device, &commandPoolCreateInfo, NULL, &vkcontext.commandPool ) );
}

/*
=============
CreateCommandBuffer
=============
*/
static void CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = vkcontext.commandPool;
	commandBufferAllocateInfo.commandBufferCount = NUM_FRAME_DATA;
	
	ID_VK_CHECK( vkAllocateCommandBuffers( vkcontext.device, &commandBufferAllocateInfo, vkcontext.commandBuffer.Ptr() ) );
	
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	
	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		ID_VK_CHECK( vkCreateFence( vkcontext.device, &fenceCreateInfo, NULL, &vkcontext.commandBufferFences[ i ] ) );
	}
}

/*
=============
CreateSemaphores
=============
*/
static void CreateSemaphores()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		ID_VK_CHECK( vkCreateSemaphore( vkcontext.device, &semaphoreCreateInfo, NULL, &vkcontext.acquireSemaphores[ i ] ) );
		ID_VK_CHECK( vkCreateSemaphore( vkcontext.device, &semaphoreCreateInfo, NULL, &vkcontext.renderCompleteSemaphores[ i ] ) );
	}
}



/*
=============
ChooseSupportedFormat
=============
*/
static VkFormat ChooseSupportedFormat( VkFormat* formats, int numFormats, VkImageTiling tiling, VkFormatFeatureFlags features )
{
	for( int i = 0; i < numFormats; ++i )
	{
		VkFormat format = formats[ i ];
		
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties( vkcontext.physicalDevice, format, &props );
		
		if( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features )
		{
			return format;
		}
		else if( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features )
		{
			return format;
		}
	}
	
	idLib::FatalError( "Failed to find a supported format." );
	
	return VK_FORMAT_UNDEFINED;
}

/*
=============
CreateRenderTargets
=============
*/
static void CreateRenderTargets()
{
	// Determine samples before creating depth
	VkImageFormatProperties fmtProps = {};
	vkGetPhysicalDeviceImageFormatProperties( vkcontext.physicalDevice, vkcontext.swapchainFormat,
			VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0, &fmtProps );
			
	int samples;
	
	switch( r_antiAliasing.GetInteger() )
	{
		case ANTI_ALIASING_MSAA_2X:
			samples = 2;
			break;
			
		case ANTI_ALIASING_MSAA_4X:
			samples = 4;
			break;
			
		case ANTI_ALIASING_MSAA_8X:
			samples = 8;
			break;
			
		default:
			samples = 0;
			break;
	}
	
	if( samples >= 16 && ( fmtProps.sampleCounts & VK_SAMPLE_COUNT_16_BIT ) )
	{
		vkcontext.sampleCount = VK_SAMPLE_COUNT_16_BIT;
	}
	else if( samples >= 8 && ( fmtProps.sampleCounts & VK_SAMPLE_COUNT_8_BIT ) )
	{
		vkcontext.sampleCount = VK_SAMPLE_COUNT_8_BIT;
	}
	else if( samples >= 4 && ( fmtProps.sampleCounts & VK_SAMPLE_COUNT_4_BIT ) )
	{
		vkcontext.sampleCount = VK_SAMPLE_COUNT_4_BIT;
	}
	else if( samples >= 2 && ( fmtProps.sampleCounts & VK_SAMPLE_COUNT_2_BIT ) )
	{
		vkcontext.sampleCount = VK_SAMPLE_COUNT_2_BIT;
	}
	
	// Select Depth Format
	{
		VkFormat formats[] =
		{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		};
		vkcontext.depthFormat = ChooseSupportedFormat(
									formats, 3,
									VK_IMAGE_TILING_OPTIMAL,
									VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
	}
	
	idImageOpts depthOptions;
	depthOptions.format = FMT_DEPTH;
	depthOptions.width = renderSystem->GetWidth();
	depthOptions.height = renderSystem->GetHeight();
	depthOptions.numLevels = 1;
	depthOptions.samples = static_cast< textureSamples_t >( vkcontext.sampleCount );
	
	globalImages->ScratchImage( "_viewDepth", depthOptions );
	
	if( vkcontext.sampleCount > VK_SAMPLE_COUNT_1_BIT )
	{
		vkcontext.supersampling = vkcontext.physicalDeviceFeatures.sampleRateShading == VK_TRUE;
		
		VkImageCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.format = vkcontext.swapchainFormat;
		createInfo.extent.width = vkcontext.swapchainExtent.width;
		createInfo.extent.height = vkcontext.swapchainExtent.height;
		createInfo.extent.depth = 1;
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.samples = vkcontext.sampleCount;
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		
		ID_VK_CHECK( vkCreateImage( vkcontext.device, &createInfo, NULL, &vkcontext.msaaImage ) );
		
#if defined( USE_AMD_ALLOCATOR )
		VmaMemoryRequirements vmaReq = {};
		vmaReq.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		
		ID_VK_CHECK( vmaCreateImage( vmaAllocator, &createInfo, &vmaReq, &vkcontext.msaaImage, &vkcontext.msaaVmaAllocation, &vkcontext.msaaAllocation ) );
#else
		VkMemoryRequirements memoryRequirements = {};
		vkGetImageMemoryRequirements( vkcontext.device, vkcontext.msaaImage, &memoryRequirements );
		
		vkcontext.msaaAllocation = vulkanAllocator.Allocate(
									   memoryRequirements.size,
									   memoryRequirements.alignment,
									   memoryRequirements.memoryTypeBits,
									   VULKAN_MEMORY_USAGE_GPU_ONLY,
									   VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL );
		
		ID_VK_CHECK( vkBindImageMemory( vkcontext.device, vkcontext.msaaImage, vkcontext.msaaAllocation.deviceMemory, vkcontext.msaaAllocation.offset ) );
#endif
		
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.format = vkcontext.swapchainFormat;
		viewInfo.image = vkcontext.msaaImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		
		ID_VK_CHECK( vkCreateImageView( vkcontext.device, &viewInfo, NULL, &vkcontext.msaaImageView ) );
	}
}

/*
=============
DestroyRenderTargets
=============
*/
static void DestroyRenderTargets()
{
	vkDestroyImageView( vkcontext.device, vkcontext.msaaImageView, NULL );
	
#if defined( USE_AMD_ALLOCATOR )
	vmaDestroyImage( vmaAllocator, vkcontext.msaaImage, vkcontext.msaaVmaAllocation );
	vkcontext.msaaAllocation = VmaAllocationInfo();
	vkcontext.msaaVmaAllocation = NULL;
#else
	vkDestroyImage( vkcontext.device, vkcontext.msaaImage, NULL );
	vulkanAllocator.Free( vkcontext.msaaAllocation );
	vkcontext.msaaAllocation = vulkanAllocation_t();
#endif
	
	vkcontext.msaaImage = VK_NULL_HANDLE;
	vkcontext.msaaImageView = VK_NULL_HANDLE;
}

/*
=============
CreateRenderPass
=============
*/
static void CreateRenderPass()
{
	VkAttachmentDescription attachments[ 3 ];
	memset( attachments, 0, sizeof( attachments ) );
	
	const bool resolve = vkcontext.sampleCount > VK_SAMPLE_COUNT_1_BIT;
	
	VkAttachmentDescription& colorAttachment = attachments[ 0 ];
	colorAttachment.format = vkcontext.swapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
	
	VkAttachmentDescription& depthAttachment = attachments[ 1 ];
	depthAttachment.format = vkcontext.depthFormat;
	depthAttachment.samples = vkcontext.sampleCount;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	VkAttachmentDescription& resolveAttachment = attachments[ 2 ];
	resolveAttachment.format = vkcontext.swapchainFormat;
	resolveAttachment.samples = vkcontext.sampleCount;
	resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
	
	VkAttachmentReference colorRef = {};
	colorRef.attachment = resolve ? 2 : 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkAttachmentReference depthRef = {};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	VkAttachmentReference resolveRef = {};
	resolveRef.attachment = 0;
	resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = &depthRef;
	if( resolve )
	{
		subpass.pResolveAttachments = &resolveRef;
	}
	
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = resolve ? 3 : 2;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 0;
	
	ID_VK_CHECK( vkCreateRenderPass( vkcontext.device, &renderPassCreateInfo, NULL, &vkcontext.renderPass ) );
}

/*
=============
CreatePipelineCache
=============
*/
static void CreatePipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ID_VK_CHECK( vkCreatePipelineCache( vkcontext.device, &pipelineCacheCreateInfo, NULL, &vkcontext.pipelineCache ) );
}

/*
=============
CreateFrameBuffers
=============
*/
static void CreateFrameBuffers()
{
	VkImageView attachments[ 3 ];
	
	// depth attachment is the same
	idImage* depthImg = globalImages->GetImage( "_viewDepth" );
	if( depthImg == NULL )
	{
		idLib::FatalError( "CreateFrameBuffers: No _viewDepth image." );
	}
	else
	{
		attachments[ 1 ] = depthImg->GetView();
	}
	
	const bool resolve = vkcontext.sampleCount > VK_SAMPLE_COUNT_1_BIT;
	if( resolve )
	{
		attachments[ 2 ] = vkcontext.msaaImageView;
	}
	
	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.renderPass = vkcontext.renderPass;
	frameBufferCreateInfo.attachmentCount = resolve ? 3 : 2;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = renderSystem->GetWidth();
	frameBufferCreateInfo.height = renderSystem->GetHeight();
	frameBufferCreateInfo.layers = 1;
	
	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		attachments[ 0 ] = vkcontext.swapchainViews[ i ];
		ID_VK_CHECK( vkCreateFramebuffer( vkcontext.device, &frameBufferCreateInfo, NULL, &vkcontext.frameBuffers[ i ] ) );
	}
}

/*
=============
DestroyFrameBuffers
=============
*/
static void DestroyFrameBuffers()
{
	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		vkDestroyFramebuffer( vkcontext.device, vkcontext.frameBuffers[ i ], NULL );
	}
	vkcontext.frameBuffers.Zero();
}

/*
=============
ClearContext
=============
*/
static void ClearContext()
{
	vkcontext.counter = 0;
	vkcontext.currentFrameData = 0;
	vkcontext.jointCacheHandle = 0;
	memset( vkcontext.stencilOperations, 0, sizeof( vkcontext.stencilOperations ) );
	vkcontext.instance = VK_NULL_HANDLE;
	vkcontext.physicalDevice = VK_NULL_HANDLE;
	vkcontext.device = VK_NULL_HANDLE;
	vkcontext.graphicsQueue = VK_NULL_HANDLE;
	vkcontext.presentQueue = VK_NULL_HANDLE;
	vkcontext.graphicsFamilyIdx = -1;
	vkcontext.presentFamilyIdx = -1;
	vkcontext.callback = VK_NULL_HANDLE;
	vkcontext.instanceExtensions.Clear();
	vkcontext.deviceExtensions.Clear();
	vkcontext.validationLayers.Clear();
	vkcontext.gpu = NULL;
	vkcontext.gpus.Clear();
	vkcontext.commandPool = VK_NULL_HANDLE;
	vkcontext.commandBuffer.Zero();
	vkcontext.commandBufferFences.Zero();
	vkcontext.commandBufferRecorded.Zero();
	vkcontext.surface = VK_NULL_HANDLE;
	vkcontext.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	vkcontext.depthFormat = VK_FORMAT_UNDEFINED;
	vkcontext.renderPass = VK_NULL_HANDLE;
	vkcontext.pipelineCache = VK_NULL_HANDLE;
	vkcontext.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	vkcontext.supersampling = false;
	vkcontext.fullscreen = 0;
	vkcontext.swapchain = VK_NULL_HANDLE;
	vkcontext.swapchainFormat = VK_FORMAT_UNDEFINED;
	vkcontext.currentSwapIndex = 0;
	vkcontext.msaaImage = VK_NULL_HANDLE;
	vkcontext.msaaImageView = VK_NULL_HANDLE;
	vkcontext.swapchainImages.Zero();
	vkcontext.swapchainViews.Zero();
	vkcontext.frameBuffers.Zero();
	vkcontext.acquireSemaphores.Zero();
	vkcontext.renderCompleteSemaphores.Zero();
	vkcontext.currentImageParm = 0;
	vkcontext.imageParms.Zero();
}

/*
=============
idRenderBackend::idRenderBackend
=============
*/
idRenderBackend::idRenderBackend()
{
	ClearContext();
}

/*
=============
idRenderBackend::~idRenderBackend
=============
*/
idRenderBackend::~idRenderBackend()
{

}

/*
=============
idRenderBackend::Init
=============
*/
void idRenderBackend::Init()
{
	if( tr.IsInitialized() )
	{
		idLib::FatalError( "R_InitVulkan called while active" );
	}
	
	// DG: make sure SDL has setup video so getting supported modes in R_SetNewMode() works
	GLimp_PreInit();
	// DG end
	
	R_SetNewMode( true );
	
	// input and sound systems need to be tied to the new window
	Sys_InitInput();
	
	idLib::Printf( "----- Initializing Vulkan driver -----\n" );
	
	glConfig.driverType = GLDRV_VULKAN;
	glConfig.gpuSkinningAvailable = true;
	
	// create the Vulkan instance and enable validation layers
	CreateVulkanInstance();
	
	// create the windowing interface
#ifdef _WIN32
	CreateSurface();
#endif
	
	// Enumerate physical devices and get their properties
	EnumeratePhysicalDevices();
	
	// Find queue family/families supporting graphics and present.
	SelectPhysicalDevice();
	
	// Create logical device and queues
	CreateLogicalDeviceAndQueues();
	
	// Create semaphores for image acquisition and rendering completion
	CreateSemaphores();
	
	// Create Command Pool
	CreateCommandPool();
	
	// Create Command Buffer
	CreateCommandBuffer();
	
	// setup the allocator
#if defined( USE_AMD_ALLOCATOR )
	extern idCVar r_vkHostVisibleMemoryMB;
	extern idCVar r_vkDeviceLocalMemoryMB;
	
	VmaAllocatorCreateInfo createInfo = {};
	createInfo.physicalDevice = vkcontext.physicalDevice;
	createInfo.device = vkcontext.device;
	createInfo.preferredSmallHeapBlockSize = r_vkHostVisibleMemoryMB.GetInteger() * 1024 * 1024;
	createInfo.preferredLargeHeapBlockSize = r_vkDeviceLocalMemoryMB.GetInteger() * 1024 * 1024;
	
	vmaCreateAllocator( &createInfo, &vmaAllocator );
#else
	vulkanAllocator.Init();
#endif
	
	// Start the Staging Manager
	stagingManager.Init();
	
	// Create Swap Chain
	CreateSwapChain();
	
	// Create Render Targets
	CreateRenderTargets();
	
	// Create Render Pass
	CreateRenderPass();
	
	// Create Pipeline Cache
	CreatePipelineCache();
	
	// Create Frame Buffers
	CreateFrameBuffers();
	
	// init RenderProg Manager
	renderProgManager.Init();
	
	// init Vertex Cache
	vertexCache.Init( vkcontext.gpu->props.limits.minUniformBufferOffsetAlignment );
}

/*
=============
idRenderBackend::Shutdown
=============
*/
void idRenderBackend::Shutdown()
{
	// RB: release input before anything goes wrong
	Sys_ShutdownInput();
	
	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		idImage::EmptyGarbage();
	}
	
	// Detroy Frame Buffers
	DestroyFrameBuffers();
	
	// Destroy Pipeline Cache
	vkDestroyPipelineCache( vkcontext.device, vkcontext.pipelineCache, NULL );
	
	// Destroy Render Pass
	vkDestroyRenderPass( vkcontext.device, vkcontext.renderPass, NULL );
	
	// Destroy Render Targets
	DestroyRenderTargets();
	
	// Destroy Swap Chain
	DestroySwapChain();
	
	// Stop the Staging Manager
	stagingManager.Shutdown();
	
	// Destroy Command Buffer
	vkFreeCommandBuffers( vkcontext.device, vkcontext.commandPool, NUM_FRAME_DATA, vkcontext.commandBuffer.Ptr() );
	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		vkDestroyFence( vkcontext.device, vkcontext.commandBufferFences[ i ], NULL );
	}
	
	// Destroy Command Pool
	vkDestroyCommandPool( vkcontext.device, vkcontext.commandPool, NULL );
	
	// Destroy Semaphores
	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		vkDestroySemaphore( vkcontext.device, vkcontext.acquireSemaphores[ i ], NULL );
		vkDestroySemaphore( vkcontext.device, vkcontext.renderCompleteSemaphores[ i ], NULL );
	}
	
	// Destroy Debug Callback
	if( r_vkEnableValidationLayers.GetBool() )
	{
		DestroyDebugReportCallback();
	}
	
	// dump all our memory
#if defined( USE_AMD_ALLOCATOR )
	vmaDestroyAllocator( vmaAllocator );
#else
	vulkanAllocator.Shutdown();
#endif
	
	// Destroy Logical Device
	vkDestroyDevice( vkcontext.device, NULL );
	
	// Destroy Surface
	vkDestroySurfaceKHR( vkcontext.instance, vkcontext.surface, NULL );
	
	// Destroy the Instance
	vkDestroyInstance( vkcontext.instance, NULL );
	
	ClearContext();
	
	// destroy main window
	GLimp_Shutdown();
}




bool GL_CheckErrors_( const char* filename, int line )
{
	return false;
}

/*
=============
idRenderBackend::DrawElementsWithCounters
=============
*/
void idRenderBackend::DrawElementsWithCounters( const drawSurf_t* surf )
{
	// get vertex buffer
	const vertCacheHandle_t vbHandle = surf->ambientCache;
	idVertexBuffer* vertexBuffer;
	if( vertexCache.CacheIsStatic( vbHandle ) )
	{
		vertexBuffer = &vertexCache.staticData.vertexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( vbHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, vertexBuffer == NULL" );
			return;
		}
		vertexBuffer = &vertexCache.frameData[vertexCache.drawListNum].vertexBuffer;
	}
	const int vertOffset = ( int )( vbHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	
	// get index buffer
	const vertCacheHandle_t ibHandle = surf->indexCache;
	idIndexBuffer* indexBuffer;
	if( vertexCache.CacheIsStatic( ibHandle ) )
	{
		indexBuffer = &vertexCache.staticData.indexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( ibHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, indexBuffer == NULL" );
			return;
		}
		indexBuffer = &vertexCache.frameData[vertexCache.drawListNum].indexBuffer;
	}
	// RB: 64 bit fixes, changed int to ptrdiff_t
	const ptrdiff_t indexOffset = ( ptrdiff_t )( ibHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	
	RENDERLOG_PRINTF( "Binding Buffers: %p:%i %p:%i\n", vertexBuffer, vertOffset, indexBuffer, indexOffset );
	
	if( surf->jointCache )
	{
		// DG: this happens all the time in the erebus1 map with blendlight.vfp,
		// so don't call assert (through verify) here until it's fixed (if fixable)
		// else the game crashes on linux when using debug builds
		
		// FIXME: fix this properly if possible?
		// RB: yes but it would require an additional blend light skinned shader
		//if( !verify( renderProgManager.ShaderUsesJoints() ) )
		if( !renderProgManager.ShaderUsesJoints() )
			// DG end
		{
			return;
		}
	}
	else
	{
		if( !verify( !renderProgManager.ShaderUsesJoints() || renderProgManager.ShaderHasOptionalSkinning() ) )
		{
			return;
		}
	}
	
	
	if( surf->jointCache )
	{
		idUniformBuffer jointBuffer;
		if( !vertexCache.GetJointBuffer( surf->jointCache, &jointBuffer ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, jointBuffer == NULL" );
			return;
		}
		assert( ( jointBuffer.GetOffset() & ( glConfig.uniformBufferOffsetAlignment - 1 ) ) == 0 );
		
		// FIXME
		
		//const GLintptr ubo = jointBuffer.GetAPIObject();
		//glBindBufferRange( GL_UNIFORM_BUFFER, 0, ubo, jointBuffer.GetOffset(), jointBuffer.GetSize() );
	}
	
	renderProgManager.CommitUniforms( glStateBits );
	
	
	/*
	if( currentIndexBuffer != ( GLintptr )indexBuffer->GetAPIObject() || !r_useStateCaching.GetBool() )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ( GLintptr )indexBuffer->GetAPIObject() );
		currentIndexBuffer = ( GLintptr )indexBuffer->GetAPIObject();
	}
	*/
	
	{
		const VkBuffer buffer = indexBuffer->GetAPIObject();
		const VkDeviceSize offset = indexBuffer->GetOffset();
		vkCmdBindIndexBuffer( vkcontext.commandBuffer[ vkcontext.currentFrameData ], buffer, offset, VK_INDEX_TYPE_UINT16 );
	}
	
	/*
	if( ( vertexLayout != LAYOUT_DRAW_VERT ) || ( currentVertexBuffer != ( GLintptr )vertexBuffer->GetAPIObject() ) || !r_useStateCaching.GetBool() )
	{
		glBindBuffer( GL_ARRAY_BUFFER, ( GLintptr )vertexBuffer->GetAPIObject() );
		currentVertexBuffer = ( GLintptr )vertexBuffer->GetAPIObject();
	
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_VERTEX );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
	
		glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), ( void* )( DRAWVERT_XYZ_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_NORMAL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_NORMAL_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_COLOR_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_COLOR2_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_ST_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_TANGENT, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_TANGENT_OFFSET ) );
	
		vertexLayout = LAYOUT_DRAW_VERT;
	}
	*/
	
	{
		const VkBuffer buffer = vertexBuffer->GetAPIObject();
		const VkDeviceSize offset = vertexBuffer->GetOffset();
		vkCmdBindVertexBuffers( vkcontext.commandBuffer[ vkcontext.currentFrameData ], 0, 1, &buffer, &offset );
	}
	
	/*
	glDrawElementsBaseVertex( GL_TRIANGLES,
							  r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
							  GL_INDEX_TYPE,
							  ( triIndex_t* )indexOffset,
							  vertOffset / sizeof( idDrawVert ) );
	*/
	
	vkCmdDrawIndexed(
		vkcontext.commandBuffer[ vkcontext.currentFrameData ],
		surf->numIndexes, 1, ( indexOffset >> 1 ), vertOffset / sizeof( idDrawVert ), 0 );
		
	// RB: added stats
	pc.c_drawElements++;
	pc.c_drawIndexes += surf->numIndexes;
}


/*
=========================================================================================================

GL COMMANDS

=========================================================================================================
*/

/*

     Drawing a frame

- Aquire the next swapchain image to use - vkAquireNextImageKHR
- Submit the command buffer to a queue - vkQueueSubmit
- Present the backbuffer - vkQueuePresentKHR
- Synchronize with Semaphores



				  |--------------------------|   |-------------------------|
                  |  vkAquireNextImageKHR	 |   |  vkQueuePresentKHR      |
                  |--------------------------|   |-------------------------|
                                    |                  ^
                                    |                  |
                                   `´                  |
         |---------------------------------|    |---------------------------------|
         |                                 |    |                                 |
         |      Backbuffer Semaphore       |    |   Render Complete Semaphore     |
         |                                 |    |								  |
         |:--------------------------------|    |---------------------------------|
                                    |                  ^
                                    |                  |
                                    `´                 |
                                |-------------------------|
								|      vkQueueSubmit      |
                                |-------------------------|
*/

/*
==================
idRenderBackend::GL_StartFrame
==================
*/
void idRenderBackend::GL_StartFrame()
{
	ID_VK_CHECK( vkAcquireNextImageKHR( vkcontext.device, vkcontext.swapchain, UINT64_MAX, vkcontext.acquireSemaphores[ vkcontext.currentFrameData ], VK_NULL_HANDLE, &vkcontext.currentSwapIndex ) );
	
	idImage::EmptyGarbage();
	
#if !defined( USE_AMD_ALLOCATOR )
	vulkanAllocator.EmptyGarbage();
#endif
	stagingManager.Flush();
	
	//TODO renderProgManager.StartFrame();
	//vkResetDescriptorPool( vkcontext.device, descriptorPools[ currentData ], 0 );
	
	
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	ID_VK_CHECK( vkBeginCommandBuffer( vkcontext.commandBuffer[ vkcontext.currentFrameData ], &commandBufferBeginInfo ) );
	
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = vkcontext.renderPass;
	renderPassBeginInfo.framebuffer = vkcontext.frameBuffers[ vkcontext.currentSwapIndex ];
	renderPassBeginInfo.renderArea.extent = vkcontext.swapchainExtent;
	
	vkCmdBeginRenderPass( vkcontext.commandBuffer[ vkcontext.currentFrameData ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
}

/*
==================
idRenderBackend::GL_EndFrame
==================
*/
void idRenderBackend::GL_EndFrame()
{
	VkCommandBuffer commandBuffer = vkcontext.commandBuffer[ vkcontext.currentFrameData ];
	
	vkCmdEndRenderPass( commandBuffer );
	
	// Transition our swap image to present.
	// Do this instead of having the renderpass do the transition
	// so we can take advantage of the general layout to avoid
	// additional image barriers.
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkcontext.swapchainImages[ vkcontext.currentSwapIndex ];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
#if 0
	barrier.srcAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT |
							VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	barrier.dstAccessMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
#else
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = 0;
#endif
	
	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier );
		
	ID_VK_CHECK( vkEndCommandBuffer( commandBuffer ) )
	vkcontext.commandBufferRecorded[ vkcontext.currentFrameData ] = true;
	
	VkSemaphore* acquire = &vkcontext.acquireSemaphores[ vkcontext.currentFrameData ];
	VkSemaphore* finished = &vkcontext.renderCompleteSemaphores[ vkcontext.currentFrameData ];
	
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkcontext.commandBuffer[ vkcontext.currentFrameData ];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = acquire;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = finished;
	submitInfo.pWaitDstStageMask = &dstStageMask;
	
	ID_VK_CHECK( vkQueueSubmit( vkcontext.graphicsQueue, 1, &submitInfo, vkcontext.commandBufferFences[ vkcontext.currentFrameData ] ) );
}

/*
=============
GL_BlockingSwapBuffers

We want to exit this with the GPU idle, right at vsync
=============
*/
void idRenderBackend::GL_BlockingSwapBuffers()
{
	RENDERLOG_PRINTF( "***************** BlockingSwapBuffers *****************\n\n\n" );
	
	if( vkcontext.commandBufferRecorded[ vkcontext.currentFrameData ] == false )
	{
		// RB: no need to present anything if no command buffers where recorded in this frame
		return;
	}
	
	ID_VK_CHECK( vkWaitForFences( vkcontext.device, 1, &vkcontext.commandBufferFences[ vkcontext.currentFrameData ], VK_TRUE, UINT64_MAX ) );
	
	ID_VK_CHECK( vkResetFences( vkcontext.device, 1, &vkcontext.commandBufferFences[ vkcontext.currentFrameData ] ) );
	vkcontext.commandBufferRecorded[ vkcontext.currentFrameData ] = false;
	
	VkSemaphore* finished = &vkcontext.renderCompleteSemaphores[ vkcontext.currentFrameData ];
	
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = finished;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &vkcontext.swapchain;
	presentInfo.pImageIndices = &vkcontext.currentSwapIndex;
	
	ID_VK_CHECK( vkQueuePresentKHR( vkcontext.presentQueue, &presentInfo ) );
	
	// RB: at this time the image is presented on the screen
	
	vkcontext.counter++;
	vkcontext.currentFrameData = vkcontext.counter % NUM_FRAME_DATA;
}

/*
========================
GL_SetDefaultState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
========================
*/
void idRenderBackend::GL_SetDefaultState()
{
	RENDERLOG_PRINTF( "--- GL_SetDefaultState ---\n" );
	
	glStateBits = 0;
	
	hdrAverageLuminance = 0;
	hdrMaxLuminance = 0;
	hdrTime = 0;
	hdrKey = 0;
	
	GL_State( 0, true );
	
	GL_Scissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
	
	// RB begin
	Framebuffer::Unbind();
	
	// RB: don't keep renderprogs that were enabled during level load
	renderProgManager.Unbind();
}

/*
====================
idRenderBackend::GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void idRenderBackend::GL_State( uint64 stateBits, bool forceGlState )
{
	glStateBits = stateBits | ( glStateBits & GLS_KEEP );
	if( viewDef != NULL && viewDef->isMirror )
	{
		glStateBits |= GLS_MIRROR_VIEW;
	}
}

/*
====================
idRenderBackend::SelectTexture
====================
*/
void idRenderBackend::GL_SelectTexture( int index )
{
	if( vkcontext.currentImageParm == index )
	{
		return;
	}
	
	RENDERLOG_PRINTF( "GL_SelectTexture( %d );\n", index );
	
	vkcontext.currentImageParm = index;
}


/*
====================
idRenderBackend::GL_Scissor
====================
*/
void idRenderBackend::GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h )
{
	VkRect2D scissor;
	scissor.offset.x = x;
	scissor.offset.y = y;
	scissor.extent.width = w;
	scissor.extent.height = h;
	
	vkCmdSetScissor( vkcontext.commandBuffer[ vkcontext.currentFrameData ], 0, 1, &scissor );
}

/*
====================
idRenderBackend::GL_Viewport
====================
*/
void idRenderBackend::GL_Viewport( int x /* left */, int y /* bottom */, int w, int h )
{
	VkViewport viewport;
	viewport.x = x;
	viewport.y = y;
	viewport.width = w;
	viewport.height = h;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	
	vkCmdSetViewport( vkcontext.commandBuffer[ vkcontext.currentFrameData ], 0, 1, &viewport );
}

/*
====================
idRenderBackend::GL_PolygonOffset
====================
*/
void idRenderBackend::GL_PolygonOffset( float scale, float bias )
{
	// TODO
}

/*
========================
idRenderBackend::GL_DepthBoundsTest
========================
*/
void idRenderBackend::GL_DepthBoundsTest( const float zmin, const float zmax )
{
	// TODO
}

/*
====================
idRenderBackend::GL_Color
====================
*/
void idRenderBackend::GL_Color( float r, float g, float b, float a )
{
	float parm[4];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, r );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, g );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, b );
	parm[3] = idMath::ClampFloat( 0.0f, 1.0f, a );
	renderProgManager.SetRenderParm( RENDERPARM_COLOR, parm );
}

/*
========================
idRenderBackend::GL_Clear
========================
*/
void idRenderBackend::GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a, bool clearHDR )
{
	RENDERLOG_PRINTF( "GL_Clear( color=%d, depth=%d, stencil=%d, stencil=%d, r=%f, g=%f, b=%f, a=%f )\n",
					  color, depth, stencil, stencilValue, r, g, b, a );
					  
	uint32 numAttachments = 0;
	VkClearAttachment attachments[ 2 ];
	memset( attachments, 0, sizeof( attachments ) );
	
	if( color )
	{
		VkClearAttachment& attachment = attachments[ numAttachments++ ];
		attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		attachment.colorAttachment = 0;
		
		VkClearColorValue& color = attachment.clearValue.color;
		color.float32[ 0 ] = r;
		color.float32[ 1 ] = g;
		color.float32[ 2 ] = b;
		color.float32[ 3 ] = a;
	}
	
	if( depth || stencil )
	{
		VkClearAttachment& attachment = attachments[ numAttachments++ ];
		
		if( depth )
		{
			attachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		
		if( stencil )
		{
			attachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		
		attachment.clearValue.depthStencil.depth = 1.0f;
		attachment.clearValue.depthStencil.stencil = stencilValue;
	}
	
	VkClearRect clearRect = {};
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect.extent = vkcontext.swapchainExtent;
	
	vkCmdClearAttachments( vkcontext.commandBuffer[ vkcontext.currentFrameData ], numAttachments, attachments, 1, &clearRect );
	
	/*
	int clearFlags = 0;
	if( color )
	{
		glClearColor( r, g, b, a );
		clearFlags |= GL_COLOR_BUFFER_BIT;
	}
	if( depth )
	{
		clearFlags |= GL_DEPTH_BUFFER_BIT;
	}
	if( stencil )
	{
		glClearStencil( stencilValue );
		clearFlags |= GL_STENCIL_BUFFER_BIT;
	}
	glClear( clearFlags );
	
	// RB begin
	if( r_useHDR.GetBool() && clearHDR && globalFramebuffers.hdrFBO != NULL )
	{
		bool isDefaultFramebufferActive = Framebuffer::IsDefaultFramebufferActive();
	
		globalFramebuffers.hdrFBO->Bind();
		glClear( clearFlags );
	
		if( isDefaultFramebufferActive )
		{
			Framebuffer::Unbind();
		}
	}
	*/
}


/*
=================
idRenderBackend::GL_GetCurrentState
=================
*/
uint64 idRenderBackend::GL_GetCurrentState() const
{
	return glStateBits;
}

/*
========================
idRenderBackend::GL_GetCurrentStateMinusStencil
========================
*/
uint64 idRenderBackend::GL_GetCurrentStateMinusStencil() const
{
	return GL_GetCurrentState() & ~( GLS_STENCIL_OP_BITS | GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS );
}


/*
=============
idRenderBackend::CheckCVars

See if some cvars that we watch have changed
=============
*/
void idRenderBackend::CheckCVars()
{
	// TODO
	
	/*
	// gamma stuff
	if( r_gamma.IsModified() || r_brightness.IsModified() )
	{
		r_gamma.ClearModified();
		r_brightness.ClearModified();
		R_SetColorMappings();
	}
	
	// filtering
	if( r_maxAnisotropicFiltering.IsModified() || r_useTrilinearFiltering.IsModified() || r_lodBias.IsModified() )
	{
		idLib::Printf( "Updating texture filter parameters.\n" );
		r_maxAnisotropicFiltering.ClearModified();
		r_useTrilinearFiltering.ClearModified();
		r_lodBias.ClearModified();
	
		for( int i = 0 ; i < globalImages->images.Num() ; i++ )
		{
			if( globalImages->images[i] )
			{
				globalImages->images[i]->Bind();
				globalImages->images[i]->SetTexParameters();
			}
		}
	}
	
	extern idCVar r_useSeamlessCubeMap;
	if( r_useSeamlessCubeMap.IsModified() )
	{
		r_useSeamlessCubeMap.ClearModified();
		if( glConfig.seamlessCubeMapAvailable )
		{
			if( r_useSeamlessCubeMap.GetBool() )
			{
				glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
			}
			else
			{
				glDisable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
			}
		}
	}
	
	extern idCVar r_useSRGB;
	if( r_useSRGB.IsModified() )
	{
		r_useSRGB.ClearModified();
		if( glConfig.sRGBFramebufferAvailable )
		{
			if( r_useSRGB.GetBool() && r_useSRGB.GetInteger() != 3 )
			{
				glEnable( GL_FRAMEBUFFER_SRGB );
			}
			else
			{
				glDisable( GL_FRAMEBUFFER_SRGB );
			}
		}
	}
	
	if( r_antiAliasing.IsModified() )
	{
		switch( r_antiAliasing.GetInteger() )
		{
			case ANTI_ALIASING_MSAA_2X:
			case ANTI_ALIASING_MSAA_4X:
			case ANTI_ALIASING_MSAA_8X:
				if( r_antiAliasing.GetInteger() > 0 )
				{
					glEnable( GL_MULTISAMPLE );
				}
				break;
	
			default:
				glDisable( GL_MULTISAMPLE );
				break;
		}
	}
	
	if( r_useHDR.IsModified() || r_useHalfLambertLighting.IsModified() )
	{
		r_useHDR.ClearModified();
		r_useHalfLambertLighting.ClearModified();
		renderProgManager.KillAllShaders();
		renderProgManager.LoadAllShaders();
	}
	*/
}

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
idRenderBackend::DrawFlickerBox
=============
*/
void idRenderBackend::DrawFlickerBox()
{
	if( !r_drawFlickerBox.GetBool() )
	{
		return;
	}
	
	// TODO
	
	/*
	if( tr.frameCount & 1 )
	{
		glClearColor( 1, 0, 0, 1 );
	}
	else
	{
		glClearColor( 0, 1, 0, 1 );
	}
	glScissor( 0, 0, 256, 256 );
	glClear( GL_COLOR_BUFFER_BIT );
	*/
}

/*
=============
idRenderBackend::SetBuffer
=============
*/
void idRenderBackend::SetBuffer( const void* data )
{
	// see which draw buffer we want to render the frame to
	
	const setBufferCommand_t* cmd = ( const setBufferCommand_t* )data;
	
	RENDERLOG_PRINTF( "---------- RB_SetBuffer ---------- to buffer # %d\n", cmd->buffer );
	
	GL_Scissor( 0, 0, tr.GetWidth(), tr.GetHeight() );
	
	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if( r_clear.GetFloat() || idStr::Length( r_clear.GetString() ) != 1 || r_singleArea.GetBool() || r_showOverDraw.GetBool() )
	{
		float c[3];
		if( sscanf( r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) == 3 )
		{
			GL_Clear( true, false, false, 0, c[0], c[1], c[2], 1.0f, true );
		}
		else if( r_clear.GetInteger() == 2 )
		{
			GL_Clear( true, false, false, 0, 0.0f, 0.0f,  0.0f, 1.0f, true );
		}
		else if( r_showOverDraw.GetBool() )
		{
			GL_Clear( true, false, false, 0, 1.0f, 1.0f, 1.0f, 1.0f, true );
		}
		else
		{
			GL_Clear( true, false, false, 0, 0.4f, 0.0f, 0.25f, 1.0f, true );
		}
	}
}





/*
====================
idRenderBackend::StereoRenderExecuteBackEndCommands

Renders the draw list twice, with slight modifications for left eye / right eye
====================
*/
void idRenderBackend::StereoRenderExecuteBackEndCommands( const emptyCommand_t* const allCmds )
{
	// RB: TODO ?
}




/*
==============================================================================================

STENCIL SHADOW RENDERING

==============================================================================================
*/

/*
=====================
idRenderBackend::DrawStencilShadowPass
=====================
*/
void idRenderBackend::DrawStencilShadowPass( const drawSurf_t* drawSurf, const bool renderZPass )
{
}


/*
==============================================================================================

OFFSCREEN RENDERING

==============================================================================================
*/

void Framebuffer::Init()
{
	// TODO
}

void Framebuffer::Shutdown()
{
	// TODO
}

bool Framebuffer::IsDefaultFramebufferActive()
{
	// TODO
	return true;
}

void Framebuffer::Bind()
{
	// TODO
}

void Framebuffer::Unbind()
{
	// TODO
}

bool Framebuffer::IsBound()
{
	// TODO
	return true;
}

void Framebuffer::Check()
{
	// TODO
}

void Framebuffer::CheckFramebuffers()
{
	// TODO
}