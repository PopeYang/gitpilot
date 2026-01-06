# 快速构建脚本（简化版）
# 直接使用Developer PowerShell for VS 2022运行

$QtPath = "D:\qt\6.8.3\msvc2022_64"

Write-Host "开始构建..." -ForegroundColor Cyan

# 清理并创建构建目录
if (Test-Path build) { Remove-Item -Recurse -Force build }
New-Item -ItemType Directory -Path build | Out-Null
Set-Location build

# CMake配置
cmake .. -DCMAKE_PREFIX_PATH="$QtPath" -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build . --config Release

Write-Host "构建完成！可执行文件: build\Release\gitgui.exe" -ForegroundColor Green
