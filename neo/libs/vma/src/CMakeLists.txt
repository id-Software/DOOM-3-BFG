set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_RELWITHDEBINFO_POSTFIX rd)
set(CMAKE_MINSIZEREL_POSTFIX s)

add_library(VulkanMemoryAllocator
    VmaUsage.cpp
    VmaUsage.h
    ${PROJECT_SOURCE_DIR}/include/vk_mem_alloc.h
)

if (MSVC)
    # Provides MSVC users nicer debugging support
    target_sources(VulkanMemoryAllocator PRIVATE ${CMAKE_CURRENT_LIST_DIR}/vk_mem_alloc.natvis)
endif()

set_target_properties(
    VulkanMemoryAllocator PROPERTIES

    CXX_EXTENSIONS OFF
    # Use C++14
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(VulkanMemoryAllocator PUBLIC 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# Only link to Vulkan if static linking is used
if(${VMA_STATIC_VULKAN_FUNCTIONS})
    target_link_libraries(VulkanMemoryAllocator PUBLIC Vulkan::Vulkan)
endif()

target_compile_definitions(
    VulkanMemoryAllocator

    PUBLIC
    VMA_STATIC_VULKAN_FUNCTIONS=$<BOOL:${VMA_STATIC_VULKAN_FUNCTIONS}>
    VMA_DYNAMIC_VULKAN_FUNCTIONS=$<BOOL:${VMA_DYNAMIC_VULKAN_FUNCTIONS}>
    VMA_DEBUG_ALWAYS_DEDICATED_MEMORY=$<BOOL:${VMA_DEBUG_ALWAYS_DEDICATED_MEMORY}>
    VMA_DEBUG_INITIALIZE_ALLOCATIONS=$<BOOL:${VMA_DEBUG_INITIALIZE_ALLOCATIONS}>
    VMA_DEBUG_GLOBAL_MUTEX=$<BOOL:${VMA_DEBUG_GLOBAL_MUTEX}>
    VMA_DEBUG_DONT_EXCEED_MAX_MEMORY_ALLOCATION_COUNT=$<BOOL:${VMA_DEBUG_DONT_EXCEED_MAX_MEMORY_ALLOCATION_COUNT}>
    VMA_RECORDING_ENABLED=$<BOOL:${VMA_RECORDING_ENABLED}>
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_target.cmake)

if(VMA_BUILD_SAMPLE)
    if(WIN32)
        set(VMA_SAMPLE_SOURCE_FILES
            Common.cpp
            Common.h
            SparseBindingTest.cpp
            SparseBindingTest.h
            Tests.cpp
            Tests.h
            VulkanSample.cpp
        )

        add_executable(VmaSample ${VMA_SAMPLE_SOURCE_FILES})
		add_dependencies(VmaSample VulkanMemoryAllocator VmaSampleShaders)
		
        if(MSVC)
            # Use Unicode instead of multibyte set
            add_compile_definitions(UNICODE _UNICODE)
            
            # Add C++ warnings and security checks
            add_compile_options(/permissive- /sdl /W3)
            
            # Enable multithreaded compiling
            target_compile_options(VmaSample PRIVATE "/MP")

            # Set VmaSample as startup project
            set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT "VmaSample")

            # Set working directory for Visual Studio debugger
            set_target_properties(
                VmaSample
                PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
            )
        endif()

        set_target_properties(
            VmaSample PROPERTIES

            CXX_EXTENSIONS OFF
            # Use C++14
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED ON
        )

        target_link_libraries(
            VmaSample
            PRIVATE
            
            VulkanMemoryAllocator
            Vulkan::Vulkan
        )
    else()
        message(STATUS "VmaSample application is not supported to Linux")
    endif()
endif()

if(VMA_BUILD_SAMPLE_SHADERS)
    add_subdirectory(Shaders)
endif()
