#requires -Version 7

<#
.SYNOPSIS
    安装 Visual Studio 2022 环境到 PowerShell 7 配置文件

.DESCRIPTION
    将 VS 2022 开发环境自动加载代码添加到 PowerShell 7 配置文件中

.EXAMPLE
    .\install-vs-profile.ps1
#>

param(
    [string]$VcvarsPath = ""
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "╔════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  VS 2022 PowerShell 7 环境安装器       ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

$profilePath = $PROFILE.CurrentUserCurrentHost

Write-Host "配置文件路径: $profilePath" -ForegroundColor Yellow
Write-Host ""

# 自动检测 VS 2022 路径
if ([string]::IsNullOrEmpty($VcvarsPath) -or -not (Test-Path $VcvarsPath)) {
    $possiblePaths = @(
        "D:\Program Files\Microsoft\2022\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($path in $possiblePaths) {
        if (Test-Path $path) {
            $VcvarsPath = $path
            break
        }
    }
}

if (-not (Test-Path $VcvarsPath)) {
    Write-Host "错误: 找不到 Visual Studio 2022 vcvars64.bat" -ForegroundColor Red
    Write-Host ""
    Write-Host "请手动指定路径:" -ForegroundColor Yellow
    Write-Host "  .\install-vs-profile.ps1 -VcvarsPath 'D:\path\to\vcvars64.bat'" -ForegroundColor Cyan
    exit 1
}

Write-Host "✓ 找到 VS 2022: $VcvarsPath" -ForegroundColor Green

# 自动检测项目信息
$projectRoot = $PSScriptRoot
$projectName = ""

# 检查 CMakeLists.txt 获取项目名
$cmakeLists = Join-Path $projectRoot "CMakeLists.txt"
if (Test-Path $cmakeLists) {
    $cmakeContent = Get-Content $cmakeLists -Raw
    if ($cmakeContent -match 'project\((\w+)\s') {
        $projectName = $matches[1]
    }
}

Write-Host "✓ 项目: $projectName ($projectRoot)" -ForegroundColor Green
Write-Host ""

# 创建配置文件目录
$profileDir = Split-Path -Parent $profilePath
if (-not (Test-Path $profileDir)) {
    New-Item -ItemType Directory -Path $profileDir -Force | Out-Null
}

# 检查是否已安装
if (Test-Path $profilePath) {
    $profileContent = Get-Content $profilePath -Raw -Encoding UTF8

    if ($profileContent -match "VS-Dev-Environment") {
        Write-Host "⚠️  VS 环境已安装，是否重新安装？" -ForegroundColor Yellow
        $confirm = Read-Host " (Y/N)"

        if ($confirm -ne "Y" -and $confirm -ne "y") {
            Write-Host "已取消" -ForegroundColor Yellow
            exit 0
        }

        $backupPath = "$profilePath.backup"
        Copy-Item $profilePath $backupPath -Force
        Write-Host "✓ 已备份原配置到: $backupPath" -ForegroundColor Green

        $profileContent = $profileContent -replace "#Region VS-Dev-Environment[\s\S]*?#EndRegion", ""
        $profileContent = $profileContent.Trim()
        Set-Content $profilePath $profileContent -Encoding UTF8 -NoNewline
    }
}

# 生成配置内容
$vsEnvironmentCode = @'

#Region VS-Dev-Environment
# ========================================
# VS 2022 开发环境 - 自动加载
# 安装时间: INSTALL_DATE_PLACEHOLDER
# ========================================
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " 加载 VS 2022 开发环境..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$vcvarsPath = "VCVARSPATH_PLACEHOLDER"

if (Test-Path $vcvarsPath) {
    $tempCmd = Join-Path $env:TEMP "vs-env-temp.bat"
    $tempEnv = Join-Path $env:TEMP "vs-env-temp.txt"

    $batContent = @"
@echo off
call "$vcvarsPath" >nul 2>&1
set
"@

    Set-Content -Path $tempCmd -Value $batContent -Encoding ASCII

    # 执行批处理并将输出重定向到文件
    & cmd /c "`"$tempCmd`" > `"$tempEnv`" 2>&1"

    # 读取并设置环境变量
    if (Test-Path $tempEnv) {
        Get-Content $tempEnv | ForEach-Object {
            if ($_ -match '^([^=]+)=(.*)$') {
                $name = $matches[1]
                $value = $matches[2]
                [Environment]::SetEnvironmentVariable($name, $value, "Process")
            }
        }
        Remove-Item $tempEnv -Force -ErrorAction SilentlyContinue
    }

    Remove-Item $tempCmd -Force -ErrorAction SilentlyContinue

    Write-Host "✓ VS 2022 环境已加载" -ForegroundColor Green

    # 显示 VS 信息
    if ($env:VCToolsInstallDir) {
        Write-Host "  VC Tools: $env:VCToolsInstallDir" -ForegroundColor DarkGray
    }
    if ($env:WindowsSDKDir) {
        Write-Host "  Windows SDK: $env:WindowsSDKDir" -ForegroundColor DarkGray
    }

    # 验证关键工具
    $clPath = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($clPath) {
        Write-Host "  MSVC: $($clPath.Source)" -ForegroundColor DarkGray
    }
} else {
    Write-Host "⚠️  未找到 VS 2022: $vcvarsPath" -ForegroundColor Yellow
}

#Region 通用构建命令
function global:Invoke-Build {
    param(
        [Parameter(Position = 0)]
        [string]$TargetOrConfig = "",
        [string]$Config = ""
    )

    $buildDir = "build"

    # 如果 build 目录不存在，自动配置
    if (-not (Test-Path $buildDir)) {
        Write-Host "build 目录不存在，正在自动配置..." -ForegroundColor Yellow
        cmake -B build -DCMAKE_BUILD_TYPE=Debug -GNinja
        if ($LASTEXITCODE -ne 0) {
            Write-Host "✗ CMake 配置失败" -ForegroundColor Red
            return 1
        }
    }

    $target = ""
    $config = "Debug"
    $configPatterns = @("Debug", "Release", "RelWithDebInfo", "MinSizeSize")

    if ($TargetOrConfig -in $configPatterns) {
        $config = $TargetOrConfig
    } elseif ($TargetOrConfig) {
        $target = $TargetOrConfig
    }

    if ($Config -and $Config -in $configPatterns) {
        $config = $Config
    }

    $argsList = @("--build", ".", "-j")

    if ($target) {
        $argsList += "--target", $target
    }

    $argsList += "--config", $config

    $displayTarget = if ($target) { $target } else { "所有目标" }
    Write-Host "构建: $displayTarget ($config)" -ForegroundColor Cyan

    Push-Location $buildDir
    cmake $argsList
    $result = $LASTEXITCODE
    Pop-Location

    if ($result -eq 0) {
        Write-Host "✓ 构建成功" -ForegroundColor Green
    } else {
        Write-Host "✗ 构建失败" -ForegroundColor Red
    }

    return $result
}

function global:Invoke-ReBuild {
    param(
        [Parameter(Position = 0)]
        [string]$Config = "Debug"
    )

    Write-Host "清理并重新构建..." -ForegroundColor Cyan

    if (Test-Path "build") {
        Remove-Item -Recurse -Force build
        Write-Host "✓ 已清理 build 目录" -ForegroundColor Green
    }

    cmake -B build -DCMAKE_BUILD_TYPE=$Config -GNinja
    if ($LASTEXITCODE -eq 0) {
        Invoke-Build -Config $Config
    }
}

function global:Invoke-RunOutput {
    param([string]$TargetName = "")

    $exePaths = @(
        "build\Debug\bin\$TargetName.exe",
        "build\Release\bin\$TargetName.exe",
        "build\bin\$TargetName.exe",
        "build\Debug\$TargetName.exe",
        "build\Release\$TargetName.exe",
        "build\Debug\bin\*.exe",
        "build\Release\bin\*.exe",
        "build\bin\*.exe",
        "build\Debug\*.exe",
        "build\Release\*.exe"
    )

    $found = $false
    foreach ($pattern in $exePaths) {
        $exes = Get-ChildItem $pattern -ErrorAction SilentlyContinue
        if ($exes) {
            $exe = $exes[0].FullName
            Write-Host "运行: $exe" -ForegroundColor Cyan
            & $exe
            $found = $true
            break
        }
    }

    if (-not $found) {
        Write-Host "错误: 未找到可执行文件" -ForegroundColor Red
        Write-Host "请先运行构建命令" -ForegroundColor Yellow
    }
}

# 创建全局别名
Set-Alias -Scope Global -Name build -Value Invoke-Build -Force
Set-Alias -Scope Global -Name rebuild -Value Invoke-ReBuild -Force
Set-Alias -Scope Global -Name run -Value Invoke-RunOutput -Force

Write-Host "📦 通用构建命令已加载" -ForegroundColor Cyan
Write-Host "  build [目标名|配置]  - 构建项目" -ForegroundColor DarkGray
Write-Host "  rebuild [配置]        - 清理并重新构建" -ForegroundColor DarkGray
Write-Host "  run [目标名]          - 运行构建输出" -ForegroundColor DarkGray
#EndRegion

#Region 当前项目快捷命令
$script:projectRoot = "PROJECTROOT_PLACEHOLDER"
$script:projectName = "PROJECTNAME_PLACEHOLDER"

function global:cd-project { Set-Location $script:projectRoot }

function global:build-project {
    param([string]$Config = "Debug")
    Push-Location $script:projectRoot
    if ($script:projectName) {
        Invoke-Build -TargetOrConfig $script:projectName -Config $Config
    } else {
        Invoke-Build -Config $Config
    }
    Pop-Location
}

function global:run-project {
    $exe = "$script:projectRoot\build\Debug\bin\$script:projectName.exe"
    if (Test-Path $exe) {
        & $exe
    } else {
        Write-Host "$script:projectName.exe 不存在，请先运行 build-project" -ForegroundColor Red
    }
}

# 创建项目别名
Set-Alias -Scope Global -Name cdproj -Value cd-project -Force
Set-Alias -Scope Global -Name bp -Value build-project -Force
Set-Alias -Scope Global -Name rp -Value run-project -Force

# 创建项目特定的函数和别名（如果项目名存在）
if ($script:projectName) {
    $projectFuncName = "build-" + $script:projectName.ToLower()
    $projectRunFuncName = "run-" + $script:projectName.ToLower()

    # 创建项目特定的函数
    $functionDef = @"
param(`$Config = "Debug")
Push-Location `$script:projectRoot
Invoke-Build -TargetOrConfig `$script:projectName -Config `$Config
Pop-Location
"@
    New-Item -Path "function:\global:$projectFuncName" -Value $functionDef -Force | Out-Null

    $runFunctionDef = @"
`$exe = "`$(`$script:projectRoot)\build\Debug\bin\`$(`$script:projectName).exe"
if (Test-Path `$exe) { & `$exe }
else { Write-Host "`$(`$script:projectName).exe 不存在" -ForegroundColor Red }
"@
    New-Item -Path "function:\global:$projectRunFuncName" -Value $runFunctionDef -Force | Out-Null

    Write-Host "📦 项目快捷命令已加载 (`$script:projectName)" -ForegroundColor Cyan
    Write-Host "  cdproj            - 切换到项目目录" -ForegroundColor DarkGray
    Write-Host "  build-project      - 编译项目" -ForegroundColor DarkGray
    Write-Host "  run-project        - 运行项目" -ForegroundColor DarkGray
    Write-Host "  $projectFuncName  - 编译 `$script:projectName" -ForegroundColor DarkGray
    Write-Host "  $projectRunFuncName - 运行 `$script:projectName" -ForegroundColor DarkGray
} else {
    Write-Host "📦 项目快捷命令已加载" -ForegroundColor Cyan
    Write-Host "  cdproj       - 切换到项目目录" -ForegroundColor DarkGray
    Write-Host "  build-project - 编译项目" -ForegroundColor DarkGray
}
#EndRegion

Write-Host ""
#EndRegion

'@.Replace("VCVARSPATH_PLACEHOLDER", $VcvarsPath).Replace("PROJECTROOT_PLACEHOLDER", $projectRoot).Replace("PROJECTNAME_PLACEHOLDER", $projectName).Replace("INSTALL_DATE_PLACEHOLDER", (Get-Date -Format "yyyy-MM-dd HH:mm:ss"))

# 添加到配置文件
Add-Content -Path $profilePath -Value $vsEnvironmentCode -Encoding UTF8

Write-Host ""
Write-Host "✓ 安装完成!" -ForegroundColor Green
Write-Host ""
Write-Host "配置文件: $profilePath" -ForegroundColor Yellow
Write-Host ""
Write-Host "重新启动 PowerShell 7 后，VS 2022 环境将自动加载" -ForegroundColor Cyan
Write-Host ""
Write-Host "通用命令:" -ForegroundColor Cyan
Write-Host "  build              - 构建所有目标" -ForegroundColor White
Write-Host "  build ChatManager  - 构建指定目标" -ForegroundColor White
Write-Host "  build Release      - 构建 Release 版本" -ForegroundColor White
Write-Host "  rebuild            - 清理并重新构建" -ForegroundColor White
Write-Host "  run                - 运行构建输出" -ForegroundColor White
if ($projectName) {
    Write-Host ""
    Write-Host "项目命令 ($projectName):" -ForegroundColor Cyan
    Write-Host "  cdproj         - 切换到项目目录" -ForegroundColor White
    Write-Host "  build-project  - 编译项目" -ForegroundColor White
    Write-Host "  run-project    - 运行项目" -ForegroundColor White
    Write-Host "  build-$projectName - 编译 $projectName" -ForegroundColor White
    Write-Host "  run-$projectName   - 运行 $projectName" -ForegroundColor White
    Write-Host "  bp             - build-project 简写" -ForegroundColor White
    Write-Host "  rp             - run-project 简写" -ForegroundColor White
}
Write-Host ""
Write-Host "卸载方式:" -ForegroundColor Yellow
Write-Host "  .\uninstall-vs-profile.ps1" -ForegroundColor DarkGray
Write-Host ""
