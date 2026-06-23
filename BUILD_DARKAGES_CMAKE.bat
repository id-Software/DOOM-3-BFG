@echo off
REM ============================================================================
REM DOOM: The Dark Ages - CMake Build Script for Windows
REM Alternative build using CMake (requires CMake + Visual Studio)
REM ============================================================================

echo =============================================
echo   DOOM: The Dark Ages - CMake Build
echo =============================================
echo.

where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found! Install from https://cmake.org/download/
    pause
    exit /b 1
)

echo [1/4] Configuring with CMake...
mkdir build_cmake 2>nul
cd build_cmake

cmake .. -G "Visual Studio 17 2022" -A Win32 ^
    -DDARKAGES_PORTABLE=ON ^
    -DDARKAGES_OPTIMIZE_T460S=ON ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo Trying Visual Studio 2019...
    cmake .. -G "Visual Studio 16 2019" -A Win32 ^
        -DDARKAGES_PORTABLE=ON ^
        -DDARKAGES_OPTIMIZE_T460S=ON ^
        -DCMAKE_BUILD_TYPE=Release
)

if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [2/4] Building Release...
cmake --build . --config Release --parallel

if errorlevel 1 (
    echo BUILD FAILED!
    pause
    exit /b 1
)

echo.
echo [3/4] Creating package...
cpack -G ZIP -C Release

echo.
echo [4/4] Done!
echo.
echo   Output: build_cmake\Release\DarkAges.exe
echo   Package: build_cmake\DOOM-The-Dark-Ages-v1.0.0-Portable.zip
echo.

cd ..
pause
