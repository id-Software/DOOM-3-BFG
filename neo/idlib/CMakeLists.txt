
add_definitions(-D__IDLIB__ -D__DOOM_DLL__)

file(GLOB_RECURSE ID_INCLUDES *.h)
file(GLOB_RECURSE ID_SOURCES *.cpp)

if(MSVC)
	list(REMOVE_ITEM ID_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/math/Simd_AltiVec.h)
	list(REMOVE_ITEM ID_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/math/Simd_AltiVec.cpp)
	list(REMOVE_ITEM ID_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/bv/Frustum_gcc.cpp)
elseif(NOT APPLE)
	list(REMOVE_ITEM ID_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/math/Simd_AltiVec.h)
	list(REMOVE_ITEM ID_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/math/Simd_AltiVec.cpp)
endif()

#if(STANDALONE)
#	add_definitions(-DSTANDALONE)
#endif()
	
add_library(idlib ${ID_SOURCES} ${ID_INCLUDES})
