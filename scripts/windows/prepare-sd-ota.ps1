param(
    [string]$EnvName = "waveshare-amoled-18",
    [string]$TargetRoot = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
Push-Location $repoRoot
try {
    if (-not $SkipBuild) {
        pio run -e $EnvName
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed for $EnvName."
        }
    }

    $firmwareBin = Join-Path $repoRoot ".pio\build\$EnvName\firmware.bin"
    if (-not (Test-Path $firmwareBin)) {
        throw "Missing firmware binary: $firmwareBin"
    }

    if ([string]::IsNullOrWhiteSpace($TargetRoot)) {
        $TargetRoot = Join-Path $repoRoot "sdcard"
    }
    $targetFirmwareDir = Join-Path $TargetRoot "firmware"
    New-Item -ItemType Directory -Force -Path $targetFirmwareDir | Out-Null

    $targetBin = Join-Path $targetFirmwareDir "update.bin"
    Copy-Item -Force $firmwareBin $targetBin

    $sizeKB = [math]::Round((Get-Item $targetBin).Length / 1024.0, 1)
    Write-Host "SD OTA package ready: $targetBin ($sizeKB KB)" -ForegroundColor Green
} finally {
    Pop-Location
}

