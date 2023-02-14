include(GNUInstallDirs)
target_include_directories(VulkanMemoryAllocator PUBLIC 
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIRS}>
)
install(TARGETS VulkanMemoryAllocator
        EXPORT  VulkanMemoryAllocatorTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
# install(FILES "${PROJECT_SOURCE_DIR}/include/vk_mem_alloc.h" DESTINATION "include")
install(FILES "${PROJECT_SOURCE_DIR}/include/vk_mem_alloc.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT VulkanMemoryAllocatorTargets
        FILE VulkanMemoryAllocatorTargets.cmake
        NAMESPACE VulkanMemoryAllocator::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VulkanMemoryAllocator
)
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/VulkanMemoryAllocatorConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VulkanMemoryAllocator
)
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/VulkanMemoryAllocatorConfig.cmake"
         DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VulkanMemoryAllocator
)



