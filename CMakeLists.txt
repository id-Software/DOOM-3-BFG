cmake_minimum_required(VERSION 3.16)
project(DoomTheDarkAges VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

# Build options
option(DARKAGES_PORTABLE "Build portable standalone executable" ON)
option(DARKAGES_OPTIMIZE_T460S "Optimize for ThinkPad T460s (Intel HD 520)" ON)
option(USE_OPENGL "Use OpenGL renderer" ON)
option(USE_OPENAL "Use OpenAL for audio" ON)

# Performance flags for T460s (Intel HD 520, Core i5-6300U)
if(DARKAGES_OPTIMIZE_T460S)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=skylake -mtune=skylake")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2 -mavx -mavx2")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto -ffast-math")
        set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -flto -ffast-math")
    elseif(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
        set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /GL /GF /Gy /DNDEBUG")
        set(CMAKE_C_FLAGS_RELEASE "/O2 /Ob2 /GL /GF /Gy /DNDEBUG")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF")
    endif()
endif()

# Portable build - static linking
if(DARKAGES_PORTABLE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
    if(WIN32)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# Source directories
set(NEO_DIR ${CMAKE_SOURCE_DIR}/neo)
set(BASE_DIR ${CMAKE_SOURCE_DIR}/darkages)

# idLib sources
file(GLOB_RECURSE IDLIB_SOURCES
    ${NEO_DIR}/idlib/*.cpp
    ${NEO_DIR}/idlib/*.h
)

# Renderer sources
file(GLOB RENDERER_SOURCES
    ${NEO_DIR}/renderer/*.cpp
    ${NEO_DIR}/renderer/*.h
)

# Framework sources
file(GLOB FRAMEWORK_SOURCES
    ${NEO_DIR}/framework/*.cpp
    ${NEO_DIR}/framework/*.h
)

# Sound sources
file(GLOB SOUND_SOURCES
    ${NEO_DIR}/sound/*.cpp
    ${NEO_DIR}/sound/*.h
)

# AAS sources
file(GLOB AAS_SOURCES
    ${NEO_DIR}/aas/*.cpp
    ${NEO_DIR}/aas/*.h
)

# Collision Model sources
file(GLOB CM_SOURCES
    ${NEO_DIR}/cm/*.cpp
    ${NEO_DIR}/cm/*.h
)

# UI sources
file(GLOB UI_SOURCES
    ${NEO_DIR}/ui/*.cpp
    ${NEO_DIR}/ui/*.h
)

# SWF sources
file(GLOB SWF_SOURCES
    ${NEO_DIR}/swf/*.cpp
    ${NEO_DIR}/swf/*.h
)

# Game sources (d3xp)
file(GLOB_RECURSE GAME_SOURCES
    ${NEO_DIR}/d3xp/*.cpp
    ${NEO_DIR}/d3xp/*.h
)

# Dark Ages specific sources
file(GLOB_RECURSE DARKAGES_SOURCES
    ${NEO_DIR}/darkages/*.cpp
    ${NEO_DIR}/darkages/*.h
)

# System sources
if(WIN32)
    file(GLOB SYS_SOURCES
        ${NEO_DIR}/sys/*.cpp
        ${NEO_DIR}/sys/*.h
        ${NEO_DIR}/sys/win32/*.cpp
        ${NEO_DIR}/sys/win32/*.h
    )
    # Windows resource file with icon
    set(WIN_RC_FILE ${NEO_DIR}/sys/win32/darkages.rc)
else()
    file(GLOB SYS_SOURCES
        ${NEO_DIR}/sys/*.cpp
        ${NEO_DIR}/sys/*.h
    )
    set(WIN_RC_FILE "")
endif()

# All sources
set(ALL_SOURCES
    ${IDLIB_SOURCES}
    ${RENDERER_SOURCES}
    ${FRAMEWORK_SOURCES}
    ${SOUND_SOURCES}
    ${AAS_SOURCES}
    ${CM_SOURCES}
    ${UI_SOURCES}
    ${SWF_SOURCES}
    ${GAME_SOURCES}
    ${DARKAGES_SOURCES}
    ${SYS_SOURCES}
)

# Include directories
include_directories(
    ${NEO_DIR}
    ${NEO_DIR}/idlib
    ${NEO_DIR}/d3xp
    ${NEO_DIR}/darkages
)

# Definitions
add_definitions(
    -D__DOOM_DLL__
    -DGAME_DLL
    -DDARKAGES_BUILD
    -DGAME_VERSION="DarkAges-1.0"
)

if(WIN32)
    add_definitions(-D_WIN32 -DWIN32 -D_WINDOWS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
endif()

# Find packages
if(USE_OPENGL)
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIR})
    add_definitions(-DUSE_OPENGL)
endif()

# Executable - WIN32 flag creates a Windows GUI app (no console window)
if(WIN32)
    enable_language(RC)
    add_executable(DoomTheDarkAges WIN32 ${ALL_SOURCES} ${WIN_RC_FILE})
    set_target_properties(DoomTheDarkAges PROPERTIES
        OUTPUT_NAME "DarkAges"
        WIN32_EXECUTABLE TRUE
        SUFFIX ".exe"
    )
else()
    add_executable(DoomTheDarkAges ${ALL_SOURCES})
    set_target_properties(DoomTheDarkAges PROPERTIES
        OUTPUT_NAME "DarkAges"
    )
endif()

if(USE_OPENGL)
    target_link_libraries(DoomTheDarkAges ${OPENGL_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(DoomTheDarkAges
        winmm
        ws2_32
        iphlpapi
        dbghelp
        dinput8
        dxguid
        dsound
        xinput
    )
elseif(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(DoomTheDarkAges
        ${CMAKE_THREAD_LIBS_INIT}
        dl
    )
    if(USE_OPENAL)
        find_package(PkgConfig)
        pkg_check_modules(OPENAL openal)
        if(OPENAL_FOUND)
            target_link_libraries(DoomTheDarkAges ${OPENAL_LIBRARIES})
            target_include_directories(DoomTheDarkAges PRIVATE ${OPENAL_INCLUDE_DIRS})
        endif()
    endif()
endif()

# Installation
install(TARGETS DoomTheDarkAges DESTINATION .)
install(DIRECTORY ${BASE_DIR}/ DESTINATION darkages)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/base/renderprogs DESTINATION darkages)
install(FILES ${CMAKE_SOURCE_DIR}/darkages/icon/darkages.ico DESTINATION .)

# Package settings for portable build
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "DoomTheDarkAges")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DOOM: The Dark Ages - Medieval FPS")
set(CPACK_PACKAGE_FILE_NAME "DOOM-The-Dark-Ages-v${PROJECT_VERSION}-Portable")
include(CPack)
