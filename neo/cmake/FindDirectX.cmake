
if(WIN32)
  
	find_path(DirectX_INCLUDE_DIR NAMES d3d9.h
		HINTS
		$ENV{DXSDK_DIR}
		PATH_SUFFIXES "include"
		)

	if(CMAKE_CL_64)
		set(DirectX_LIBPATH_SUFFIX "lib/x64")
	else(CMAKE_CL_64)
		set(DirectX_LIBPATH_SUFFIX "lib/x86")
	endif(CMAKE_CL_64)
	
	# dsound dxguid DxErr
	find_library(DirectX_DINPUT8_LIBRARY NAMES dinput8 HINTS $ENV{DXSDK_DIR} PATH_SUFFIXES ${DirectX_LIBPATH_SUFFIX})
	find_library(DirectX_DSOUND_LIBRARY NAMES dsound HINTS $ENV{DXSDK_DIR} PATH_SUFFIXES ${DirectX_LIBPATH_SUFFIX})
	find_library(DirectX_DXGUID_LIBRARY NAMES dxguid HINTS $ENV{DXSDK_DIR} PATH_SUFFIXES ${DirectX_LIBPATH_SUFFIX})
	#find_library(DirectX_DXERR_LIBRARY NAMES dxerr HINTS $ENV{DXSDK_DIR} PATH_SUFFIXES ${DirectX_LIBPATH_SUFFIX})
	find_library(DirectX_XINPUT_LIBRARY NAMES Xinput HINTS $ENV{DXSDK_DIR} PATH_SUFFIXES ${DirectX_LIBPATH_SUFFIX})
	find_library(DirectX_X3DAUDIO_LIBRARY NAMES x3daudio HINTS $ENV{DXSDK_DIR} PATH_SUFFIXES ${DirectX_LIBPATH_SUFFIX})
	
	set(DirectX_LIBRARIES
		${DirectX_DINPUT8_LIBRARY}
		${DirectX_DSOUND_LIBRARY}
		${DirectX_DXGUID_LIBRARY}
		#${DirectX_DXERR_LIBRARY}
		${DirectX_XINPUT_LIBRARY}
		${DirectX_X3DAUDIO_LIBRARY}
		)

	# handle the QUIETLY and REQUIRED arguments and set DirectX_FOUND to TRUE if
	# all listed variables are TRUE
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(DirectX 
		DEFAULT_MSG
		DirectX_INCLUDE_DIR
		DirectX_DINPUT8_LIBRARY
		DirectX_DSOUND_LIBRARY
		DirectX_DXGUID_LIBRARY
		#DirectX_DXERR_LIBRARY
		DirectX_XINPUT_LIBRARY
		DirectX_X3DAUDIO_LIBRARY
		)

	mark_as_advanced(DirectX_LIBRARIES DirectX_INCLUDE_DIR)
  
endif(WIN32)
