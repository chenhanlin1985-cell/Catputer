param(
    [string]$Port = "COM5"
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path

Push-Location $RepoRoot
try {
    python -m platformio device monitor -p $Port -b 115200
} finally {
    Pop-Location
}
