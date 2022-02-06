cmake_minimum_required(VERSION 3.2)

project(RBDoom3BFG)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if(CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION GREATER_EQUAL 6)
	# set MSVC startup project
	set_property (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RBDoom3BFG)
endif()
	
option(STANDALONE
		"Skip DOOM-3-BFG base/ folder and use assets/ instead" OFF)

option(DOOM_CLASSIC
		"Build with Doom 1 & 2" ON)

option(FORCE_COLOR_OUTPUT
		"Always produce ANSI-colored output (GNU/Clang only)." OFF)

option(COMPILE_COMMANDS
		"Generate compile_commands.json" OFF)

option(USE_MFC_TOOLS
		"Compile the legacy MFC based tools (unsupported)" OFF)

option(USE_PRECOMPILED_HEADERS
		"Use precompiled headers during build" ON)
		
option(SDL2
		"Use SDL2 instead of SDL1.2" ON)
		
option(OPENAL
		"Use OpenAL soft instead of XAudio2" OFF)

option(FFMPEG
		"Use FMPEG to render Bink videos" ON)

option(BINKDEC
		"Use included libbinkdec to render Bink videos" OFF)
		
option(USE_VULKAN
		"Use Vulkan instead of OpenGL" OFF)

option(SPIRV_SHADERC
		"Compile SPIR-V shader byte code using shaderc instead of using Glslang directly" OFF)

option(USE_MoltenVK
		"Use MoltenVK library directly instead of Vulkan loader on macOS" OFF)

option(ONATIVE
		"Optimize for the host CPU" OFF)
		
option(WINDOWS10
		"Build for Windows 10+" OFF)
		
option(USE_SYSTEM_ZLIB
                "Use the system zlib instead of the bundled one" OFF)

option(USE_SYSTEM_LIBPNG
                "Use the system libpng instead of the bundled one" OFF)

option(USE_SYSTEM_LIBJPEG
                "Use the system libjpeg instead of the bundled one" OFF)

option(USE_SYSTEM_LIBGLEW
                "Use the system libglew instead of the bundled one" OFF)

option(USE_SYSTEM_RAPIDJSON
                "Use the system rapidjson instead of the bundled one" OFF)

set(CPU_TYPE "" CACHE STRING "When set, passes this string as CPU-ID which will be embedded into the binary.")

# SRS - Turn on optimization when cross-compiling from Apple arm64 to x86_64
if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)" OR CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
	set(CPU_OPTIMIZATION "-mmmx -msse -msse2" CACHE STRING "Which CPU specific optimitations should be used beside the compiler's default?")
endif()

# SRS - Turn off MMX/SSE intrinsics when cross-compiling from Apple x86_64 to arm64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)|(e2k)|(E2K)" AND NOT CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
	option(USE_INTRINSICS_SSE "Compile using SSE intrinsics (e.g mmx, sse, msse2)" ON)
else()
	option(USE_INTRINSICS_SSE "Compile using SSE intrinsics (e.g mmx, sse, msse2)" OFF)
endif()

if(FFMPEG AND BINKDEC)
	message(FATAL_ERROR "Only one of FFMPEG and BINKDEC (or neither) can be enabled at a time")
endif()

if(UNIX)
	set(OPENAL ON)
	
    # SRS - Disable precompiled headers when cross-compiling on OSX, e.g. when creating universal binaries
    if((CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64" AND CMAKE_OSX_ARCHITECTURES MATCHES "arm64" ) OR
       (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64"  AND CMAKE_OSX_ARCHITECTURES MATCHES "x86_64"))
        set(USE_PRECOMPILED_HEADERS OFF)
    endif()

	if(USE_VULKAN)
		SET(SDL2 ON)
		
		# RB: disable precompiled headers on Linux with Vulkan because of error: POSIX thread support was disabled in PCH file but is currently enabled
		if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
            set(USE_PRECOMPILED_HEADERS OFF)
        endif()
	endif()
endif()

if(COMPILE_COMMANDS)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if (FORCE_COLOR_OUTPUT)
	if (CMAKE_COMPILER_IS_GNUCC)
		add_compile_options(-fdiagnostics-color=always)
	elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
		add_compile_options(-fcolor-diagnostics)
	endif ()
endif ()

if(MSVC)
	#message(STATUS CMAKE_ROOT: ${CMAKE_ROOT})
	
	#if(CMAKE_CL_64)
	#	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin/win64)
	#else()
	#	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin/win32)
	#endif()
	
	#message(STATUS EXECUTABLE_OUTPUT_PATH: ${EXECUTABLE_OUTPUT_PATH})
	#message(STATUS PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR})
	#message(STATUS CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR})
else()
	message(STATUS CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE})
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")

	add_definitions(-pipe)
	#add_definitions(-Wall)
	add_definitions(-Werror=format-security)
	add_definitions(-Werror=format)

	# Compiler check (needs -std=c++11 flag)
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	if(COMPILER_SUPPORTS_CXX11)
		set(CMAKE_CXX_STANDARD 11)
		set(CMAKE_CXX_STANDARD_REQUIRED ON)
		set(CMAKE_CXX_EXTENSIONS OFF)
	else()
		message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
	endif()

	if(CPU_TYPE)
		add_definitions(-DCPUSTRING="${CPU_TYPE}")
	endif()
	if (CPU_OPTIMIZATION)
		add_definitions(${CPU_OPTIMIZATION})
	endif()
	if(WIN32)
		# require msvcr70.dll or newer for _aligned_malloc etc
		# I think it is from Visual C++ .NET 2002, so it should be available on any remotely modern system.
		add_definitions(-D__MSVCRT_VERSION__=0x0700)
		
		find_package(DirectX REQUIRED)
		include_directories(libs/mingw-hacks)
		include_directories(${DirectX_INCLUDE_DIR})
		add_definitions(-DUSE_XINPUT)
	endif()	
	
	#if(NOT ANDROID)
		add_definitions(-DUSE_EXCEPTIONS)
	#endif()
	
	add_compile_options(-Wno-pragmas -Wno-unused-variable -Wno-switch -Wno-unused-value -Winvalid-pch -Wno-multichar)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
		# add clang-specific settings for warnings (the second one make sure clang doesn't complain
		# about unknown -W flags, like -Wno-unused-but-set-variable)
		# SRS - Add -Wno-deprecated-register and -Wno-expansion-to-defined to list of warning settings
		add_definitions(-Wno-local-type-template-args -Wno-unknown-warning-option -Wno-inline-new-delete -Wno-switch-enum -Wno-deprecated-register -Wno-expansion-to-defined)
	endif()
	
	if(NOT CMAKE_CROSSCOMPILING AND ONATIVE)
		if(CMAKE_SYSTEM_PROCESSOR MATCHES "((powerpc|ppc)64le)|(mips64)")
			add_definitions(-mcpu=native)
		else()
			add_definitions(-march=native)
		endif()
	endif()

	add_compile_options(-fno-strict-aliasing)

	# SRS - Make sure OSX can find system headers and add support for minimum OSX runtime version
	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		# SRS - Also add -fasm-blocks otherwise Xcode complains and -Qunused-arguments to silence MMX/SSE unused arg warnings when compiling for Apple arm64
		add_definitions(-fasm-blocks -Qunused-arguments)
        if(CMAKE_OSX_SYSROOT)
            add_definitions(-isysroot "${CMAKE_OSX_SYSROOT}")
            message(STATUS "Using macOS sysroot: " ${CMAKE_OSX_SYSROOT})
        endif()
		if(CMAKE_OSX_DEPLOYMENT_TARGET)
			add_definitions(-mmacosx-version-min="${CMAKE_OSX_DEPLOYMENT_TARGET}")
		endif()
	endif()

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -D_DEBUG -O0")
    #set(CMAKE_C_FLAGS_DEBUGALL "${CMAKE_C_FLAGS_DEBUGALL} -g -ggdb -D_DEBUG")
    #set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE} -g -ggdb -D_DEBUG -O1 -fno-omit-frame-pointer")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffp-contract=off -fomit-frame-pointer -fno-math-errno -fno-trapping-math")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g -O3 -ffp-contract=off -fomit-frame-pointer -fno-math-errno -fno-trapping-math")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -ffp-contract=off -fomit-frame-pointer -fno-math-errno -fno-trapping-math")
	
	set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUGALL ${CMAKE_C_FLAGS_DEBUGALL})
	#set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})
	set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
	set(CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL})

elseif(MSVC)
	
	# Multiprocessor support for compiling
	# /MP
	
	# /MT  use multithreaded, static runtime library
	#      (=> binary won't need MSVCP100D.dll or similar)
	# /MTd the same with debug symbols
	# /MD and /MDd - the same with dynamic runtime lib
	#     (=> MSVCP100D.dll or similar is needed to run binary)

	# Inline Function Expansion: Any Suitable (/Ob2)
	# "/Ob2",
			
	# enable Intrinsic Functions
	# "/Oi",
			
	# Omit Frame Pointers
	# "/Oy",
	
	# if(MSVC_VERSION EQUAL 1700)
		# #message(STATUS "$ENV{LIBPATH}")
		
		# find_path(Windows_winmd_DIR NAMES Windows.winmd
			# HINTS
			# C:/Program Files (x86)/Windows Kits/8.0/References/CommonConfiguration/Neutral
			# PATH_SUFFIXES "Neutral")
			
		# message(STATUS "${Windows_winmd_DIR}")
		
		# #set(WINRT_OPTIONS "/ZW /D USE_WINRT /AI \"C:\Program Files (x86)\Windows Kits\8.0\References\CommonConfiguration\Neutral\"")
		# #set(WINRT_OPTIONS "/ZW /D USE_WINRT /AI \"$ENV{LIBPATH}\" ")
		# set(WINRT_OPTIONS "/ZW /D USE_WINRT")
	
		# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D _DEBUG /MP ${WINRT_OPTIONS}")
		# set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP /Oi /Oy ${WINRT_OPTIONS}")
		# set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MP /Oi ${WINRT_OPTIONS}")
		# set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MP /Oi /Oy ${WINRT_OPTIONS}")
		
		# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP ${WINRT_OPTIONS}")
		# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /Oi /Oy ${WINRT_OPTIONS}")
		# set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP ${WINRT_OPTIONS}")
		# set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP /Oi /Oy ${WINRT_OPTIONS}")
	# else()
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D _DEBUG /MP /MDd")
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP /Oi /Oy /MD")
		set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MP /Oi /MDd")
		set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MP /Oi /Oy /MD")
		
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP /MDd")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /Oi /Oy /MD")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /MDd")
		set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP /Oi /Oy /MD")
		
		# RB: without /SAFESEH:NO we can't link against ffmpeg libs and VS2013 or we have to build our own
		# libs for newer compilers
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /SAFESEH:NO")
	# endif()

	add_definitions(-DWIN32 
					-D_WINDOWS
					-D_CRT_SECURE_NO_DEPRECATE
					-D_CRT_NONSTDC_NO_DEPRECATE
					-D_CRT_SECURE_NO_WARNINGS
					-D_MBCS
					-DUSE_EXCEPTIONS)

	if( WINDOWS10 ) # Windows RT
		add_definitions(-DUSE_WINRT)
	endif()
					
	if(NOT CMAKE_CL_64)
		add_definitions(-D_USE_32BIT_TIME_T)
	endif()
					
	if(USE_MFC_TOOLS)
		add_definitions(-D_AFXDLL)
		
		# 0: Use Standard Windows Libraries
		# 1: Use MFC in a Static Library
		# 2: Use MFC in a Shared DLL 
		set(CMAKE_MFC_FLAG 2)
	endif()

