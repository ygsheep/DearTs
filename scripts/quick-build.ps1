#requires -Version 7

<#
.SYNOPSIS
    ChatManager 快速编译脚本

.DESCRIPTION
    使用 Visual Studio 2022 环境编译 ChatManager 项目

.EXAMPLE
    .\quick-build.ps1
#>

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [switch]$Clean,
    [switch]$Run,
    [switch]$Watch
)

$ErrorActionPreference = "Stop"

# 项目路径
$ProjectRoot = $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build"

# VS 2022 路径
$VsDevCmd = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
$VsDevCmdAlt = "D:\Program Files\Microsoft\2022\VC\Auxiliary\Build\vcvars64.bat"

# 查找 vcvars64.bat
$vcvarsPath = if (Test-Path $VsDevCmd) { $VsDevCmd }
              elseif (Test-Path $VsDevCmdAlt) { $VsDevCmdAlt }
              else { $null }

if (-not $vcvarsPath) {
    Write-Host "错误: 找不到 Visual Studio 2022 vcvars64.bat" -ForegroundColor Red
    Write-Host "请安装 VS 2022 或修改脚本中的路径" -ForegroundColor Yellow
    exit 1
}

# 函数：执行带 VS 环境的命令
function Invoke-VsEnvironment {
    param(
        [scriptblock]$ScriptBlock
    )

    $tempFile = [System.IO.Path]::GetTempFileName()
    $outputFile = [System.IO.Path]::GetTempFileName()

    # 创建临时批处理文件
        @echo off
        setlocal EnableDelayedExpansion
        call "`"$vcvarsPath`""
        set > "`"$outputFile`""
    "@ | Out-File -FilePath "$tempFile.cmd" -Encoding ASCII

    # 执行并获取环境变量
    & cmd /c "`"$tempFile.cmd`"" | Out-Null

    # 读取并应用环境变量
    if (Test-Path $outputFile) {
        Get-Content $outputFile | ForEach-Object {
            if ($_ -match '^([^=]+)=(.*)$') {
                $name = $matches[1]
                $value = $matches[2]
                [Environment]::SetEnvironmentVariable($name, $value, "Process")
            }
        }
        Remove-Item $outputFile -Force
    }

    Remove-Item "$tempFile.cmd" -Force -ErrorAction SilentlyContinue

    # 执行脚本
    & $ScriptBlock
}

# 函数：清理
function Invoke-Clean {
    Write-Host "🧹 清理构建目录..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Host "✓ 清理完成" -ForegroundColor Green
    }
}

# 函数：配置 CMake
function Invoke-Configure {
    Write-Host "⚙️  配置 CMake ($Configuration)..." -ForegroundColor Cyan

    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
    }

    $ cmakeArgs = @(
        "-G", "Ninja"
        "-DCMAKE_BUILD_TYPE=$Configuration"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        $ProjectRoot
    )

    Push-Location $BuildDir
    cmake $cmakeArgs
    $result = $LASTEXITCODE
    Pop-Location

    if ($result -eq 0) {
        Write-Host "✓ 配置完成" -ForegroundColor Green
    }

    return $result
}

# 函数：编译
function Invoke-Build {
    Write-Host "🔨 编译 ChatManager ($Configuration)..." -ForegroundColor Cyan

    $ cmakeArgs = @(
        "--build", "."
        "--target", "ChatManager"
        "--config", $Configuration
        "-j", (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
    )

    Push-Location $BuildDir
    cmake $cmakeArgs
    $result = $LASTEXITCODE
    Pop-Location

    if ($result -eq 0) {
        Write-Host "✓ 编译成功" -ForegroundColor Green
    } else {
        Write-Host "✗ 编译失败 (退出码: $result)" -ForegroundColor Red
    }

    return $result
}

# 函数：运行
function Invoke-Run {
    $exePath = Join-Path $BuildDir "$Configuration\bin\ChatManager.exe"

    if (Test-Path $exePath) {
        Write-Host "🚀 启动 ChatManager..." -ForegroundColor Cyan
        & $exePath
    } else {
        Write-Host "✗ ChatManager.exe 不存在: $exePath" -ForegroundColor Red
        return 1
    }
}

# 函数：监视文件变化并自动重新编译
function Invoke-Watch {
    Write-Host "👀 监视模式启动 (Ctrl+C 退出)..." -ForegroundColor Cyan
    Write-Host ""

    $sourceDirs = @(
        "main\chatmanager",
        "plugins\chat",
        "core"
    ) | ForEach-Object { Join-Path $ProjectRoot $_ }

    $watcher = New-Object System.IO.FileSystemWatcher
    $watcher.Path = $ProjectRoot
    $watcher.IncludeSubdirectories = $true
    $watcher.EnableRaisingEvents = $true

    $changed = $false

    $action = {
        $global:changed = $true
        $path = $Event.SourceEventArgs.FullPath
        $changeType = $Event.SourceEventArgs.ChangeType
        Write-Host "[$(Get-Date -Format 'HH:mm:ss')] 变更: $changeType - $path" -ForegroundColor DarkGray
    }

    $handlers = @(
        Register-ObjectEvent $watcher "Changed" -Action $action,
        Register-ObjectEvent $watcher "Created" -Action $action,
        Register-ObjectEvent $watcher "Deleted" -Action $action,
        Register-ObjectEvent $watcher "Renamed" -Action $action
    )

    try {
        while ($true) {
            if ($global:changed) {
                $global:changed = $false
                Write-Host ""
                Write-Host "检测到变更，重新编译..." -ForegroundColor Yellow
                if (Invoke-Build -eq 0 -and $Run) {
                    Invoke-Run
                }
                Write-Host "等待变更..." -ForegroundColor DarkGray
            }
            Start-Sleep -Milliseconds 500
        }
    }
    finally {
        $handlers | Unregister-Event -ErrorAction SilentlyContinue
        $watcher.Dispose()
    }
}

# 主流程
Write-Host ""
Write-Host "╔════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║    ChatManager 快速编译脚本           ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

Invoke-VsEnvironment {
    if ($Clean) {
        Invoke-Clean
    }

    if (-not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))) {
        Invoke-Configure
    }

    if (Invoke-Build -eq 0 -and $Run) {
        Write-Host ""
        Invoke-Run
    }

    if ($Watch) {
        Invoke-Watch
    }
}
