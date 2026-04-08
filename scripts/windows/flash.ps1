param(
    [string]$Port
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$LocalConfig = Join-Path $RepoRoot "clawputer.local.ps1"

if (Test-Path $LocalConfig) {
    . $LocalConfig
} else {
    Write-Warning "Missing clawputer.local.ps1. Copy clawputer.local.ps1.example first."
}

$required = @(
    "WIFI_SSID",
    "WIFI_PASS",
    "API_HOST",
    "API_PORT",
    "API_KEY",
    "STT_PROXY_HOST",
    "STT_PROXY_PORT",
    "DEFAULT_CITY"
)

$missing = foreach ($name in $required) {
    $item = Get-Item "Env:$name" -ErrorAction SilentlyContinue
    if (-not $item -or -not $item.Value) {
        $name
    }
}
if ($missing) {
    throw "Missing environment variables: $($missing -join ', ')"
}

if (-not $Port) {
    $ports = Get-PnpDevice -Class Ports |
        Where-Object { $_.FriendlyName -like "USB*" -or $_.InstanceId -match "VID_303A" } |
        Select-Object -ExpandProperty FriendlyName

    $match = $ports | Select-String -Pattern "COM\d+" | Select-Object -First 1
    if ($match) {
        $Port = $match.Matches[0].Value
    }
}

if (-not $Port) {
    throw "Could not auto-detect a USB serial port. Re-run with -Port COMx."
}

Write-Host "Flashing Catputer to $Port ..." -ForegroundColor Cyan
Push-Location $RepoRoot
try {
    python -m platformio run -t upload --upload-port $Port
} finally {
    Pop-Location
}
