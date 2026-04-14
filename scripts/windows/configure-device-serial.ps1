param(
    [string]$Port,
    [string]$WifiSsid,
    [string]$WifiPass,
    [string]$ApiKey,
    [string]$ApiHost,
    [string]$ApiPort,
    [string]$City
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$LocalConfig = Join-Path $RepoRoot "clawputer.local.ps1"

if (Test-Path $LocalConfig) {
    . $LocalConfig
}

if (-not $Port) {
    $Port = Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
        Where-Object { $_.PNPDeviceID -match "VID_303A&PID_1001" } |
        Select-Object -First 1 -ExpandProperty DeviceID
}

if (-not $Port) {
    throw "Could not auto-detect a Catputer USB serial port. Re-run with -Port COMx."
}

$argsList = @("--port", $Port)
if ($WifiSsid) { $argsList += @("--wifi-ssid", $WifiSsid) }
if ($WifiPass) { $argsList += @("--wifi-pass", $WifiPass) }
if ($ApiKey) { $argsList += @("--api-key", $ApiKey) }
if ($ApiHost) { $argsList += @("--api-host", $ApiHost) }
if ($ApiPort) { $argsList += @("--api-port", $ApiPort) }
if ($City) { $argsList += @("--city", $City) }

Push-Location $RepoRoot
try {
    python scripts\windows\configure-device-serial.py @argsList
} finally {
    Pop-Location
}
