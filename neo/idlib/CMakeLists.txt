
add_definitions(-D__IDLIB__ -D__DOOM_DLL__)

file(GLOB_RECURSE ID_INCLUDES *.h)
file(GLOB_RECURSE ID_SOURCES *.cpp)

if(MSVC)
	list(REMOVE_ITEM ID_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/posix_thread.cpp)
else()
	list(REMOVE_ITEM ID_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/win32/win_thread.cpp)
endif()

#if(STANDALONE)
#	add_definitions(-DSTANDALONE)
#endif()
	
add_library(idlib ${ID_SOURCES} ${ID_INCLUDES})
