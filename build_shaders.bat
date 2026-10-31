@ECHO OFF
REM Compiles every *.vert.hlsl / *.frag.hlsl / *.comp.hlsl in shaders\src and the
REM rg_text shader directory into shaders\Compiled\{DXIL,SPIRV,MSL}\<stem>.<ext>.
REM <stem> keeps the .vert/.frag suffix, which is what rg_gpu_shader_load expects.
SetLocal
PUSHD "%~dp0" >NUL
IF "%SHADERCROSS%"=="" SET "SHADERCROSS=C:\libs\SDL3_shadercross-3.0.0-windows-VC-x64\bin\shadercross.exe"
IF NOT EXIST "%SHADERCROSS%" (
	ECHO shadercross not found at "%SHADERCROSS%". Set SHADERCROSS.
	GOTO :fail
)
CALL "%~dp0setup_rg.bat" text
IF ERRORLEVEL 1 GOTO :fail
FOR %%D IN (DXIL SPIRV MSL) DO IF NOT EXIST "shaders\Compiled\%%D" MKDIR "shaders\Compiled\%%D"
SET SHADER_COUNT=0
CALL :dir "shaders\src"
IF ERRORLEVEL 1 GOTO :fail
CALL :dir "%RG_TEXT_DIR%\shaders"
IF ERRORLEVEL 1 GOTO :fail
ECHO Compiled %SHADER_COUNT% shader stages.
POPD >NUL
EndLocal
EXIT /B 0

:dir
FOR %%f IN ("%~1\*.vert.hlsl") DO (
	CALL :one "%%~ff" vertex
	IF ERRORLEVEL 1 EXIT /B 1
)
FOR %%f IN ("%~1\*.frag.hlsl") DO (
	CALL :one "%%~ff" fragment
	IF ERRORLEVEL 1 EXIT /B 1
)
FOR %%f IN ("%~1\*.comp.hlsl") DO (
	CALL :one "%%~ff" compute
	IF ERRORLEVEL 1 EXIT /B 1
)
EXIT /B 0

:one
ECHO   %~n1 [%2]
"%SHADERCROSS%" "%~1" -s HLSL -d DXIL -t %2 -e main -I shaders\src -o "shaders\Compiled\DXIL\%~n1.dxil"
IF ERRORLEVEL 1 EXIT /B 1
"%SHADERCROSS%" "%~1" -s HLSL -d SPIRV -t %2 -e main -I shaders\src -o "shaders\Compiled\SPIRV\%~n1.spv"
IF ERRORLEVEL 1 EXIT /B 1
"%SHADERCROSS%" "%~1" -s HLSL -d MSL -t %2 -e main --msl-version 2.1.0 -I shaders\src -o "shaders\Compiled\MSL\%~n1.msl"
IF ERRORLEVEL 1 EXIT /B 1
SET /A SHADER_COUNT+=1
EXIT /B 0

:fail
POPD >NUL
EndLocal
EXIT /B 1
