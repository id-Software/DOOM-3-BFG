# ============================================================================
# DOOM: The Dark Ages - Windows x86_64 Cross-Compilation Toolchain
# For building standalone portable EXE from Linux
# ============================================================================

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compilers
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Target environment
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Static linking for portable build
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

# Windows-specific
add_definitions(-D_WIN32 -DWIN32 -D_WINDOWS)
add_definitions(-DDARKAGES_BUILD)

# MinGW compatibility flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wno-unknown-pragmas -Wno-narrowing -Wno-sign-compare -Wno-write-strings -Wno-unused-variable -Wno-unused-but-set-variable -Wno-multichar")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas -Wno-sign-compare -Wno-unused-variable")
