@ECHO OFF
REM rgd3 build: one cl.exe invocation over the single translation unit src\all.c.
REM Usage: build.bat [debug^|dev^|release^|profile^|clean] [output.exe]
SetLocal EnableDelayedExpansion
PUSHD "%~dp0" >NUL
IF ERRORLEVEL 1 (
	EndLocal
	EXIT /B 1
)

SET BUILD_MODE=%~1
IF "%BUILD_MODE%"=="" SET BUILD_MODE=dev
IF /I "%BUILD_MODE%"=="clean" GOTO :clean

SET PROJECT_NAME=rgd3_%BUILD_MODE%
SET OBJECT_DIR=obj\%BUILD_MODE%
SET LINK_EXTRA=
SET COMMON_FLAGS=/nologo /W3 /std:c11 /TC /FS /utf-8 /bigobj /fp:precise
SET COMMON_DEFINES=/DSDL_MAIN_HANDLED /D_CRT_SECURE_NO_WARNINGS /DRG_SPRINTF_NO_ASM /DRGD3_BUILD_MODE=\"%BUILD_MODE%\"

IF /I "%BUILD_MODE%"=="debug" (
	SET MODE_FLAGS=/Od /Z7 /MDd
	SET MODE_DEFINES=/D_DEBUG /DRGD3_DEBUG_BUILD=1 /DRG_ASSERT_ENABLED=1
) ELSE IF /I "%BUILD_MODE%"=="dev" (
	REM Everyday build: optimized code with asserts and full debug info.
	REM /Ob1 avoids pathological compile-time memory use in the header-based unity build.
	SET MODE_FLAGS=/O2 /Ob1 /Z7 /Zo /MD
	SET MODE_DEFINES=/DNDEBUG /DRG_ASSERT_ENABLED=1 /DRGD3_DEV_BUILD=1
	SET LINK_EXTRA=/DEBUG:FULL
) ELSE IF /I "%BUILD_MODE%"=="release" (
	SET PROJECT_NAME=rgd3
	SET MODE_FLAGS=/O2 /Ob1 /MD
	SET MODE_DEFINES=/DNDEBUG /DRGD3_RELEASE_BUILD=1
) ELSE IF /I "%BUILD_MODE%"=="profile" (
	SET MODE_FLAGS=/O2 /Ob1 /Z7 /Zo /MD
	SET MODE_DEFINES=/DNDEBUG /DRGD3_PROFILE_BUILD=1
	SET LINK_EXTRA=/DEBUG:FULL /OPT:REF /OPT:ICF
) ELSE (
	ECHO Usage: build.bat [debug^|dev^|release^|profile^|clean] [output.exe]
	GOTO :fail
)
SET OUTPUT_EXE=%~2
IF "%OUTPUT_EXE%"=="" SET OUTPUT_EXE=%PROJECT_NAME%.exe

CALL :ensure_vs_env x64
IF %ERRORLEVEL% NEQ 0 GOTO :fail

IF NOT EXIST obj MKDIR obj
IF NOT EXIST "%OBJECT_DIR%" MKDIR "%OBJECT_DIR%"

IF "%SDL3_DIR%"=="" SET SDL3_DIR=C:\libs\SDL3-3.4.10
IF NOT EXIST "%SDL3_DIR%\include\SDL3\SDL.h" (
	ECHO SDL3 not found at "%SDL3_DIR%". Set SDL3_DIR.
	GOTO :fail
)
CALL "%~dp0setup_rg.bat" core text image
IF ERRORLEVEL 1 GOTO :fail

SET INCLUDE_FLAGS=/Isrc /Ithird_party /I"%SDL3_DIR%\include" /I"%RG_CORE_DIR%\src" /I"%RG_TEXT_DIR%\src" /I"%RG_TEXT_DIR%\examples\assets" /I"%RG_IMAGE_DIR%\src" /I"%RG_IMAGE_DIR%\third_party"
SET LINK_FLAGS=/link /subsystem:console /STACK:8388608 /LIBPATH:"%SDL3_DIR%\lib\x64" SDL3.lib advapi32.lib %LINK_EXTRA%

ECHO Building %OUTPUT_EXE% [%BUILD_MODE%]...
cl %COMMON_FLAGS% %MODE_FLAGS% %COMMON_DEFINES% %MODE_DEFINES% %INCLUDE_FLAGS% src\all.c /Fo"%OBJECT_DIR%\\" /Fe"%OUTPUT_EXE%" %LINK_FLAGS%
IF %ERRORLEVEL% NEQ 0 (
	ECHO ERROR: build failed, error code %ERRORLEVEL%
	GOTO :fail
)
COPY /Y "%SDL3_DIR%\lib\x64\SDL3.dll" . >NUL
ECHO %OUTPUT_EXE% build success.
POPD >NUL
EndLocal
EXIT /B 0

:clean
IF EXIST obj RMDIR /S /Q obj
DEL /Q rgd3*.exe rgd3*.pdb rgd3*.ilk 2>NUL
ECHO Clean complete.
POPD >NUL
EndLocal
EXIT /B 0

:fail
POPD >NUL
EndLocal
EXIT /B 1

:ensure_vs_env
SET "REQ_ARCH=%~1"
IF /I "!VSCMD_ARG_TGT_ARCH!"=="!REQ_ARCH!" EXIT /B 0
SET "HOST_ARCH=x86"
IF /I "%PROCESSOR_ARCHITECTURE%"=="AMD64" SET "HOST_ARCH=x64"
SET "VSDEV="
IF DEFINED VSINSTALLDIR (
	IF EXIST "!VSINSTALLDIR!\Common7\Tools\VsDevCmd.bat" SET "VSDEV=!VSINSTALLDIR!\Common7\Tools\VsDevCmd.bat"
)
IF NOT DEFINED VSDEV (
	SET "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
	IF EXIST "!VSWHERE!" (
		FOR /F "usebackq delims=" %%i IN (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) DO (
			SET "VSDEV=%%i\Common7\Tools\VsDevCmd.bat"
		)
	)
)
IF DEFINED VSDEV (
	PUSHD "%TEMP%" >NUL
	CALL "!VSDEV!" -arch=!REQ_ARCH! -host_arch=!HOST_ARCH! >NUL
	POPD >NUL
)
IF /I "!VSCMD_ARG_TGT_ARCH!"=="!REQ_ARCH!" EXIT /B 0
ECHO Failed to configure MSVC !REQ_ARCH! toolchain.
EXIT /B 1
