# DearTs Framework - CCache 快速安装脚本
# 使用方法：在 PowerShell 中运行
#   .\scripts\install_ccache.ps1

Write-Host "==================================" -ForegroundColor Cyan
Write-Host "  DearTs Framework - CCache 安装脚本" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan
Write-Host ""

# 检查是否已安装
$ccacheInstalled = Get-Command ccache -ErrorAction SilentlyContinue

if ($ccacheInstalled) {
    Write-Host "✓ CCache 已安装: $($ccacheInstalled.Source)" -ForegroundColor Green
    Write-Host ""
    Write-Host "版本信息:" -ForegroundColor Yellow
    ccache --version
    exit 0
}

Write-Host "✗ CCache 未安装，开始安装..." -ForegroundColor Red
Write-Host ""

# 检查管理员权限
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "注意：添加 PATH 需要管理员权限" -ForegroundColor Yellow
    Write-Host "如果脚本失败，请以管理员身份运行 PowerShell" -ForegroundColor Yellow
    Write-Host ""
}

# 创建安装目录
$installDir = "C:\Program Files\ccache"
Write-Host "创建安装目录: $installDir" -ForegroundColor Cyan

if (-not (Test-Path $installDir)) {
    New-Item -ItemType Directory -Force -Path $installDir | Out-Null
    Write-Host "✓ 目录创建成功" -ForegroundColor Green
} else {
    Write-Host "✓ 目录已存在" -ForegroundColor Green
}

# 下载 CCache
Write-Host ""
Write-Host "正在下载 CCache..." -ForegroundColor Cyan

$ccacheVersion = "4.10.2"  # 最新版本
$downloadUrl = "https://github.com/ccache/ccache/releases/download/v${ccacheVersion}/ccache-${ccacheVersion}-windows-x86_64.zip"
$zipFile = "$env:TEMP\ccache.zip"

Write-Host "  下载地址: $downloadUrl" -ForegroundColor Gray

try {
    # 使用 PowerShell 下载（如果没有 Progress 会更快）
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $downloadUrl -OutFile $zipFile -UseBasicParsing
    Write-Host "✓ 下载完成" -ForegroundColor Green
} catch {
    Write-Host "✗ 下载失败: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "请手动下载并解压：" -ForegroundColor Yellow
    Write-Host "  1. 访问: https://github.com/ccache/ccache/releases" -ForegroundColor Gray
    Write-Host "  2. 下载: ccache-${ccacheVersion}-windows-x86_64.zip" -ForegroundColor Gray
    Write-Host "  3. 解压到: $installDir" -ForegroundColor Gray
    Write-Host "  4. 添加到 PATH: $installDir" -ForegroundColor Gray
    exit 1
}

# 解压
Write-Host ""
Write-Host "正在解压..." -ForegroundColor Cyan

try {
    # 使用 PowerShell 5.0+ 的 Expand-Archive
    Expand-Archive -Path $zipFile -DestinationPath $installDir -Force

    # Expand-Archive 会创建子目录，需要移动文件
    $extractedDir = Get-ChildItem -Path $installDir -Directory | Select-Object -First 1
    if ($extractedDir) {
        Move-Item -Path "$($extractedDir.FullName)\*" -Destination $installDir -Force
        Remove-Item -Path $extractedDir.FullName -Force
    }

    Write-Host "✓ 解压完成" -ForegroundColor Green
} catch {
    Write-Host "✗ 解压失败: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "请手动解压到: $installDir" -ForegroundColor Yellow
    exit 1
}

# 清理临时文件
Remove-Item -Path $zipFile -Force -ErrorAction SilentlyContinue

# 添加到 PATH
Write-Host ""
Write-Host "添加到 PATH..." -ForegroundColor Cyan

try {
    $pathEnv = [Environment]::GetEnvironmentVariable("Path", "Machine")

    if ($pathEnv -notlike "*ccache*") {
        if ($isAdmin) {
            [Environment]::SetEnvironmentVariable("Path", $pathEnv + ";$installDir", [EnvironmentVariableTarget]::Machine)
            Write-Host "✓ 已添加到系统 PATH（需要重启终端）" -ForegroundColor Green
        } else {
            Write-Host "⚠ 需要管理员权限添加到系统 PATH" -ForegroundColor Yellow
            Write-Host "  请手动添加: $installDir" -ForegroundColor Gray
            Write-Host ""
            Write-Host "或者以管理员身份运行：" -ForegroundColor Yellow
            Write-Host "  [Environment]::SetEnvironmentVariable('Path', " -ForegroundColor Gray
            Write-Host "    [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';$installDir', " -ForegroundColor Gray
            Write-Host "    [EnvironmentVariableTarget]::Machine" -ForegroundColor Gray
            Write-Host "  )" -ForegroundColor Gray
        }
    } else {
        Write-Host "✓ 已在 PATH 中" -ForegroundColor Green
    }
} catch {
    Write-Host "✗ 添加 PATH 失败: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "请手动添加到 PATH: $installDir" -ForegroundColor Yellow
}

# 为当前会话添加 PATH
$env:Path += ";$installDir"

# 验证安装
Write-Host ""
Write-Host "验证安装..." -ForegroundColor Cyan

$ccacheExe = Get-Command ccache -ErrorAction SilentlyContinue

if ($ccacheExe) {
    Write-Host "✓ CCache 安装成功!" -ForegroundColor Green
    Write-Host ""
    Write-Host "版本信息:" -ForegroundColor Yellow
    & ccache --version
    Write-Host ""
    Write-Host "==================================" -ForegroundColor Cyan
    Write-Host "安装完成！" -ForegroundColor Green
    Write-Host "==================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "下一步：" -ForegroundColor Yellow
    Write-Host "  1. 重启终端（以使 PATH 生效）" -ForegroundColor White
    Write-Host "  2. 验证: ccache --version" -ForegroundColor White
    Write-Host "  3. 配置项目: cmake -B build" -ForegroundColor White
    Write-Host ""
    Write-Host "如果需要立即使用，运行：" -ForegroundColor Yellow
    Write-Host "  `$env:Path += ';$installDir'" -ForegroundColor Gray
} else {
    Write-Host "✗ CCache 未找到，请手动添加到 PATH: $installDir" -ForegroundColor Red
    exit 1
}
