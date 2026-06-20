@echo off
REM ============================================================================
REM DOOM: The Dark Ages - Windows Build Script
REM Run this on your ThinkPad T460s with Visual Studio installed
REM ============================================================================

echo =============================================
echo   DOOM: The Dark Ages - Build Script
echo =============================================
echo.

REM Try to find Visual Studio
set "VS_FOUND=0"

REM Visual Studio 2022
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    set "VS_FOUND=1"
    echo Found Visual Studio 2022 Community
)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
    set "VS_FOUND=1"
    echo Found Visual Studio 2022 Professional
)

REM Visual Studio 2019
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
    set "VS_FOUND=1"
    echo Found Visual Studio 2019 Community
)

REM Visual Studio 2017
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
    set "VS_FOUND=1"
    echo Found Visual Studio 2017 Community
)

if "%VS_FOUND%"=="0" (
    echo ERROR: Visual Studio not found!
    echo Please install Visual Studio 2017/2019/2022 with C++ tools
    echo Or open "Developer Command Prompt" and run this script from there
    pause
    exit /b 1
)

echo.
echo [1/3] Building DOOM: The Dark Ages...
echo.

cd /d "%~dp0neo"

REM Build Release configuration
msbuild doom3.sln /p:Configuration=Release /p:Platform=Win32 /m /v:minimal

if errorlevel 1 (
    echo.
    echo BUILD FAILED! Check errors above.
    pause
    exit /b 1
)

echo.
echo [2/3] Build successful!
echo.

REM Copy output
set OUTPUT_DIR=%~dp0build\Win32\Release
echo Output directory: %OUTPUT_DIR%

if exist "%OUTPUT_DIR%\DarkAges.exe" (
    echo.
    echo [3/3] Packaging standalone...
    
    REM Create standalone directory
    set STANDALONE_DIR=%~dp0DOOM_Dark_Ages_Standalone
    mkdir "%STANDALONE_DIR%" 2>nul
    
    copy "%OUTPUT_DIR%\DarkAges.exe" "%STANDALONE_DIR%\" /Y
    xcopy "%~dp0darkages" "%STANDALONE_DIR%\darkages\" /E /I /Y /Q
    xcopy "%~dp0base\renderprogs" "%STANDALONE_DIR%\base\renderprogs\" /E /I /Y /Q
    copy "%~dp0darkages\icon\darkages.ico" "%STANDALONE_DIR%\" /Y
    
    echo.
    echo =============================================
    echo   BUILD COMPLETE!
    echo =============================================
    echo.
    echo   EXE: %STANDALONE_DIR%\DarkAges.exe
    echo   Icon: darkages.ico (embedded in EXE)
    echo.
    echo   To play: Run DarkAges.exe
    echo =============================================
) else (
    echo WARNING: DarkAges.exe not found at expected location
    echo Check: %OUTPUT_DIR%
    dir "%OUTPUT_DIR%\*.exe" 2>nul
)

echo.
pause
