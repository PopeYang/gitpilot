# Qt Git客户端 - 自动构建脚本
# Qt 6.8.3 + MSVC 2022

# 配置参数
$QtPath = "D:\qt\6.8.3\msvc2022_64"
$BuildType = "Release"
$ProjectRoot = $PSScriptRoot

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  GitPilot 客户端 - 自动构建" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 1. 检查Qt路径
Write-Host "[1/5] 检查Qt安装..." -ForegroundColor Yellow
if (Test-Path "$QtPath\bin\qmake.exe") {
    Write-Host "  ✓ Qt路径有效: $QtPath" -ForegroundColor Green
} else {
    Write-Host "  ✗ Qt路径无效，请检查安装路径" -ForegroundColor Red
    exit 1
}

# 2. 检查MSVC环境
Write-Host "[2/5] 检查MSVC编译器..." -ForegroundColor Yellow
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath

if ($vsPath) {
    Write-Host "  ✓ 找到Visual Studio: $vsPath" -ForegroundColor Green
    
    # 导入MSVC环境变量
    $vcvarsPath = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
    if (Test-Path $vcvarsPath) {
        Write-Host "  ✓ 正在加载MSVC 2022环境..." -ForegroundColor Green
        cmd /c "`"$vcvarsPath`" && set" | ForEach-Object {
            if ($_ -match "^(.*?)=(.*)$") {
                Set-Content "env:\$($matches[1])" $matches[2]
            }
        }
    }
} else {
    Write-Host "  ✗ 未找到Visual Studio 2022，请先安装" -ForegroundColor Red
    exit 1
}

# 3. 清理旧构建
Write-Host "[3/5] 清理旧构建目录..." -ForegroundColor Yellow
$BuildDir = Join-Path $ProjectRoot "build"
if (Test-Path $BuildDir) {
    Remove-Item -Recurse -Force $BuildDir
    Write-Host "  ✓ 已删除旧构建" -ForegroundColor Green
}
New-Item -ItemType Directory -Path $BuildDir | Out-Null

# 4. CMake配置
Write-Host "[4/5] 运行CMake配置..." -ForegroundColor Yellow
Set-Location $BuildDir

$cmakeArgs = @(
    ".."
    "-DCMAKE_PREFIX_PATH=$QtPath"
    "-DCMAKE_BUILD_TYPE=$BuildType"
    "-G", "Visual Studio 17 2022"
    "-A", "x64"
)

& cmake $cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✗ CMake配置失败" -ForegroundColor Red
    Set-Location $ProjectRoot
    exit 1
}

Write-Host "  ✓ CMake配置成功" -ForegroundColor Green

# 5. 编译项目
Write-Host "[5/5] 编译项目..." -ForegroundColor Yellow
& cmake --build . --config $BuildType --parallel

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✗ 编译失败" -ForegroundColor Red
    Set-Location $ProjectRoot
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  ✓ 编译成功！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "可执行文件位置:" -ForegroundColor Cyan
Write-Host "  $BuildDir\$BuildType\gitpilot.exe" -ForegroundColor White
Write-Host ""
Write-Host "运行程序:" -ForegroundColor Cyan
Write-Host "  cd build\$BuildType" -ForegroundColor White
Write-Host "  .\gitpilot.exe" -ForegroundColor White
Write-Host ""

# 询问是否立即运行
$run = Read-Host "是否立即运行程序? (y/n)"
if ($run -eq 'y' -or $run -eq 'Y') {
    Set-Location "$BuildDir\$BuildType"
    
    # 添加Qt DLL到PATH（运行时需要）
    $env:Path = "$QtPath\bin;" + $env:Path
    
    Start-Process "gitpilot.exe"
    Write-Host "  ✓ 程序已启动" -ForegroundColor Green
}

Set-Location $ProjectRoot
