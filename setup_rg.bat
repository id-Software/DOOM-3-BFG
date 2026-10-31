@ECHO OFF
REM Resolves the public rg_* sibling checkouts. Called inside the caller's
REM SETLOCAL so RG_*_DIR overrides propagate. Pattern from boomer\setup_rg.bat.
IF NOT DEFINED RG_CORE_DIR SET "RG_CORE_DIR=%~dp0..\rg_core"
IF NOT DEFINED RG_TEXT_DIR SET "RG_TEXT_DIR=%~dp0..\rg_text"
IF NOT DEFINED RG_GUI_DIR SET "RG_GUI_DIR=%~dp0..\rg_gui"
IF NOT DEFINED RG_IMAGE_DIR SET "RG_IMAGE_DIR=%~dp0..\rg_image"
IF NOT DEFINED RG_AUDIO_DIR SET "RG_AUDIO_DIR=%~dp0..\rg_audio"
FOR %%I IN ("%RG_CORE_DIR%") DO SET "RG_CORE_DIR=%%~fI"
FOR %%I IN ("%RG_TEXT_DIR%") DO SET "RG_TEXT_DIR=%%~fI"
FOR %%I IN ("%RG_GUI_DIR%") DO SET "RG_GUI_DIR=%%~fI"
FOR %%I IN ("%RG_IMAGE_DIR%") DO SET "RG_IMAGE_DIR=%%~fI"
FOR %%I IN ("%RG_AUDIO_DIR%") DO SET "RG_AUDIO_DIR=%%~fI"

FOR %%I IN (%*) DO (
	CALL :check %%I
	IF ERRORLEVEL 1 EXIT /B 1
)
EXIT /B 0

:check
IF /I "%~1"=="core" GOTO core
IF /I "%~1"=="text" GOTO text
IF /I "%~1"=="gui" GOTO gui
IF /I "%~1"=="image" GOTO image
IF /I "%~1"=="audio" GOTO audio
ECHO Unknown rg dependency: %~1
EXIT /B 1

:core
CALL :require "%RG_CORE_DIR%\src\rg_defs.h" RG_CORE_DIR
EXIT /B %ERRORLEVEL%

:text
CALL :require "%RG_TEXT_DIR%\src\rg_text.h" RG_TEXT_DIR
EXIT /B %ERRORLEVEL%

:gui
CALL :require "%RG_GUI_DIR%\src\rg_gui.h" RG_GUI_DIR
EXIT /B %ERRORLEVEL%

:image
CALL :require "%RG_IMAGE_DIR%\src\rg_rgi.h" RG_IMAGE_DIR
IF ERRORLEVEL 1 EXIT /B 1
CALL :require "%RG_IMAGE_DIR%\third_party\stb_image.h" RG_IMAGE_DIR
EXIT /B %ERRORLEVEL%

:audio
CALL :require "%RG_AUDIO_DIR%\src\rg_rgs.h" RG_AUDIO_DIR
EXIT /B %ERRORLEVEL%

:require
IF EXIST "%~1" EXIT /B 0
ECHO Missing public rg dependency header: "%~1".
ECHO Set %~2 to the corresponding public repository root.
EXIT /B 1
