#requires -Version 7

<#
.SYNOPSIS
    从 PowerShell 7 配置文件中卸载 Visual Studio 2022 环境

.DESCRIPTION
    移除之前通过 install-vs-profile.ps1 安装的 VS 环境配置

.EXAMPLE
    .\uninstall-vs-profile.ps1
#>

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "╔════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  VS 2022 PowerShell 7 环境卸载器       ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

$profilePath = $PROFILE.CurrentUserCurrentHost

Write-Host "配置文件: $profilePath" -ForegroundColor Yellow
Write-Host ""

if (-not (Test-Path $profilePath)) {
    Write-Host "配置文件不存在，无需卸载" -ForegroundColor Yellow
    exit 0
}

# 读取配置文件
$profileContent = Get-Content $profilePath -Raw -Encoding UTF8

if (-not ($profileContent -match "VS-Dev-Environment")) {
    Write-Host "未找到 VS 环境配置，无需卸载" -ForegroundColor Yellow
    exit 0
}

Write-Host "⚠️  即将从 PowerShell 配置中移除 VS 环境" -ForegroundColor Yellow
Write-Host ""
$confirm = Read-Host "确认卸载? (Y/N)"

if ($confirm -ne "Y" -and $confirm -ne "y") {
    Write-Host "已取消" -ForegroundColor Yellow
    exit 0
}

# 备份原配置
$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$backupPath = "$profilePath.backup_$timestamp"
Copy-Item $profilePath $backupPath -Force
Write-Host "✓ 已备份到: $backupPath" -ForegroundColor Green

# 移除 VS 环境配置（多种格式支持）
$newContent = $profileContent -replace "#Region VS-Dev-Environment[\s\S]*?#EndRegion", ""
$newContent = $newContent -replace "#= VS-Dev-Environment Start[\s\S]*?= VS-Dev-Environment End", ""

# 清理多余的空行
$newContent = $newContent -replace "`r`n`r`n`r`n+", "`r`n`r`n"
$newContent = $newContent -replace "`n`n`n+", "`n`n"
$newContent = $newContent.Trim()

# 如果配置文件为空，删除它
if ([string]::IsNullOrWhiteSpace($newContent)) {
    Remove-Item $profilePath -Force
    Write-Host "✓ 配置文件已删除（内容为空）" -ForegroundColor Green
} else {
    Set-Content $profilePath $newContent -Encoding UTF8 -NoNewline
    Write-Host "✓ VS 环境已从配置中移除" -ForegroundColor Green
}

Write-Host ""
Write-Host "卸载完成!" -ForegroundColor Green
Write-Host ""
Write-Host "重新启动 PowerShell 7 生效" -ForegroundColor Cyan
Write-Host ""
