
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

set(ID_PRECOMPILED_SOURCES ${ID_SOURCES})
list(REMOVE_ITEM ID_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/geometry/RenderMatrix.cpp)
list(REMOVE_ITEM ID_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/SoftwareCache.cpp)

if(MSVC)
	
	#set_target_properties(idlib PROPERTIES COMPILE_FLAGS "/Yuprecompiled.h")
	
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
        
    add_library(idlib ${ID_SOURCES} ${ID_INCLUDES})
else()
	foreach( src_file ${ID_PRECOMPILED_SOURCES} )
		#message(STATUS "-include precompiled.h for ${src_file}")
		set_source_files_properties(
			${src_file}
			PROPERTIES
			COMPILE_FLAGS "-include ${CMAKE_CURRENT_SOURCE_DIR}/precompiled.h"
			)
	endforeach()
	
	include_directories(.)
	
	# precompiled magic for GCC/clang, adapted from https://gist.github.com/573926
	STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
	SET(_compiler_FLAGS ${${_flags_var_name}})
	GET_DIRECTORY_PROPERTY(_directory_flags INCLUDE_DIRECTORIES)
	FOREACH(item ${_directory_flags})
		LIST(APPEND _compiler_FLAGS " -I${item}")
	ENDFOREACH(item)

	GET_DIRECTORY_PROPERTY(_directory_flags DEFINITIONS)
	LIST(APPEND _compiler_FLAGS ${_directory_flags})
	
	SEPARATE_ARGUMENTS(_compiler_FLAGS)
	
	add_custom_target(precomp_header_idlib ALL
	                  COMMAND ${CMAKE_CXX_COMPILER} ${_compiler_FLAGS} -x c++-header precompiled.h -o precompiled.h.gch
	                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	                  COMMENT "Creating idlib/precompiled.h.gch for idlib"
	                  )
	
	add_library(idlib ${ID_SOURCES} ${ID_INCLUDES})
	add_dependencies(idlib precomp_header_idlib)
	
endif()
	

# if(MSVC)
	# # set_source_files_properties(precompiled.cpp
        # # PROPERTIES
        # # COMPILE_FLAGS "/Ycprecompiled.h"
        # # )
		
	# #set_target_properties(idlib PROPERTIES COMPILE_FLAGS "/Yuprecompiled.h")
# endif()