endif()

if (USE_INTRINSICS_SSE)
	add_definitions(-DUSE_INTRINSICS_SSE)
endif()

if(STANDALONE)
	add_definitions(-DSTANDALONE)

	set(DOOM_CLASSIC OFF)
endif()

if (USE_SYSTEM_ZLIB)
  find_package(ZLIB REQUIRED)
endif(USE_SYSTEM_ZLIB)

if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    set(ZLIB_LIBRARY ${ZLIB_LIBRARIES})
else (ZLIB_FOUND)
    include_directories("libs/zlib")
    set(ZLIB_LIBRARY "" )
endif (ZLIB_FOUND)

if(USE_SYSTEM_LIBPNG)
	find_package(PNG REQUIRED)
endif (USE_SYSTEM_LIBPNG)

if (PNG_FOUND)
    include_directories(${PNG_INCLUDE_DIRS})
    set(PNG_LIBRARY ${PNG_LIBRARIES})
else (PNG_FOUND)
    include_directories("libs/png")
    set(PNG_LIBRARY "" )
endif (PNG_FOUND)

if(USE_SYSTEM_LIBJPEG)
	find_package(JPEG REQUIRED)
endif(USE_SYSTEM_LIBJPEG)

if (JPEG_FOUND)
    include_directories(${JPEG_INCLUDE_DIRS})
    set(JPEG_LIBRARY ${JPEG_LIBRARIES})
    add_definitions(-DUSE_NEWER_JPEG)
else (JPEG_FOUND)
    include_directories("libs/jpeg-6")
    set(JPEG_LIBRARY "" )
endif (JPEG_FOUND)


macro(SET_OPTION option value)
  set(${option} ${value} CACHE "" INTERNAL FORCE)
endmacro()

if(USE_VULKAN)

	# RB: moved this above the general Vulkan part so glslang does not include Vulkan SDK headers
	# which causes all kinds of weird segmentation faults because struct sizes don't match
 
    # SRS - Set default VULKAN_SDK location if environment variable not defined on OSX
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT DEFINED ENV{VULKAN_SDK})
        if(NOT USE_MoltenVK)
            # SRS - Vulkan SDK installer copies standard vulkan headers and libs to /usr/local on OSX
            set(ENV{VULKAN_SDK} /usr/local)
        else()
            message(FATAL_ERROR "Must define VULKAN_SDK location if USE_MoltenVK option enabled!")
        endif()
    endif()

	if(SPIRV_SHADERC)
		add_definitions(-DSPIRV_SHADERC)

		# SRS - Add case for UNIX/OSX
		if(CMAKE_CL_64 OR UNIX)
			link_directories($ENV{VULKAN_SDK}/Lib)
		else()
			link_directories($ENV{VULKAN_SDK}/Lib32)
		endif()

	else()
		# override Glslang build options
		
		# options required by November 2018 release
		SET_OPTION(ENABLE_SPVREMAPPER ON)
		SET_OPTION(ENABLE_GLSLANG_BINARIES OFF)
		SET_OPTION(ENABLE_HLSL OFF)
		SET_OPTION(ENABLE_OPT ON)
		SET_OPTION(SPIRV_SKIP_EXECUTABLES ON)
		
		# options required by July 2020 release
		SET_OPTION(ENABLE_PCH OFF)
		SET_OPTION(ENABLE_CTEST OFF)
		SET_OPTION(BUILD_EXTERNAL OFF)
		SET_OPTION(SKIP_GLSLANG_INSTALL ON)

		#add_definitions(-DGLSLANG_MINOR_VERSION=13)
	
		set(GLSLANG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/glslang)
		add_subdirectory(${GLSLANG_DIR})
	
		include_directories(${GLSLANG_DIR})
		
		#link_directories($ENV{VULKAN_SDK}/Lib)
	endif()

	# Use FindVulkan module added with CMAKE 3.7
	if(NOT CMAKE_VERSION VERSION_LESS 3.7.0)
		message( STATUS "Using module to find Vulkan" )
		find_package(Vulkan)
	endif()

	if(NOT Vulkan_FOUND)
		message(FATAL_ERROR "Could not find Vulkan library!")
	else()
        add_definitions(-DUSE_VULKAN)
        include_directories($ENV{VULKAN_SDK}/include)

		if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            # SRS - Enable Beta extensions for VULKAN_SDK portability subset features on OSX
            add_definitions(-DVK_ENABLE_BETA_EXTENSIONS)
            # SRS - Optionally use MoltenVK headers/library for runtime config functions on OSX
            if(USE_MoltenVK)
                add_definitions(-DUSE_MoltenVK)
                include_directories($ENV{VULKAN_SDK}/../MoltenVK/include)
                set(Vulkan_LIBRARY $ENV{VULKAN_SDK}/../MoltenVK/dylib/macOS/libMoltenVK.dylib CACHE FILEPATH "Path to MoltenVK library." FORCE)
            endif()
		endif()
		message(STATUS "Using Vulkan: " ${Vulkan_LIBRARY})
	endif()
	
	# Eric: For use with SDL2/Vulkan
	if(UNIX)
        # SRS - For UNIX-like target OSs use the generic SDL Vulkan platform
        add_definitions(-DVULKAN_USE_PLATFORM_SDL)
	elseif(WIN32)
		add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
	endif()

else()

	if(USE_SYSTEM_LIBGLEW)
		find_package(GLEW)
	endif()

	if(GLEW_FOUND)
		include_directories(${GLEW_INCLUDE_DIRS})
		set(GLEW_LIBRARY ${GLEW_LIBRARIES})
	else()
		include_directories("libs/glew/include")
		set(GLEW_LIBRARY "" )
		add_definitions(-DGLEW_STATIC)
		
		set(GLEW_INCLUDES libs/glew/include/GL/glew.h)
		set(GLEW_SOURCES libs/glew/src/glew.c)

		if(WIN32)
			set(GLEW_INCLUDES ${GLEW_INCLUDES} libs/glew/include/GL/wglew.h)
		else()
			set(GLEW_INCLUDES ${GLEW_INCLUDES} libs/glew/include/GL/glxew.h)
		endif()
		
	endif()
endif()

if(USE_SYSTEM_RAPIDJSON)
find_package(rapidjson REQUIRED)
endif(USE_SYSTEM_RAPIDJSON)

if (RAPIDJSON_FOUND)
	include_directories("${RAPIDJSON_INCLUDE_DIRS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RAPIDJSON_CXX_FLAGS}")
else (RAPIDJSON_FOUND)
	include_directories("libs/rapidjson/include")
endif (RAPIDJSON_FOUND)


	
add_subdirectory(idlib)

