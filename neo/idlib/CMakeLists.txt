
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

if(MSVC)
	
	#set_target_properties(idlib PROPERTIES COMPILE_FLAGS "/Yuprecompiled.h")
	
	set(ID_PRECOMPILED_SOURCES ${ID_SOURCES})
	list(REMOVE_ITEM ID_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/geometry/RenderMatrix.cpp)
	list(REMOVE_ITEM ID_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/SoftwareCache.cpp)
	
	foreach( src_file ${ID_PRECOMPILED_SOURCES} )
		#message(STATUS "/Yuprecompiled.h for ${src_file}")
        set_source_files_properties(
            ${src_file}
            PROPERTIES
            COMPILE_FLAGS "/Yuprecompiled.h"
            )
    endforeach()
	
	set_source_files_properties(precompiled.cpp
        PROPERTIES
        COMPILE_FLAGS "/Ycprecompiled.h"
        )
endif()
	
add_library(idlib ${ID_SOURCES} ${ID_INCLUDES})

# if(MSVC)
	# # set_source_files_properties(precompiled.cpp
        # # PROPERTIES
        # # COMPILE_FLAGS "/Ycprecompiled.h"
        # # )
		
	# #set_target_properties(idlib PROPERTIES COMPILE_FLAGS "/Yuprecompiled.h")
# endif()
