@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo === 编译 C++ 游戏引擎 (MSVC) ===
echo.

REM 通过 vswhere 自动查找 Visual Studio 安装路径
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
)

if "%VSPATH%"=="" (
    echo [错误] 未找到安装了 C++ 工具链的 Visual Studio。
    echo        请在 Visual Studio Installer 中安装 "Desktop development with C++" 工作负载。
    pause
    exit /b 1
)

echo 使用 Visual Studio: %VSPATH%
echo.

REM 配置 MSVC 编译环境并编译
call "%VSPATH%\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /EHsc /std:c++17 /utf-8 /O2 engine.cpp /Fe:engine.exe /link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
if errorlevel 1 (
    echo.
    echo [编译失败] 请检查上方错误信息。
    pause
    exit /b 1
) else (
    echo.
    echo [编译成功] engine.exe
    del engine.obj 2>nul
)
pause
