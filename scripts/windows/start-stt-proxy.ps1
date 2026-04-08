$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$LocalConfig = Join-Path $RepoRoot "clawputer.local.ps1"

if (Test-Path $LocalConfig) {
    . $LocalConfig
} else {
    Write-Warning "Missing clawputer.local.ps1. Copy clawputer.local.ps1.example first."
}

if (-not $env:GROQ_API_KEY) {
    throw "GROQ_API_KEY is not set."
}

$ffmpegLocal = Join-Path $RepoRoot ".tools\ffmpeg\bin"
if ($env:FFMPEG_BIN) {
    $env:PATH = "$env:FFMPEG_BIN;$env:PATH"
} elseif (Test-Path $ffmpegLocal) {
    $env:PATH = "$ffmpegLocal;$env:PATH"
}

Push-Location $RepoRoot
try {
    python -u .\tools\stt_proxy.py
} finally {
    Pop-Location
}
