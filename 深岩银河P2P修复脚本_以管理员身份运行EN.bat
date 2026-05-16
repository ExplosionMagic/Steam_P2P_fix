@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo =================================================
echo    Deep Rock Galactic P2P Connection Fix Script
echo =================================================
echo.

:: ==============================
:: 1. Get Steam Installation Path
:: ==============================
echo [1/3] Getting Steam installation path...

set "STEAM_ROOT="

for /f "skip=2 tokens=2*" %%a in ('reg query "HKLM\SOFTWARE\Wow6432Node\Valve\Steam" /v "InstallPath" 2^>nul') do set "STEAM_ROOT=%%b"

if not defined STEAM_ROOT (
    echo Error: Steam installation path not found in the registry.
    pause
    exit /b 1
)

echo Steam installation path %STEAM_ROOT%
echo.

:: ==============================
:: 2. Get Deep Rock Galactic Installation Path
:: ==============================
set "APPID=548430"
set "VDF_FILE=%STEAM_ROOT%\steamapps\libraryfolders.vdf"

echo [2/3] Getting Deep Rock Galactic installation path...

if not exist "%VDF_FILE%" (
    echo Error: libraryfolders.vdf file not found.
	echo.
    goto :end
)

set "TARGET_LIB_PATH="
set "CURRENT_PATH="

for /f "tokens=1,2*" %%A in ('type "%VDF_FILE%" ^| findstr /i /c:"\"path\"" /c:"\"%APPID%\""') do (
    
    if /I "%%~A"=="path" (
        set "CURRENT_PATH=%%~B"
        set "CURRENT_PATH=!CURRENT_PATH:\\=\!"
    )

    if /I "%%~A"=="%APPID%" (
        set "TARGET_LIB_PATH=!CURRENT_PATH!"
        goto :found_lib
    )
)

:found_lib
if "%TARGET_LIB_PATH%"=="" (
    echo Error: Installation record for AppID: %APPID% not found in any Steam library directory.
	echo.
    goto :end
)

set "ACF_FILE=%TARGET_LIB_PATH%\steamapps\appmanifest_%APPID%.acf"
set "INSTALL_DIR="

if not exist "%ACF_FILE%" (
    echo Error: %ACF_FILE% not found.
	echo.
    goto :end
)

for /f "tokens=1,*" %%C in ('type "%ACF_FILE%" ^| findstr /i /c:"\"installdir\""') do (
    set "INSTALL_DIR=%%~D"
)

if "%INSTALL_DIR%"=="" (
    echo Error: Cannot read installdir from the acf file.
	echo.
    goto :end
)

set "FINAL_PATH=%TARGET_LIB_PATH%\steamapps\common\%INSTALL_DIR%"

echo Game installation path %FINAL_PATH%
echo.

:: ==============================
:: 3. Check and Copy File
:: ==============================
echo [3/3] Checking and copying file...

set "sourceFile=%STEAM_ROOT%\steamwebrtc64.dll"
set "targetFolder=%FINAL_PATH%\FSD\Binaries\Win64"

copy /Y "%sourceFile%" "%targetFolder%"

if errorlevel 1 (
    echo Error: Failed to copy the file.
	echo.
    pause
    exit /b 1
)
echo.
echo =================================================
echo               Operation completed.
echo =================================================
echo.
pause