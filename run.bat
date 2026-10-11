@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo ========================================
echo   长征 · 从烽火到星辰
echo   策略叙事游戏
echo ========================================
echo.

REM 检查 Java 是否可用
where java >nul 2>&1
if errorlevel 1 (
    echo [错误] 未检测到 Java，请安装 JDK 后重试。
    pause
    exit /b 1
)

REM 如果没有编译过，先编译
if not exist Game.class (
    echo 正在编译 Java 前端...
    javac -encoding UTF-8 Game.java
    if errorlevel 1 (
        echo [错误] Java 编译失败。
        pause
        exit /b 1
    )
)

REM 检查引擎：优先 C++ 编译版，否则用 Python 版
if exist engine.exe (
    echo 使用 C++ 编译版游戏引擎 (engine.exe)
) else (
    where python >nul 2>&1
    if errorlevel 1 (
        echo [错误] 未检测到 engine.exe，且未安装 Python。
        echo        请双击 compile_engine.bat 编译 C++ 引擎，或安装 Python。
        pause
        exit /b 1
    )
    echo 使用 Python 版游戏引擎 (engine.py)
)

echo.
echo 启动游戏...
java Game
pause