file(GLOB AAS_INCLUDES aas/*.h)
file(GLOB AAS_SOURCES aas/*.cpp)

file(GLOB_RECURSE CM_INCLUDES cm/*.h)
file(GLOB_RECURSE CM_SOURCES cm/*.cpp)

file(GLOB FRAMEWORK_INCLUDES framework/*.h)
file(GLOB FRAMEWORK_SOURCES framework/*.cpp)

file(GLOB FRAMEWORK_IMGUI_INCLUDES imgui/*.h)
file(GLOB FRAMEWORK_IMGUI_SOURCES imgui/*.cpp)

file(GLOB RENDERER_INCLUDES renderer/*.h)
file(GLOB RENDERER_SOURCES renderer/*.cpp)
file(GLOB RENDERER_COLOR_INCLUDES renderer/Color/*.h)
file(GLOB RENDERER_COLOR_SOURCES renderer/Color/*.cpp)
file(GLOB RENDERER_DXT_INCLUDES renderer/DXT/*.h)
file(GLOB RENDERER_DXT_SOURCES renderer/DXT/*.cpp)
file(GLOB RENDERER_JOBS_INCLUDES renderer/jobs/*.h)
file(GLOB RENDERER_JOBS_SOURCES renderer/jobs/*.cpp)
file(GLOB RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES renderer/jobs/dynamicshadowvolume/*.h)
file(GLOB RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES renderer/jobs/dynamicshadowvolume/*.cpp)
file(GLOB RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES renderer/jobs/prelightshadowvolume/*.h)
file(GLOB RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES renderer/jobs/prelightshadowvolume/*.cpp)
file(GLOB RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES renderer/jobs/staticshadowvolume/*.h)
file(GLOB RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES renderer/jobs/staticshadowvolume/*.cpp)

file(GLOB RENDERER_SMAA_INCLUDES renderer/SMAA/*.h)

file(GLOB RENDERER_OPENGL_INCLUDES renderer/OpenGL/*.h)
file(GLOB RENDERER_OPENGL_SOURCES renderer/OpenGL/*.cpp)

file(GLOB RENDERER_VULKAN_INCLUDES renderer/Vulkan/*.h)
file(GLOB RENDERER_VULKAN_SOURCES renderer/Vulkan/*.cpp)

file(GLOB SHADERS ../base/renderprogs/*.hlsl ../base/renderprogs/*.inc )
file(GLOB SHADERS_BUILTIN ../base/renderprogs/builtin/*.hlsl )
file(GLOB SHADERS_BUILTIN_DEBUG ../base/renderprogs/builtin/debug/*.hlsl )
file(GLOB SHADERS_BUILTIN_FOG ../base/renderprogs/builtin/fog/*.hlsl )
file(GLOB SHADERS_BUILTIN_LEGACY ../base/renderprogs/builtin/legacy/*.hlsl )
file(GLOB SHADERS_BUILTIN_LIGHTING ../base/renderprogs/builtin/lighting/*.hlsl )
file(GLOB SHADERS_BUILTIN_POST ../base/renderprogs/builtin/post/*.hlsl )
file(GLOB SHADERS_BUILTIN_SSAO ../base/renderprogs/builtin/SSAO/*.hlsl )
file(GLOB SHADERS_BUILTIN_SSGI ../base/renderprogs/builtin/SSGI/*.hlsl )
file(GLOB SHADERS_BUILTIN_VIDEO ../base/renderprogs/builtin/video/*.hlsl )
file(GLOB SHADERS_BUILTIN_VR ../base/renderprogs/builtin/VR/*.hlsl )

file(GLOB IRRXML_INCLUDES libs/irrxml/src/*.h)
file(GLOB IRRXML_SOURCES libs/irrxml/src/*.cpp)

file(GLOB IMGUI_INCLUDES libs/imgui/*.h)
file(GLOB IMGUI_SOURCES libs/imgui/*.cpp)

file(GLOB STB_INCLUDES libs/stb/*.h)

file(GLOB MESA_INCLUDES libs/mesa/*.h)

file(GLOB TINYEXR_INCLUDES libs/tinyexr/*.h)

file(GLOB MIKKTSPACE_INCLUDES libs/mikktspace/*.h)
file(GLOB MIKKTSPACE_SOURCES libs/mikktspace/*.c)

if (NOT JPEG_FOUND)
    set(JPEG_INCLUDES
        libs/jpeg-6/jchuff.h
        libs/jpeg-6/jconfig.h
        libs/jpeg-6/jdct.h
        libs/jpeg-6/jdhuff.h
        libs/jpeg-6/jerror.h
        libs/jpeg-6/jinclude.h
        libs/jpeg-6/jmemsys.h
        libs/jpeg-6/jmorecfg.h
        libs/jpeg-6/jpegint.h
        libs/jpeg-6/jpeglib.h
        libs/jpeg-6/jversion.h)

    set(JPEG_SOURCES
        libs/jpeg-6/jcapimin.cpp
        libs/jpeg-6/jcapistd.cpp
        libs/jpeg-6/jccoefct.cpp
        libs/jpeg-6/jccolor.cpp
        libs/jpeg-6/jcdctmgr.cpp
        libs/jpeg-6/jchuff.cpp
        libs/jpeg-6/jcinit.cpp
        libs/jpeg-6/jcmainct.cpp
        libs/jpeg-6/jcmarker.cpp
        libs/jpeg-6/jcmaster.cpp
        libs/jpeg-6/jcomapi.cpp
        libs/jpeg-6/jcparam.cpp
        libs/jpeg-6/jcphuff.cpp
        libs/jpeg-6/jcprepct.cpp
        libs/jpeg-6/jcsample.cpp
        libs/jpeg-6/jctrans.cpp
        libs/jpeg-6/jdapimin.cpp
        libs/jpeg-6/jdapistd.cpp
        libs/jpeg-6/jdatadst.cpp
        libs/jpeg-6/jdatasrc.cpp
        libs/jpeg-6/jdcoefct.cpp
        libs/jpeg-6/jdcolor.cpp
        libs/jpeg-6/jddctmgr.cpp
        libs/jpeg-6/jdhuff.cpp
        libs/jpeg-6/jdinput.cpp
        libs/jpeg-6/jdmainct.cpp
        libs/jpeg-6/jdmarker.cpp
        libs/jpeg-6/jdmaster.cpp
        libs/jpeg-6/jdmerge.cpp
        libs/jpeg-6/jdphuff.cpp
        libs/jpeg-6/jdpostct.cpp
        libs/jpeg-6/jdsample.cpp
        libs/jpeg-6/jdtrans.cpp
        libs/jpeg-6/jerror.cpp
        libs/jpeg-6/jfdctflt.cpp
        libs/jpeg-6/jfdctfst.cpp
        libs/jpeg-6/jfdctint.cpp
        libs/jpeg-6/jidctflt.cpp
        libs/jpeg-6/jidctfst.cpp
        libs/jpeg-6/jidctint.cpp
        libs/jpeg-6/jidctred.cpp
        #libs/jpeg-6/jload.cpp
        libs/jpeg-6/jmemmgr.cpp
        libs/jpeg-6/jmemnobs.cpp
        #libs/jpeg-6/jpegtran.cpp
        libs/jpeg-6/jquant1.cpp
        libs/jpeg-6/jquant2.cpp
        libs/jpeg-6/jutils.cpp)
else (NOT JPEG_FOUND)
   set(JPEG_INCLUDES "")
   set(JPEG_SOURCES "")
endif (NOT JPEG_FOUND)

if (NOT PNG_FOUND)
    file(GLOB PNG_INCLUDES libs/png/*.h)
    file(GLOB PNG_SOURCES libs/png/*.c)
else (NOT PNG_FOUND)
    set(PNG_INCLUDES "")
    set(PNG_SOURCES "")
endif (NOT PNG_FOUND)

if (NOT ZLIB_FOUND)
file(GLOB ZLIB_INCLUDES libs/zlib/*.h)
file(GLOB ZLIB_SOURCES libs/zlib/*.c)
else (NOT ZLIB_FOUND)
  set(ZLIB_INCLUDES "")
  set(ZLIB_SOURCES "")
endif (NOT ZLIB_FOUND)

if(BINKDEC)
	file(GLOB BINKDEC_INCLUDES libs/libbinkdec/include/*.h)
	file(GLOB BINKDEC_SOURCES libs/libbinkdec/src/*.c libs/libbinkdec/src/*.cpp)
	add_definitions(-DUSE_BINKDEC)
	include_directories("libs/libbinkdec/include")
endif()

file(GLOB MINIZIP_INCLUDES libs/zlib/minizip/*.h)
file(GLOB MINIZIP_SOURCES libs/zlib/minizip/*.c libs/zlib/minizip/*.cpp)

set(FREETYPE_SOURCES
	libs/freetype/src/autofit/autofit.c
	libs/freetype/src/bdf/bdf.c
	libs/freetype/src/cff/cff.c
	libs/freetype/src/base/ftbase.c
	libs/freetype/src/base/ftbitmap.c
	libs/freetype/src/cache/ftcache.c
	libs/freetype/src/base/ftdebug.c
	libs/freetype/src/base/ftgasp.c
	libs/freetype/src/base/ftglyph.c
	libs/freetype/src/gzip/ftgzip.c
	libs/freetype/src/base/ftinit.c
	libs/freetype/src/lzw/ftlzw.c
	libs/freetype/src/base/ftstroke.c
	libs/freetype/src/base/ftsystem.c
	libs/freetype/src/smooth/smooth.c
	libs/freetype/src/base/ftbbox.c
	libs/freetype/src/base/ftmm.c
	libs/freetype/src/base/ftpfr.c
	libs/freetype/src/base/ftsynth.c
	libs/freetype/src/base/fttype1.c
	libs/freetype/src/base/ftwinfnt.c
	libs/freetype/src/pcf/pcf.c
	libs/freetype/src/pfr/pfr.c
	libs/freetype/src/psaux/psaux.c
	libs/freetype/src/pshinter/pshinter.c
	libs/freetype/src/psnames/psmodule.c
	libs/freetype/src/raster/raster.c
	libs/freetype/src/sfnt/sfnt.c
	libs/freetype/src/truetype/truetype.c
	libs/freetype/src/type1/type1.c
	libs/freetype/src/cid/type1cid.c
	libs/freetype/src/type42/type42.c
	libs/freetype/src/winfonts/winfnt.c)
	
	
set(SOUND_INCLUDES
	sound/snd_local.h
	sound/sound.h
	sound/SoundVoice.h
	sound/WaveFile.h
	sound/CinematicAudio.h)
	
set(SOUND_SOURCES
	sound/snd_emitter.cpp
	sound/snd_shader.cpp
	sound/snd_system.cpp
	sound/snd_world.cpp
	sound/SoundVoice.cpp
	sound/WaveFile.cpp)
	
set(XAUDIO2_INCLUDES
	sound/XAudio2/XA2_SoundHardware.h
	sound/XAudio2/XA2_SoundSample.h
	sound/XAudio2/XA2_SoundVoice.h
	sound/Xaudio2/XA2_CinematicAudio.h)
	
set(XAUDIO2_SOURCES
	sound/XAudio2/XA2_SoundHardware.cpp
	sound/XAudio2/XA2_SoundSample.cpp
	sound/XAudio2/XA2_SoundVoice.cpp
	sound/Xaudio2/XA2_CinematicAudio.cpp)
	
set(OPENAL_INCLUDES
	sound/OpenAL/AL_SoundHardware.h
	sound/OpenAL/AL_SoundSample.h
	sound/OpenAL/AL_SoundVoice.h
	sound/OpenAL/AL_CinematicAudio.h)
	
set(OPENAL_SOURCES
	sound/OpenAL/AL_SoundHardware.cpp
	sound/OpenAL/AL_SoundSample.cpp
	sound/OpenAL/AL_SoundVoice.cpp
	sound/OpenAL/AL_CinematicAudio.cpp)

set(STUBAUDIO_INCLUDES
	sound/stub/SoundStub.h)

set(STUBAUDIO_SOURCES
	sound/stub/SoundSample.cpp)
	
set(OGGVORBIS_INCLUDES
	libs/oggvorbis/ogg/ogg.h
	libs/oggvorbis/ogg/os_types.h)
	
set(OGGVORBIS_SOURCES
	libs/oggvorbis/oggsrc/bitwise.c
	libs/oggvorbis/oggsrc/framing.c
	
	libs/oggvorbis/vorbissrc/mdct.c
	libs/oggvorbis/vorbissrc/smallft.c
	libs/oggvorbis/vorbissrc/block.c
	libs/oggvorbis/vorbissrc/envelope.c
	libs/oggvorbis/vorbissrc/windowvb.c
	libs/oggvorbis/vorbissrc/lsp.c
	libs/oggvorbis/vorbissrc/lpc.c
	libs/oggvorbis/vorbissrc/analysis.c
	libs/oggvorbis/vorbissrc/synthesis.c
	libs/oggvorbis/vorbissrc/psy.c
	libs/oggvorbis/vorbissrc/info.c
	libs/oggvorbis/vorbissrc/floor1.c
	libs/oggvorbis/vorbissrc/floor0.c
	libs/oggvorbis/vorbissrc/res0.c
	libs/oggvorbis/vorbissrc/mapping0.c
	libs/oggvorbis/vorbissrc/registry.c
	libs/oggvorbis/vorbissrc/codebook.c
	libs/oggvorbis/vorbissrc/sharedbook.c
	libs/oggvorbis/vorbissrc/lookup.c
	libs/oggvorbis/vorbissrc/bitrate.c
	libs/oggvorbis/vorbissrc/vorbisfile.c)

file(GLOB RAPIDJSON_INCLUDES libs/rapidjson/include/rapidjson/*.h)
	
file(GLOB SYS_INCLUDES sys/*.h)
file(GLOB SYS_SOURCES sys/*.cpp)

file(GLOB UI_INCLUDES ui/*.h)
file(GLOB UI_SOURCES ui/*.cpp)

file(GLOB SWF_INCLUDES swf/*.h)
file(GLOB SWF_SOURCES swf/*.cpp)

set(GAMED3XP_INCLUDES
	d3xp/Achievements.h
	d3xp/Actor.h
	d3xp/AF.h
	d3xp/AFEntity.h
	d3xp/AimAssist.h
	d3xp/BrittleFracture.h
	d3xp/Camera.h
	#d3xp/EndLevel.h
	d3xp/Entity.h
	d3xp/EnvironmentProbe.h
	d3xp/Fx.h
	d3xp/Game.h
	d3xp/GameEdit.h
	d3xp/Game_local.h
	d3xp/Grabber.h
	d3xp/IK.h
	d3xp/Item.h
	d3xp/Leaderboards.h
	d3xp/Light.h
	d3xp/Misc.h
	d3xp/Moveable.h
	d3xp/Mover.h
	d3xp/MultiplayerGame.h
	d3xp/Player.h
	d3xp/PlayerIcon.h
	d3xp/PlayerView.h
	d3xp/PredictedValue.h
	d3xp/Projectile.h
	d3xp/Pvs.h
	d3xp/SecurityCamera.h
	d3xp/SmokeParticles.h
	d3xp/Sound.h
	d3xp/Target.h
	d3xp/Trigger.h
	d3xp/Weapon.h
	d3xp/WorldSpawn.h)
	
set(GAMED3XP_SOURCES
	d3xp/Achievements.cpp
	d3xp/Actor.cpp
	d3xp/AF.cpp
	d3xp/AFEntity.cpp
	d3xp/AimAssist.cpp
	d3xp/BrittleFracture.cpp
	d3xp/Camera.cpp
	#d3xp/EndLevel.cpp
	d3xp/Entity.cpp
	d3xp/EnvironmentProbe.cpp
	d3xp/Fx.cpp
	d3xp/GameEdit.cpp
	d3xp/Game_local.cpp
	d3xp/Game_network.cpp
	d3xp/Grabber.cpp
	d3xp/IK.cpp
	d3xp/Item.cpp
	d3xp/Leaderboards.cpp
	d3xp/Light.cpp
	d3xp/Misc.cpp
	d3xp/Moveable.cpp
	d3xp/Mover.cpp
	d3xp/MultiplayerGame.cpp
	d3xp/Player.cpp
	d3xp/PlayerIcon.cpp
	d3xp/PlayerView.cpp
	d3xp/precompiled.cpp
	d3xp/Projectile.cpp
	d3xp/Pvs.cpp
	d3xp/SecurityCamera.cpp
	d3xp/SmokeParticles.cpp
	d3xp/Sound.cpp
	d3xp/Target.cpp
	d3xp/Trigger.cpp
	d3xp/Weapon.cpp
	d3xp/WorldSpawn.cpp)

file(GLOB GAMED3XP_AI_INCLUDES d3xp/ai/*.h)
file(GLOB GAMED3XP_AI_SOURCES d3xp/ai/*.cpp)

file(GLOB GAMED3XP_ANIM_INCLUDES d3xp/anim/*.h)
file(GLOB GAMED3XP_ANIM_SOURCES d3xp/anim/*.cpp)

file(GLOB GAMED3XP_GAMESYS_INCLUDES d3xp/gamesys/*.h)
set(GAMED3XP_GAMESYS_SOURCES
	#d3xp/gamesys/Callbacks.cpp
	d3xp/gamesys/Class.cpp
	#d3xp/gamesys/DebugGraph.cpp
	d3xp/gamesys/Event.cpp
	d3xp/gamesys/SaveGame.cpp
	d3xp/gamesys/SysCmds.cpp
	d3xp/gamesys/SysCvar.cpp
	#d3xp/gamesys/TypeInfo.cpp
	)

file(GLOB GAMED3XP_MENUS_INCLUDES d3xp/menus/*.h)
file(GLOB GAMED3XP_MENUS_SOURCES d3xp/menus/*.cpp)
	
file(GLOB GAMED3XP_PHYSICS_INCLUDES d3xp/physics/*.h)
file(GLOB GAMED3XP_PHYSICS_SOURCES d3xp/physics/*.cpp)

file(GLOB GAMED3XP_SCRIPT_INCLUDES d3xp/script/*.h)
file(GLOB GAMED3XP_SCRIPT_SOURCES d3xp/script/*.cpp)

file(GLOB COMPILER_INCLUDES tools/compilers/*.h)

file(GLOB COMPILER_AAS_INCLUDES tools/compilers/aas/*.h)
file(GLOB COMPILER_AAS_SOURCES tools/compilers/aas/*.cpp)

file(GLOB COMPILER_DMAP_INCLUDES tools/compilers/dmap/*.h)
file(GLOB COMPILER_DMAP_SOURCES tools/compilers/dmap/*.cpp)

file(GLOB COMPILER_ROQVQ_INCLUDES tools/compilers/roqvq/*.h)
file(GLOB COMPILER_ROQVQ_SOURCES tools/compilers/roqvq/*.cpp)

set(TOOLS_INCLUDES tools/edit_public.h)

file(GLOB TOOLS_COMAFX_INCLUDES tools/comafx/*.h)
file(GLOB TOOLS_COMAFX_SOURCES tools/comafx/*.cpp)

set(TOOLS_COMMON_INCLUDES
	tools/common/ColorButton.h
	tools/common/DialogHelpers.h
	tools/common/MaskEdit.h
	tools/common/OpenFileDialog.h
	tools/common/PropertyGrid.h
	tools/common/RegistryOptions.h
	#tools/common/RenderBumpFlatDialog.h
	tools/common/RollupPanel.h
	tools/common/SpinButton.h)

set(TOOLS_COMMON_SOURCES
	tools/common/AlphaPopup.cpp
	tools/common/ColorButton.cpp
	tools/common/MaskEdit.cpp
	tools/common/OpenFileDialog.cpp
	tools/common/PropertyGrid.cpp
	tools/common/RegistryOptions.cpp
	#tools/common/RenderBumpFlatDialog.cpp
	tools/common/RollupPanel.cpp
	tools/common/SpinButton.cpp)

file(GLOB TOOLS_COMMON_PROPTREE_INCLUDES tools/common/PropTree/*.h)
file(GLOB TOOLS_COMMON_PROPTREE_SOURCES tools/common/PropTree/*.cpp)

file(GLOB EDITOR_AF_INCLUDES tools/af/*.h)
file(GLOB EDITOR_AF_SOURCES tools/af/*.cpp)

set(EDITOR_DECL_INCLUDES
	tools/decl/DialogDeclBrowser.h
	tools/decl/DialogDeclEditor.h
	tools/decl/DialogDeclNew.h)
	
set(EDITOR_DECL_SOURCES
	tools/decl/DialogDeclBrowser.cpp
	tools/decl/DialogDeclEditor.cpp
	tools/decl/DialogDeclNew.cpp)
	
file(GLOB EDITOR_DEBUGGER_INCLUDES tools/debugger/*.h)
file(GLOB EDITOR_DEBUGGER_SOURCES tools/debugger/*.cpp)

file(GLOB EDITOR_GUIED_INCLUDES tools/guied/*.h)
set(EDITOR_GUIED_SOURCES
	tools/guied/GEApp.cpp
	tools/guied/GECheckInDlg.cpp
	tools/guied/GEDeleteModifier.cpp
	tools/guied/GEHideModifier.cpp
	tools/guied/GEInsertModifier.cpp
	tools/guied/GEItemPropsDlg.cpp
	tools/guied/GEItemScriptsDlg.cpp
	tools/guied/GEKeyValueModifier.cpp
	tools/guied/GEModifier.cpp
	tools/guied/GEModifierGroup.cpp
	tools/guied/GEModifierStack.cpp
	tools/guied/GEMoveModifier.cpp
	tools/guied/GENavigator.cpp
	tools/guied/GEOptions.cpp
	tools/guied/GEOptionsDlg.cpp
	tools/guied/GEProperties.cpp
	tools/guied/GEPropertyPage.cpp
	tools/guied/GESelectionMgr.cpp
	tools/guied/GESizeModifier.cpp
	tools/guied/GEStateModifier.cpp
	tools/guied/GEStatusBar.cpp
	tools/guied/GETransformer.cpp
	tools/guied/GEViewer.cpp
	tools/guied/GEWindowWrapper.cpp
	#tools/guied/GEWindowWrapper_stub.cpp
	tools/guied/GEWorkspace.cpp
	tools/guied/GEWorkspaceFile.cpp
	tools/guied/GEZOrderModifier.cpp
	tools/guied/guied.cpp)

file(GLOB EDITOR_MATERIALEDITOR_INCLUDES tools/materialeditor/*.h)
file(GLOB EDITOR_MATERIALEDITOR_SOURCES tools/materialeditor/*.cpp)

file(GLOB EDITOR_PARTICLE_INCLUDES tools/particle/*.h)
file(GLOB EDITOR_PARTICLE_SOURCES tools/particle/*.cpp)

file(GLOB EDITOR_PDA_INCLUDES tools/pda/*.h)
file(GLOB EDITOR_PDA_SOURCES tools/pda/*.cpp)

file(GLOB EDITOR_RADIANT_INCLUDES tools/radiant/*.h)
file(GLOB EDITOR_RADIANT_SOURCES tools/radiant/*.cpp)

file(GLOB EDITOR_SCRIPT_INCLUDES tools/script/*.h)
file(GLOB EDITOR_SCRIPT_SOURCES tools/script/*.cpp)

file(GLOB EDITOR_SOUND_INCLUDES tools/sound/*.h)
file(GLOB EDITOR_SOUND_SOURCES tools/sound/*.cpp)

file(GLOB IMGUI_EDITOR_INCLUDES tools/imgui/*.h)
file(GLOB IMGUI_EDITOR_SOURCES tools/imgui/*.cpp)

file(GLOB IMGUI_EDITOR_LIGHT_INCLUDES tools/imgui/lighteditor/*.h)
file(GLOB IMGUI_EDITOR_LIGHT_SOURCES tools/imgui/lighteditor/*.cpp)

file(GLOB DOOMCLASSIC_INCLUDES ../doomclassic/doom/*.h)
file(GLOB DOOMCLASSIC_SOURCES ../doomclassic/doom/*.cpp)

set(DOOMCLASSIC_STUBAUDIO_SOURCES ../doomclassic/doom/i_sound_stub.cpp)
set(DOOMCLASSIC_OPENAL_SOURCES ../doomclassic/doom/i_sound_openal.cpp)
set(DOOMCLASSIC_XAUDIO2_SOURCES ../doomclassic/doom/i_sound_win32.cpp)

list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/doom/i_sound_stub.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/doom/i_sound_openal.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/doom/i_sound_win32.cpp)

file(GLOB TIMIDITY_INCLUDES libs/timidity/*.h)
file(GLOB TIMIDITY_SOURCES libs/timidity/*.cpp)

file(GLOB_RECURSE WIN32_INCLUDES sys/win32/*.h)
#file(GLOB_RECURSE WIN32_SOURCES sys/win32/*.cpp)

set(WIN32_SOURCES
	sys/win32/win_glimp.cpp
	sys/win32/win_input.cpp
	sys/win32/win_main.cpp
	sys/win32/win_shared.cpp
	sys/win32/win_snd.cpp
	sys/win32/win_syscon.cpp
	sys/win32/win_taskkeyhook.cpp
	sys/win32/win_wndproc.cpp)
	
if(MSVC)
	list(APPEND WIN32_SOURCES sys/win32/win_cpu.cpp)
endif()

set(WIN32_RESOURCES
	# sys/win32/rc/res/BEVEL.BMP
	# sys/win32/rc/res/BITMAP2.BMP
	# sys/win32/rc/res/BMP00001.BMP
	# sys/win32/rc/res/BMP0002.BMP
	# sys/win32/rc/res/DEFTEX.WAL
	# sys/win32/rc/res/ENDCAP.BMP
	# sys/win32/rc/res/GetString.htm
	# sys/win32/rc/res/IBEVEL.BMP
	# sys/win32/rc/res/IENDCAP.BMP
	# sys/win32/rc/res/MEFileToolbar.bmp
	# sys/win32/rc/res/MEtoolbar.bmp
	# sys/win32/rc/res/MaterialEditor.ico
	# sys/win32/rc/res/PropTree.rc2
	# sys/win32/rc/res/Q.BMP
	# sys/win32/rc/res/RADIANT3.GIF
	# sys/win32/rc/res/Radiant.ico
	# sys/win32/rc/res/RadiantDoc.ico
	# sys/win32/rc/res/TOOLBAR1.BMP
	# sys/win32/rc/res/TOOLBAR2.BMP
	# sys/win32/rc/res/Toolbar.bmp
	# sys/win32/rc/res/VIEWDEFA.BMP
	# sys/win32/rc/res/VIEWOPPO.BMP
	# sys/win32/rc/res/bmp00002.bmp
	# sys/win32/rc/res/bmp00003.bmp
	# sys/win32/rc/res/bmp00004.bmp
	# sys/win32/rc/res/bmp00005.bmp
	# sys/win32/rc/res/cchsb.bmp
	# sys/win32/rc/res/ccrgb.bmp
	# sys/win32/rc/res/dbg_back.bmp
	# sys/win32/rc/res/dbg_breakpoint.ico
	# sys/win32/rc/res/dbg_current.ico
	# sys/win32/rc/res/dbg_currentline.ico
	# sys/win32/rc/res/dbg_empty.ico
	# sys/win32/rc/res/dbg_open.bmp
	# sys/win32/rc/res/dbg_toolbar.bmp
	sys/win32/rc/res/doom.ico
	# sys/win32/rc/res/fpoint.cur
	# sys/win32/rc/res/fxed_link.ico
	# sys/win32/rc/res/fxed_toolbar.bmp
	# sys/win32/rc/res/fxeditor.ico
	# sys/win32/rc/res/guied.ico
	# sys/win32/rc/res/guied_collapse.ico
	# sys/win32/rc/res/guied_expand.ico
	# sys/win32/rc/res/guied_hand.cur
	# sys/win32/rc/res/guied_nav_visible.ico
	# sys/win32/rc/res/guied_nav_visibledisabled.ico
	# sys/win32/rc/res/guied_scripts.ico
	# sys/win32/rc/res/guied_scripts_white.ico
	# sys/win32/rc/res/guied_viewer_toolbar.bmp
	# sys/win32/rc/res/icon2.ico
	# sys/win32/rc/res/logo_sm3dfx.bmp
	# sys/win32/rc/res/matedtree.bmp
	# sys/win32/rc/res/me_disabled_icon.ico
	# sys/win32/rc/res/me_enabled.ico
	# sys/win32/rc/res/me_off_icon.ico
	# sys/win32/rc/res/me_on_icon.ico
	# sys/win32/rc/res/qe3.ico
	# sys/win32/rc/res/shaderbar.bmp
	# sys/win32/rc/res/shaderdoc.ico
	# sys/win32/rc/res/shaderframe.ico
	# sys/win32/rc/res/spliter.cur
	)



#if(USE_MFC_TOOLS)
	list(APPEND WIN32_RESOURCES sys/win32/rc/doom.rc)
#else()
#	list(APPEND WIN32_RESOURCES sys/win32/rc/doom_nomfc.rc)
#endif()


file(GLOB POSIX_INCLUDES sys/posix/*.h)
file(GLOB POSIX_SOURCES sys/posix/*.cpp)

file(GLOB COMMON_INCLUDES sys/common/*.h)
file(GLOB COMMON_SOURCES sys/common/*.cpp)

file(GLOB SDL_INCLUDES sys/sdl/*.h)
file(GLOB SDL_SOURCES sys/sdl/*.cpp)

# Eric: Utilize either the Vulkan or GL implementation of SDL
if(UNIX)
	if(USE_VULKAN)
		get_filename_component(sdl_glimp_cpp_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/sdl/sdl_glimp.cpp ABSOLUTE)
		list(REMOVE_ITEM SDL_SOURCES "${sdl_glimp_cpp_full_path}")
	else()
		get_filename_component(sdl_vkimp_cpp_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/sdl/sdl_vkimp.cpp ABSOLUTE)
		list(REMOVE_ITEM SDL_SOURCES "${sdl_vkimp_cpp_full_path}")
	endif()
endif()

source_group("aas" FILES ${AAS_INCLUDES})
source_group("aas" FILES ${AAS_SOURCES})

source_group("cm" FILES ${CM_INCLUDES})
source_group("cm" FILES ${CM_SOURCES})

source_group("framework" FILES ${FRAMEWORK_INCLUDES})
source_group("framework" FILES ${FRAMEWORK_SOURCES})

source_group("imgui" FILES ${FRAMEWORK_IMGUI_INCLUDES})
source_group("imgui" FILES ${FRAMEWORK_IMGUI_SOURCES})

source_group("renderer" FILES ${RENDERER_INCLUDES})
source_group("renderer" FILES ${RENDERER_SOURCES})

source_group("renderer\\Color" FILES ${RENDERER_COLOR_INCLUDES})
source_group("renderer\\Color" FILES ${RENDERER_COLOR_SOURCES})

source_group("renderer\\DXT" FILES ${RENDERER_DXT_INCLUDES})
source_group("renderer\\DXT" FILES ${RENDERER_DXT_SOURCES})

source_group("renderer\\jobs" FILES ${RENDERER_JOBS_INCLUDES})
source_group("renderer\\jobs" FILES ${RENDERER_JOBS_SOURCES})

source_group("renderer\\jobs\\dynamicshadowvolume" FILES ${RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\dynamicshadowvolume" FILES ${RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES})

source_group("renderer\\jobs\\prelightshadowvolume" FILES ${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\prelightshadowvolume" FILES ${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES})

source_group("renderer\\jobs\\staticshadowvolume" FILES ${RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\staticshadowvolume" FILES ${RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES})

source_group("renderer\\OpenGL" FILES ${RENDERER_OPENGL_INCLUDES})
source_group("renderer\\OpenGL" FILES ${RENDERER_OPENGL_SOURCES})

source_group("renderer\\Vulkan" FILES ${RENDERER_VULKAN_INCLUDES})
source_group("renderer\\Vulkan" FILES ${RENDERER_VULKAN_SOURCES})

source_group("renderer\\SMAA" FILES ${RENDERER_SMAA_INCLUDES})

source_group("renderer\\Shaders" FILES ${SHADERS})
source_group("renderer\\Shaders\\builtin" FILES ${SHADERS_BUILTIN})
source_group("renderer\\Shaders\\builtin\\debug" FILES ${SHADERS_BUILTIN_DEBUG})
source_group("renderer\\Shaders\\builtin\\fog" FILES ${SHADERS_BUILTIN_FOG})
source_group("renderer\\Shaders\\builtin\\legacy" FILES ${SHADERS_BUILTIN_LEGACY})
source_group("renderer\\Shaders\\builtin\\lighting" FILES ${SHADERS_BUILTIN_LIGHTING})
source_group("renderer\\Shaders\\builtin\\post" FILES ${SHADERS_BUILTIN_POST})
source_group("renderer\\Shaders\\builtin\\SSAO" FILES ${SHADERS_BUILTIN_SSAO})
source_group("renderer\\Shaders\\builtin\\SSGI" FILES ${SHADERS_BUILTIN_SSGI})
source_group("renderer\\Shaders\\builtin\\video" FILES ${SHADERS_BUILTIN_VIDEO})
source_group("renderer\\Shaders\\builtin\\VR" FILES ${SHADERS_BUILTIN_VR})

source_group("libs\\irrxml" FILES ${IRRXML_INCLUDES})
source_group("libs\\irrxml" FILES ${IRRXML_SOURCES})

source_group("libs\\imgui" FILES ${IMGUI_INCLUDES})
source_group("libs\\imgui" FILES ${IMGUI_SOURCES})

source_group("libs\\stb" FILES ${STB_INCLUDES})

source_group("libs\\mesa" FILES ${MESA_INCLUDES})

source_group("libs\\tinyexr" FILES ${TINYEXR_INCLUDES})

source_group("libs\\mikktspace" FILES ${MIKKTSPACE_INCLUDES})
source_group("libs\\mikktspace" FILES ${MIKKTSPACE_SOURCES})

source_group("libs\\jpeg-6" FILES ${JPEG_INCLUDES})
source_group("libs\\jpeg-6" FILES ${JPEG_SOURCES})

source_group("libs\\libbinkdec" FILES ${BINKDEC_INCLUDES})
source_group("libs\\libbinkdec" FILES ${BINKDEC_SOURCES})

source_group("libs\\png" FILES ${PNG_INCLUDES})
source_group("libs\\png" FILES ${PNG_SOURCES})

source_group("libs\\zlib" FILES ${ZLIB_INCLUDES})
source_group("libs\\zlib" FILES ${ZLIB_SOURCES})

source_group("libs\\zlib\\minizip" FILES ${MINIZIP_INCLUDES})
source_group("libs\\zlib\\minizip" FILES ${MINIZIP_SOURCES})

source_group("libs\\glew" FILES ${GLEW_INCLUDES})
source_group("libs\\glew" FILES ${GLEW_SOURCES})

source_group("libs\\rapidjson" FILES ${RAPIDJSON_INCLUDES})

source_group("libs\\freetype" FILES ${FREETYPE_SOURCES})

source_group("sound" FILES ${SOUND_INCLUDES})
source_group("sound" FILES ${SOUND_SOURCES})

source_group("sound\\XAudio2" FILES ${XAUDIO2_INCLUDES})
source_group("sound\\XAudio2" FILES ${XAUDIO2_SOURCES})

source_group("sound\\OpenAL" FILES ${OPENAL_INCLUDES})
source_group("sound\\OpenAL" FILES ${OPENAL_SOURCES})

source_group("sound\\stub" FILES ${STUBAUDIO_INCLUDES})
source_group("sound\\stub" FILES ${STUBAUDIO_SOURCES})

source_group("libs\\oggvorbis" FILES ${OGGVORBIS_INCLUDES})
source_group("libs\\oggvorbis" FILES ${OGGVORBIS_SOURCES})

source_group("ui" FILES ${UI_INCLUDES})
source_group("ui" FILES ${UI_SOURCES})

source_group("swf" FILES ${SWF_INCLUDES})
source_group("swf" FILES ${SWF_SOURCES})

source_group("sys" FILES ${SYS_INCLUDES})
source_group("sys" FILES ${SYS_SOURCES})

source_group("doomclassic" FILES ${DOOMCLASSIC_INCLUDES})
source_group("doomclassic" FILES ${DOOMCLASSIC_SOURCES})

source_group("doomclassic" FILES ${DOOMCLASSIC_STUBAUDIO_SOURCES})
source_group("doomclassic" FILES ${DOOMCLASSIC_OPENAL_SOURCES})
source_group("doomclassic" FILES ${DOOMCLASSIC_XAUDIO2_SOURCES})

source_group("libs\\timidity" FILES ${TIMIDITY_INCLUDES})
source_group("libs\\timidity" FILES ${TIMIDITY_SOURCES})


source_group("game-d3xp" FILES ${GAMED3XP_INCLUDES})
source_group("game-d3xp" FILES ${GAMED3XP_SOURCES})

source_group("game-d3xp\\ai" FILES ${GAMED3XP_AI_INCLUDES})
source_group("game-d3xp\\ai" FILES ${GAMED3XP_AI_SOURCES})

source_group("game-d3xp\\anim" FILES ${GAMED3XP_ANIM_INCLUDES})
source_group("game-d3xp\\anim" FILES ${GAMED3XP_ANIM_SOURCES})

source_group("game-d3xp\\gamesys" FILES ${GAMED3XP_GAMESYS_INCLUDES})
source_group("game-d3xp\\gamesys" FILES ${GAMED3XP_GAMESYS_SOURCES})

source_group("game-d3xp\\menus" FILES ${GAMED3XP_MENUS_INCLUDES})
source_group("game-d3xp\\menus" FILES ${GAMED3XP_MENUS_SOURCES})

source_group("game-d3xp\\physics" FILES ${GAMED3XP_PHYSICS_INCLUDES})
source_group("game-d3xp\\physics" FILES ${GAMED3XP_PHYSICS_SOURCES})

source_group("game-d3xp\\script" FILES ${GAMED3XP_SCRIPT_INCLUDES})
source_group("game-d3xp\\script" FILES ${GAMED3XP_SCRIPT_SOURCES})




source_group("sys\\win32" FILES ${WIN32_INCLUDES})
source_group("sys\\win32" FILES ${WIN32_SOURCES})
source_group("sys\\win32\\Resources" FILES ${WIN32_RESOURCES})

source_group("sys\\posix" FILES ${POSIX_INCLUDES})
source_group("sys\\posix" FILES ${POSIX_SOURCES})

source_group("sys\\common" FILES ${COMMON_INCLUDES})
source_group("sys\\common" FILES ${COMMON_SOURCES})

source_group("sys\\sdl" FILES ${SDL_INCLUDES})
source_group("sys\\sdl" FILES ${SDL_SOURCES})


source_group("tools\\compilers" FILES ${COMPILER_INCLUDES})

source_group("tools\\compilers\\aas" FILES ${COMPILER_AAS_INCLUDES})
source_group("tools\\compilers\\aas" FILES ${COMPILER_AAS_SOURCES})

source_group("tools\\compilers\\dmap" FILES ${COMPILER_DMAP_INCLUDES})
source_group("tools\\compilers\\dmap" FILES ${COMPILER_DMAP_SOURCES})

source_group("tools\\compilers\\roqvq" FILES ${COMPILER_ROQVQ_INCLUDES})
source_group("tools\\compilers\\roqvq" FILES ${COMPILER_ROQVQ_SOURCES})

source_group("tools" FILES ${TOOLS_INCLUDES})

source_group("tools\\comafx" FILES ${TOOLS_COMAFX_INCLUDES})
source_group("tools\\comafx" FILES ${TOOLS_COMAFX_SOURCES})

source_group("tools\\common" FILES ${TOOLS_COMMON_INCLUDES})
source_group("tools\\common" FILES ${TOOLS_COMMON_SOURCES})

source_group("tools\\common\\proptree" FILES ${TOOLS_COMMON_PROPTREE_INCLUDES})
source_group("tools\\common\\proptree" FILES ${TOOLS_COMMON_PROPTREE_SOURCES})

source_group("tools\\af" FILES ${EDITOR_AF_INCLUDES})
source_group("tools\\af" FILES ${EDITOR_AF_SOURCES})

source_group("tools\\debugger" FILES ${EDITOR_DEBUGGER_INCLUDES})
source_group("tools\\debugger" FILES ${EDITOR_DEBUGGER_SOURCES})

source_group("tools\\decl" FILES ${EDITOR_DECL_INCLUDES})
source_group("tools\\decl" FILES ${EDITOR_DECL_SOURCES})

source_group("tools\\guied" FILES ${EDITOR_GUIED_INCLUDES})
source_group("tools\\guied" FILES ${EDITOR_GUIED_SOURCES})

source_group("tools\\materialeditor" FILES ${EDITOR_MATERIALEDITOR_INCLUDES})
source_group("tools\\materialeditor" FILES ${EDITOR_MATERIALEDITOR_SOURCES})

source_group("tools\\particle" FILES ${EDITOR_PARTICLE_INCLUDES})
source_group("tools\\particle" FILES ${EDITOR_PARTICLE_SOURCES})

source_group("tools\\pda" FILES ${EDITOR_PDA_INCLUDES})
source_group("tools\\pda" FILES ${EDITOR_PDA_SOURCES})

source_group("tools\\radiant" FILES ${EDITOR_RADIANT_INCLUDES})
source_group("tools\\radiant" FILES ${EDITOR_RADIANT_SOURCES})

source_group("tools\\script" FILES ${EDITOR_SCRIPT_INCLUDES})
source_group("tools\\script" FILES ${EDITOR_SCRIPT_SOURCES})

source_group("tools\\sound" FILES ${EDITOR_SOUND_INCLUDES})
source_group("tools\\sound" FILES ${EDITOR_SOUND_SOURCES})

source_group("tools\\imgui" FILES ${IMGUI_EDITOR_INCLUDES})
source_group("tools\\imgui" FILES ${IMGUI_EDITOR_SOURCES})

source_group("tools\\imgui\\lighteditor" FILES ${IMGUI_EDITOR_LIGHT_INCLUDES})
source_group("tools\\imgui\\lighteditor" FILES ${IMGUI_EDITOR_LIGHT_SOURCES})


set(RBDOOM3_INCLUDES
			${AAS_INCLUDES}
			${CM_INCLUDES}
			${FRAMEWORK_INCLUDES}
			${FRAMEWORK_ASYNC_INCLUDES}
			${RENDERER_INCLUDES}
			${RENDERER_COLOR_INCLUDES}
			${RENDERER_DXT_INCLUDES}
			${RENDERER_JOBS_INCLUDES}
			${RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES}
			${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES}
			${RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES}
			
			${SHADERS}
			${SHADERS_BUILTIN}
			${SHADERS_BUILTIN_DEBUG}
			${SHADERS_BUILTIN_FOG}
			${SHADERS_BUILTIN_LEGACY}
			${SHADERS_BUILTIN_LIGHTING}
			${SHADERS_BUILTIN_POST}
			${SHADERS_BUILTIN_SSAO}
			${SHADERS_BUILTIN_SSGI}
			${SHADERS_BUILTIN_VIDEO}
			${SHADERS_BUILTIN_VR}
			
			${RENDERER_SMAA_INCLUDES}
			${IRRXML_INCLUDES}
			${FRAMEWORK_IMGUI_INCLUDES}
			${IMGUI_INCLUDES}
			${STB_INCLUDES}
			${MESA_INCLUDES}
			${TINYEXR_INCLUDES}
			${MIKKTSPACE_INCLUDES}
			${JPEG_INCLUDES}
			${PNG_INCLUDES}
			${ZLIB_INCLUDES}
			${MINIZIP_INCLUDES}
			${BINKDEC_INCLUDES}
			${GLEW_INCLUDES}
			${RAPIDJSON_INCLUDES}
			#${FREETYPE_SOURCES}
			${SOUND_INCLUDES}
			#${OGGVORBIS_INCLUDES}
			${UI_INCLUDES}
			${SWF_INCLUDES}
			${COMMON_INCLUDES}
			
			${COMPILER_INCLUDES}
			${COMPILER_AAS_INCLUDES} ${COMPILER_AAS_SOURCES}
			${COMPILER_DMAP_INCLUDES} ${COMPILER_DMAP_SOURCES}
			
			${TOOLS_INCLUDES}
			${IMGUI_EDITOR_INCLUDES}
			${IMGUI_EDITOR_LIGHT_INCLUDES}
			
			${GAMED3XP_INCLUDES}
			${GAMED3XP_AI_INCLUDES} 
			${GAMED3XP_ANIM_INCLUDES} 
			${GAMED3XP_GAMESYS_INCLUDES} 
			${GAMED3XP_MENUS_INCLUDES}
			${GAMED3XP_PHYSICS_INCLUDES}
			${GAMED3XP_SCRIPT_INCLUDES}
			)
			
set(RBDOOM3_SOURCES
			${AAS_SOURCES}
			${CM_SOURCES}
			${FRAMEWORK_SOURCES}
			${FRAMEWORK_ASYNC_SOURCES} 
			${RENDERER_SOURCES}
			${RENDERER_COLOR_SOURCES}
			${RENDERER_DXT_SOURCES}
			${RENDERER_JOBS_SOURCES}
			${RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES}
			${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES}
			${RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES}
			${IRRXML_SOURCES}
			${FRAMEWORK_IMGUI_SOURCES}
			${IMGUI_SOURCES}
			${MIKKTSPACE_SOURCES}
			${JPEG_SOURCES}
			${PNG_SOURCES}
			${ZLIB_SOURCES}
			${MINIZIP_SOURCES}
			${BINKDEC_SOURCES}
			${GLEW_SOURCES}
			#${FREETYPE_SOURCES}
			${SOUND_SOURCES}
			#${OGGVORBIS_INCLUDES} ${OGGVORBIS_SOURCES}
			${UI_SOURCES}
			${SWF_SOURCES}
			${COMMON_SOURCES}
			
			${COMPILER_AAS_SOURCES}
			${COMPILER_DMAP_SOURCES}
			
			${IMGUI_EDITOR_SOURCES}
			${IMGUI_EDITOR_LIGHT_SOURCES}
			
			${GAMED3XP_SOURCES}
			${GAMED3XP_AI_SOURCES}
			${GAMED3XP_ANIM_SOURCES}
			${GAMED3XP_GAMESYS_SOURCES}
			${GAMED3XP_MENUS_SOURCES}
			${GAMED3XP_PHYSICS_SOURCES}
			${GAMED3XP_SCRIPT_SOURCES}
			)

if(DOOM_CLASSIC)
	add_definitions(-DUSE_DOOMCLASSIC)

	list(APPEND RBDOOM3_INCLUDES ${DOOMCLASSIC_INCLUDES})
	list(APPEND RBDOOM3_INCLUDES ${TIMIDITY_INCLUDES})
	
	list(APPEND RBDOOM3_SOURCES ${DOOMCLASSIC_SOURCES})
	list(APPEND RBDOOM3_SOURCES ${TIMIDITY_SOURCES})
endif()

add_definitions(-D__DOOM__
				#-DBUILD_FREETYPE
				#-DFT2_BUILD_LIBRARY
				)
				

include_directories(
			.
			idlib
            )

if(MSVC)
	
	if(WINDOWS10)
		set(DirectX_LIBRARIES
			dinput8
			dsound
			dxguid
			#dxerr
			Xinput
			xaudio2.lib
			)
	else()
		find_package(DirectX REQUIRED)
		include_directories(${DirectX_INCLUDE_DIR})
	endif()
	
	if(USE_VULKAN)
		list(APPEND RBDOOM3_INCLUDES ${RENDERER_VULKAN_INCLUDES})
		list(APPEND RBDOOM3_SOURCES ${RENDERER_VULKAN_SOURCES})
		
		set(Vulkan_LIBRARIES 
			${Vulkan_LIBRARY})

		if(SPIRV_SHADERC)
			list(APPEND Vulkan_LIBRARIES shaderc_combined)
		else()
			list(APPEND Vulkan_LIBRARIES glslang SPIRV)

			if(ENABLE_GLSLANG_BINARIES)
				list(APPEND Vulkan_LIBRARIES glslang-default-resource-limits)
			endif()
		endif()
		
	else()
		list(APPEND RBDOOM3_INCLUDES ${RENDERER_OPENGL_INCLUDES})
		list(APPEND RBDOOM3_SOURCES ${RENDERER_OPENGL_SOURCES})
		
		set(OpenGL_LIBRARIES
			opengl32
			glu32)
	endif()
	
	if(OPENAL)
		add_definitions(-DUSE_OPENAL)
	
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/include)
		
		if(CMAKE_CL_64)
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/win64)
		else()
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/win32)
		endif()
		
		list(APPEND RBDOOM3_INCLUDES ${OPENAL_INCLUDES})
		list(APPEND RBDOOM3_SOURCES	${OPENAL_SOURCES})
		if(DOOM_CLASSIC)
			list(APPEND RBDOOM3_SOURCES	${DOOMCLASSIC_OPENAL_SOURCES})
		endif()		
		
		set(OpenAL_LIBRARIES
			OpenAL32)
			
		if(CMAKE_CL_64)
			install(FILES libs/openal-soft/lib/bin/win64/OpenAL32.dll DESTINATION .)
		else()
			install(FILES libs/openal-soft/lib/bin/win32/OpenAL32.dll DESTINATION .)
			install(FILES libs/openal-soft/lib/bin/win32/OpenAL32.pdb DESTINATION .)
		endif()
	else()
		list(APPEND RBDOOM3_INCLUDES ${XAUDIO2_INCLUDES})
		list(APPEND RBDOOM3_SOURCES	${XAUDIO2_SOURCES})

		if(DOOM_CLASSIC)
			list(APPEND RBDOOM3_SOURCES	${DOOMCLASSIC_XAUDIO2_SOURCES})
		endif()
	endif()
	
	if(FFMPEG)
		add_definitions(-DUSE_FFMPEG)
	
		if(CMAKE_CL_64)
			include_directories(libs/ffmpeg-win64/include)
			include_directories(libs/ffmpeg-win64/include/libswscale)
			include_directories(libs/ffmpeg-win64/include/libavformat)
			include_directories(libs/ffmpeg-win64/include/libavdevice)
			include_directories(libs/ffmpeg-win64/include/libavcodec)
			include_directories(libs/ffmpeg-win64/include/libswresample)
			
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg-win64/lib)
		else()
			include_directories(libs/ffmpeg-win32/include)
			include_directories(libs/ffmpeg-win32/include/libswscale)
			include_directories(libs/ffmpeg-win32/include/libavformat)
			include_directories(libs/ffmpeg-win32/include/libavdevice)
			include_directories(libs/ffmpeg-win32/include/libavcodec)
			include_directories(libs/ffmpeg-win32/include/libswresample)
			
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg-win32/lib)
		endif()
		
		set(FFmpeg_LIBRARIES
				avcodec
				avformat
				avutil
				swscale
				swresample)
		
	endif()
	
	if(USE_MFC_TOOLS)
		
		include_directories(libs/atlmfc/include)
		if(CMAKE_CL_64)
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/atlmfc/lib/amd64)
		else()
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/atlmfc/lib)
		endif()
		
		add_definitions(-DUSE_MFC_TOOLS)
		
		list(APPEND RBDOOM3_SOURCES
			${TOOLS_COMAFX_INCLUDES} ${TOOLS_COMAFX_SOURCES}
			${TOOLS_COMMON_INCLUDES} ${TOOLS_COMMON_SOURCES}
			${TOOLS_COMMON_PROPTREE_INCLUDES} ${TOOLS_COMMON_PROPTREE_SOURCES}
			
			${EDITOR_AF_INCLUDES} ${EDITOR_AF_SOURCES}
			${EDITOR_DEBUGGER_INCLUDES} ${EDITOR_DEBUGGER_SOURCES}
			${EDITOR_DECL_INCLUDES} ${EDITOR_DECL_SOURCES}
			${EDITOR_GUIED_INCLUDES} ${EDITOR_GUIED_SOURCES}
			${EDITOR_MATERIALEDITOR_INCLUDES} ${EDITOR_MATERIALEDITOR_SOURCES}
			${EDITOR_PARTICLE_INCLUDES} ${EDITOR_PARTICLE_SOURCES}
			${EDITOR_PDA_INCLUDES} ${EDITOR_PDA_SOURCES}
			${EDITOR_RADIANT_INCLUDES} ${EDITOR_RADIANT_SOURCES}
			${EDITOR_SCRIPT_INCLUDES} ${EDITOR_SCRIPT_SOURCES}
			${EDITOR_SOUND_INCLUDES} ${EDITOR_SOUND_SOURCES})
	endif()
	
	list(APPEND RBDOOM3_INCLUDES
		${SYS_INCLUDES} 
		${WIN32_INCLUDES})
		
	list(APPEND RBDOOM3_SOURCES
		${SYS_SOURCES}
		${WIN32_SOURCES})

	list(REMOVE_DUPLICATES RBDOOM3_SOURCES)
	
	if(USE_PRECOMPILED_HEADERS) 
		set(RBDOOM3_PRECOMPILED_SOURCES ${RBDOOM3_SOURCES})
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${TIMIDITY_SOURCES} ${JPEG_SOURCES} ${PNG_SOURCES} ${ZLIB_SOURCES} ${GLEW_SOURCES} ${BINKDEC_SOURCES} ${IMGUI_SOURCES} ${MIKKTSPACE_SOURCES})
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib/minizip/ioapi.c)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTDecoder.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder_SSE2.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/dynamicshadowvolume/DynamicShadowVolume.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/prelightshadowvolume/PreLightShadowVolume.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/staticshadowvolume/StaticShadowVolume.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/ShadowShared.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/RenderLog.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/Vulkan/vma.cpp)
		#list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES d3xp/gamesys/Class.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/framework/precompiled.cpp)
	
		foreach( src_file ${RBDOOM3_PRECOMPILED_SOURCES} )
			message(STATUS "-include precompiled.h for ${src_file}")
		endforeach()
	
		set_source_files_properties(
		${RBDOOM3_PRECOMPILED_SOURCES}
		PROPERTIES
		COMPILE_FLAGS "/Yuprecompiled.h"
		OBJECT_DEPENDS "precompiled.pch"
		)
		
		set_source_files_properties(framework/precompiled.cpp
		PROPERTIES
		COMPILE_FLAGS "/Ycprecompiled.h"
		OBJECT_OUTPUTS "precompiled.pch"
		)
	endif()
	
	set_source_files_properties( ${SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None" )
	set_source_files_properties( ${SHADERS_BUILTIN} PROPERTIES VS_TOOL_OVERRIDE "None" )
	set_source_files_properties( ${SHADERS_BUILTIN_DEBUG} PROPERTIES VS_TOOL_OVERRIDE "None" )
	set_source_files_properties( ${SHADERS_BUILTIN_FOG} PROPERTIES VS_TOOL_OVERRIDE "None" )
	set_source_files_properties( ${SHADERS_BUILTIN_LEGACY} PROPERTIES VS_TOOL_OVERRIDE "None" )
	set_source_files_properties( ${SHADERS_BUILTIN_LIGHTING} PROPERTIES VS_TOOL_OVERRIDE "None" )
	set_source_files_properties( ${SHADERS_BUILTIN_POST} PROPERTIES VS_TOOL_OVERRIDE "None" )
	set_source_files_properties( ${SHADERS_BUILTIN_SSAO} PROPERTIES VS_TOOL_OVERRIDE "None" )
	set_source_files_properties( ${SHADERS_BUILTIN_SSGI} PROPERTIES VS_TOOL_OVERRIDE "None" )
	set_source_files_properties( ${SHADERS_BUILTIN_VIDEO} PROPERTIES VS_TOOL_OVERRIDE "None" )
	set_source_files_properties( ${SHADERS_BUILTIN_VR} PROPERTIES VS_TOOL_OVERRIDE "None" )
	
	list(APPEND RBDOOM3_SOURCES ${WIN32_RESOURCES})
	
	add_executable(RBDoom3BFG WIN32 ${RBDOOM3_INCLUDES} ${RBDOOM3_SOURCES})

	add_dependencies(RBDoom3BFG idlib)
	target_link_libraries(RBDoom3BFG
		idlib
		${DirectX_LIBRARIES}
		dbghelp
		#eaxguid
		iphlpapi
		winmm
		wsock32.lib
		${Vulkan_LIBRARIES}
		${OpenGL_LIBRARIES}
		${OpenAL_LIBRARIES}
		${FFmpeg_LIBRARIES}
        ${ZLIB_LIBRARY}
        ${PNG_LIBRARY}
        ${JPEG_LIBRARY}
        ${GLEW_LIBRARY}
		${CMAKE_DL_LIBS}
		)
		
	# set MSVC default debug directory on executable	
	if(CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION GREATER_EQUAL 8)
		set_target_properties(RBDoom3BFG PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/..")
	endif()
	
	#CMAKE_BINARY_DIR
	if(CMAKE_CL_64)
		install(TARGETS RBDoom3BFG
			RUNTIME DESTINATION .)
	else()
		install(TARGETS RBDoom3BFG
			RUNTIME DESTINATION .)
	endif()	

else()

	list(APPEND RBDOOM3_SOURCES
		${SYS_INCLUDES} ${SYS_SOURCES})
	
	if(UNIX)
		if(FFMPEG)
			find_package(FFMPEG REQUIRED)
			add_definitions(-DUSE_FFMPEG)
			
			include_directories(${FFMPEG_INCLUDE_DIR})
			link_directories(${FFMPEG_LIBRARIES_DIRS})
		endif()

		if(SDL2)
			find_package(SDL2 REQUIRED)
			include_directories(${SDL2_INCLUDE_DIRS})
			set(SDLx_LIBRARY ${SDL2_LIBRARIES})
		else()
			find_package(SDL REQUIRED)
			include_directories(${SDL_INCLUDE_DIR})
			set(SDLx_LIBRARY ${SDL_LIBRARY})
		endif()

		if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
			list(REMOVE_ITEM POSIX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/platform_linux.cpp)
		else()
			list(REMOVE_ITEM POSIX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/platform_osx.cpp)
		endif()

		list(APPEND RBDOOM3_SOURCES
			${POSIX_INCLUDES} ${POSIX_SOURCES}
			${SDL_INCLUDES} ${SDL_SOURCES})
			
		if(OPENAL)
			find_package(OpenAL REQUIRED)
			add_definitions(-DUSE_OPENAL)

			# SRS - Added support for OpenAL Soft headers on OSX (vs default macOS SDK headers)
			if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT OPENAL_INCLUDE_DIR MATCHES "SDKs/MacOSX.*\.sdk")
				include_directories(${OPENAL_INCLUDE_DIR})
				add_definitions(-DUSE_OPENAL_SOFT_INCLUDES)
			endif()

			list(APPEND RBDOOM3_INCLUDES ${OPENAL_INCLUDES})
			list(APPEND RBDOOM3_SOURCES ${OPENAL_SOURCES})
				
			if(DOOM_CLASSIC)
				list(APPEND RBDOOM3_SOURCES	${DOOMCLASSIC_OPENAL_SOURCES})
			endif()
		else()
			list(APPEND RBDOOM3_INCLUDES ${STUBAUDIO_INCLUDES})
			list(APPEND RBDOOM3_SOURCES ${STUBAUDIO_SOURCES})
			
			if(DOOM_CLASSIC)
				list(APPEND RBDOOM3_SOURCES	${DOOMCLASSIC_STUBAUDIO_SOURCES})
			endif()
		endif()
		
		if(USE_VULKAN)
            list(APPEND RBDOOM3_INCLUDES ${RENDERER_VULKAN_INCLUDES})
            list(APPEND RBDOOM3_SOURCES ${RENDERER_VULKAN_SOURCES})
            
            set(Vulkan_LIBRARIES 
            	${Vulkan_LIBRARY})

			# SRS - Added UNIX/OSX support for shaderc
			if(SPIRV_SHADERC)
				list(APPEND Vulkan_LIBRARIES shaderc_combined)
			else()
				list(APPEND Vulkan_LIBRARIES glslang SPIRV)

                if(ENABLE_GLSLANG_BINARIES)
                    list(APPEND Vulkan_LIBRARIES glslang-default-resource-limits)
                endif()
            endif()

        else()
			find_package(OpenGL REQUIRED)
			include_directories(${OPENGL_INCLUDE_DIRS})

            list(APPEND RBDOOM3_INCLUDES ${RENDERER_OPENGL_INCLUDES})
            list(APPEND RBDOOM3_SOURCES ${RENDERER_OPENGL_SOURCES})
        endif()
			
	endif()

	list(REMOVE_DUPLICATES RBDOOM3_SOURCES)
	
	if(USE_PRECOMPILED_HEADERS) 	
		set(RBDOOM3_PRECOMPILED_SOURCES ${RBDOOM3_SOURCES})
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${TIMIDITY_SOURCES} ${JPEG_SOURCES} ${PNG_SOURCES} ${ZLIB_SOURCES} ${GLEW_SOURCES} ${BINKDEC_SOURCES} ${IMGUI_SOURCES} ${MIKKTSPACE_SOURCES})
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib/minizip/ioapi.c)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTDecoder.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder_SSE2.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/dynamicshadowvolume/DynamicShadowVolume.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/prelightshadowvolume/PreLightShadowVolume.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/staticshadowvolume/StaticShadowVolume.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/ShadowShared.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/RenderLog.cpp)

		foreach( src_file ${RBDOOM3_PRECOMPILED_SOURCES} )
			#message(STATUS "-include precompiled.h for ${src_file}")
			set_source_files_properties(
				${src_file}
				PROPERTIES
				COMPILE_FLAGS "-include ${CMAKE_CURRENT_SOURCE_DIR}/idlib/precompiled.h"
				)
		endforeach()

		# precompiled magic for GCC/clang, adapted from https://gist.github.com/573926
		STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
		SET(_compiler_FLAGS "${${_flags_var_name}} -std=c++${CMAKE_CXX_STANDARD}")
		GET_DIRECTORY_PROPERTY(_directory_flags INCLUDE_DIRECTORIES)
		FOREACH(item ${_directory_flags})
			LIST(APPEND _compiler_FLAGS " -I${item}")
		ENDFOREACH(item)
	endif()
		
	GET_DIRECTORY_PROPERTY(_directory_flags DEFINITIONS)
	LIST(APPEND _compiler_FLAGS ${_directory_flags})
	SEPARATE_ARGUMENTS(_compiler_FLAGS)
	
	if(USE_PRECOMPILED_HEADERS)
		# we need to recreate the precompiled header for RBDoom3BFG 
		# (i.e. can't use the one created for idlib before)
		# because some definitions (e.g. -D__IDLIB__ -D__DOOM_DLL__) differ
		add_custom_target(precomp_header_rbdoom3bfg ALL
				COMMAND ${CMAKE_CXX_COMPILER} ${_compiler_FLAGS} -x c++-header idlib/precompiled.h -o idlib/precompiled.h.gch
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				COMMENT "Creating idlib/precompiled.h.gch for RBDoom3BFG"
				)
	endif()
	
	if(WIN32)
		set(remove_command "del")
	else()
		set(remove_command "rm")
	endif()
	
	if (USE_PRECOMPILED_HEADERS)
		# it's ugly enough that the precompiled header binary needs to be in the 
		# source directory (instead of the build directory), so let's at least
		# delete it after build.
		add_custom_target(rm_precomp_header ALL
				COMMAND ${remove_command} "idlib/precompiled.h.gch"
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				COMMENT "remove idlib/precompiled.h.gch"
				)
				
		# make sure this is run after creating idlib
		add_dependencies(precomp_header_rbdoom3bfg idlib)
	endif()

	add_executable(RBDoom3BFG WIN32 ${RBDOOM3_SOURCES})

	if (USE_PRECOMPILED_HEADERS)
		# make sure precompiled header is created before executable is compiled
		add_dependencies(RBDoom3BFG precomp_header_rbdoom3bfg)
		
		# make sure precompiled header is deleted after executable is compiled
		add_dependencies(rm_precomp_header RBDoom3BFG)
	endif()	

	if(NOT WIN32)
		if(NOT "${CMAKE_SYSTEM}" MATCHES "Darwin")
			set(RT_LIBRARY rt)
		endif()

		target_link_libraries(RBDoom3BFG
			idlib
			${Vulkan_LIBRARIES}
			${OPENGL_LIBRARIES}
			pthread
			${RT_LIBRARY}
			${SDLx_LIBRARY}
			${OPENAL_LIBRARY}
			${FFMPEG_LIBRARIES}
            ${ZLIB_LIBRARY}
            ${PNG_LIBRARY}
            ${JPEG_LIBRARY}
            ${GLEW_LIBRARY}
	    	${CMAKE_DL_LIBS}
			)
	endif()
    
endif()
