# MinGW-w64 cross-compilation toolchain
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Ensure __MINGW32__ is defined
add_definitions(-D_MINGW64_)

# Force linking libmingwex to get _chkstk
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--whole-archive /usr/x86_64-w64-mingw32/lib/libmingwex.a /usr/lib/gcc/x86_64-w64-mingw32/14-win32/libgcc.a -Wl,--no-whole-archive")

# Search for programs only in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
