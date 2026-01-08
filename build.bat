@echo off
REM 批处理版构建脚本（适用于不能运行PowerShell的环境）

echo ========================================
echo   Git客户端 - 构建脚本
echo ========================================
echo.

set QT_PATH=D:\qt\6.8.3\msvc2022_64

echo [1/3] 清理构建目录...
if exist build rmdir /s /q build
mkdir build
cd build

echo [2/3] CMake配置...
cmake .. -DCMAKE_PREFIX_PATH=%QT_PATH% -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto error

echo [3/3] 编译项目...
cmake --build . --config Release
if errorlevel 1 goto error

echo.
echo ========================================
echo   编译成功！
echo ========================================
echo.
echo 可执行文件: build\Release\gitpilot.exe
echo.
goto end

:error
echo.
echo ========================================
echo   编译失败！
echo ========================================
echo.
pause
exit /b 1

:end
pause
