@echo off
setlocal enabledelayedexpansion

:menu
cls
echo =============================================
echo          Steam 游戏 P2P 连接修复脚本
echo =============================================
echo.
echo 请选择需要修复的游戏:
echo [1] 深岩银河 (Deep Rock Galactic)
echo [2] 战锤：末世鼠疫2 (Warhammer: Vermintide 2)
echo [3] 雨中冒险2 (Risk of Rain 2)
echo [4] 遥遥西土 (Far Far West)
echo [5] 街霸6 (Street Fighter? 6)
echo [0] 退出脚本
echo.
set /p "choice=请输入数字选择 (0-5): "

if "%choice%"=="1" (
    set "GAME_NAME=深岩银河"
    set "APPID=548430"
    goto :start_fix
) else if "%choice%"=="2" (
    set "GAME_NAME=战锤：末世鼠疫2"
    set "APPID=552500"
    goto :start_fix
) else if "%choice%"=="3" (
    set "GAME_NAME=雨中冒险2"
    set "APPID=632360"
    goto :start_fix	
) else if "%choice%"=="4" (
    set "GAME_NAME=遥遥西土"
    set "APPID="3124540"
    goto :start_fix	
) else if "%choice%"=="5" (
    set "GAME_NAME=街霸6"
    set "APPID="1364780"
    goto :start_fix	
) else if "%choice%"=="0" (
    exit /b 0
) else (
    echo.
    echo 发生错误：无效的输入，请重新选择！
    pause
    goto :menu
)

:start_fix
cls
echo =============================================
echo          当前正在修复：%GAME_NAME%
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
    goto :end
)

echo Steam 安装路径 %STEAM_ROOT%
echo.

:: ==============================
:: 2. 获取游戏安装路径
:: ==============================
set "VDF_FILE=%STEAM_ROOT%\steamapps\libraryfolders.vdf"

echo [2/3] 正在获取 %GAME_NAME% 安装路径...

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
    echo 发生错误：未找到 %ACF_FILE%
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

if "%APPID%"=="548430" (
    :: ========== 深岩银河的复制逻辑 ==========
    set "targetFolder=%FINAL_PATH%\FSD\Binaries\Win64"
    echo 正在复制到：!targetFolder!
    copy /Y "%sourceFile%" "!targetFolder!"
    
    if errorlevel 1 (
        echo 发生错误：复制文件到 !targetFolder! 时出错
        echo.
        goto :end
    )
) else if "%APPID%"=="552500" (
    :: ========== 鼠疫2的复制逻辑 ==========
    set "targetFolder1=%FINAL_PATH%\binaries"
    set "targetFolder2=%FINAL_PATH%\binaries_dx12"
    echo 正在复制到 !targetFolder1!
    copy /Y "%sourceFile%" "!targetFolder1!"

    if errorlevel 1 (
        echo 发生错误：复制文件到 !targetFolder1! 时出错
        echo.
        goto :end
    )

    echo 正在复制到 !targetFolder2!
    copy /Y "%sourceFile%" "!targetFolder2!"

    if errorlevel 1 (
        echo 发生错误：复制文件到 !targetFolder2! 时出错
        echo.
        goto :end
    )
) else if "%APPID%"=="632360" (
 :: ========== 雨中冒险2的复制逻辑 ==========
    set "targetFolder3=%FINAL_PATH%"
    echo 正在复制到 !targetFolder3!
    copy /Y "%sourceFile%" "!targetFolder3!"

    if errorlevel 1 (
        echo 发生错误：复制文件到 !targetFolder3! 时出错
        echo.
        goto :end
    )
) else if "%APPID%"=="3124540" (
 :: ========== 遥遥西土的复制逻辑 ==========
    set "targetFolder4=%FINAL_PATH%\FarFarWest\Binaries\Win64"
    echo 正在复制到 !targetFolder4!
    copy /Y "%sourceFile%" "!targetFolder4!"

    if errorlevel 1 (
        echo 发生错误：复制文件到 !targetFolder4! 时出错
        echo.
        goto :end
    )
) else if "%APPID%"=="1364780" (
 :: ========== Street Fighter 6 的复制逻辑 ==========
    set "targetFolder5=%FINAL_PATH%"
    echo 正在复制到 !targetFolder5!
    copy /Y "%sourceFile%" "!targetFolder5!"

    if errorlevel 1 (
        echo 发生错误：复制文件到 !targetFolder5! 时出错
        echo.
        goto :end
    )
)

echo.
echo =============================================
echo                   操作完成
echo =============================================
echo.

:end
:: 操作完成或报错后，暂停查看结果，然后清空变量并返回主菜单
pause
set "GAME_NAME="
set "APPID="
set "STEAM_ROOT="
set "TARGET_LIB_PATH="
set "INSTALL_DIR="
set "FINAL_PATH="
goto :menu