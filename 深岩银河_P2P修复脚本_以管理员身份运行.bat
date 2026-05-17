@echo off
setlocal enabledelayedexpansion

echo =============================================
echo           深岩银河 P2P 连接修复脚本
echo =============================================
echo.

:: ==============================
:: 1. 获取 Steam 安装路径
:: ==============================
echo [1/3] 正在获取 Steam 安装路径...

set "STEAM_ROOT="

for /f "skip=2 tokens=2*" %%a in ('reg query "HKLM\SOFTWARE\Wow6432Node\Valve\Steam" /v "InstallPath" 2^>nul') do set "STEAM_ROOT=%%b"

if not defined STEAM_ROOT (
    echo 发生错误：注册表中未找到 Steam 安装路径
	echo.
    pause
    exit /b 1
)

echo Steam 安装路径 %STEAM_ROOT%
echo.

:: ==============================
:: 2. 获取深岩银河安装路径
:: ==============================
set "APPID=548430"
set "VDF_FILE=%STEAM_ROOT%\steamapps\libraryfolders.vdf"

echo [2/3] 正在获取深岩银河安装路径...

if not exist "%VDF_FILE%" (
    echo 发生错误：找不到 libraryfolders.vdf 文件
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
    echo 发生错误：所有 Steam 库目录下均未找到 AppID: %APPID% 的安装记录
	echo.
    goto :end
)

set "ACF_FILE=%TARGET_LIB_PATH%\steamapps\appmanifest_%APPID%.acf"
set "INSTALL_DIR="

if not exist "%ACF_FILE%" (
    echo 发生错误：未找到找到 %ACF_FILE%
	echo.
    goto :end
)

for /f "tokens=1,*" %%C in ('type "%ACF_FILE%" ^| findstr /i /c:"\"installdir\""') do (
    set "INSTALL_DIR=%%~D"
)

if "%INSTALL_DIR%"=="" (
    echo 发生错误：无法在 acf 文件中读取 installdir
	echo.
    goto :end
)

set "FINAL_PATH=%TARGET_LIB_PATH%\steamapps\common\%INSTALL_DIR%"

echo 游戏安装路径 %FINAL_PATH%
echo.

:: ==============================
:: 3. 检查并复制文件
:: ==============================
echo [3/3] 正在检查并复制文件...

set "sourceFile=%STEAM_ROOT%\steamwebrtc64.dll"
set "targetFolder=%FINAL_PATH%\FSD\Binaries\Win64"
echo 正在复制到 %targetFolder%
copy /Y "%sourceFile%" "%targetFolder%"

if errorlevel 1 (
    echo 发生错误：复制文件到 %targetFolder% 时出错
	echo.
    pause
	exit /b 1
)

echo.
echo =============================================
echo                   操作完成
echo =============================================
echo.
pause