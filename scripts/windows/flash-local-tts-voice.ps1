param(
    [string]$Port = "COM5",
    [string]$VoiceDataPath = ".\\tools\\local_tts\\voice_data\\esp_tts_voice_data_xiaoxin.dat",
    [string]$Offset = "0x290000"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\\..")
$voiceFullPath = Resolve-Path (Join-Path $repoRoot $VoiceDataPath) -ErrorAction SilentlyContinue

if (-not $voiceFullPath) {
    throw "找不到 voice_data 文件：$VoiceDataPath"
}

Write-Host "Flashing local TTS voice data..."
Write-Host "  Port:   $Port"
Write-Host "  Offset: $Offset"
Write-Host "  File:   $voiceFullPath"

$esptoolPath = Join-Path $env:USERPROFILE ".platformio\packages\tool-esptoolpy\esptool.py"
if (-not (Test-Path $esptoolPath)) {
    throw "找不到 PlatformIO 自带 esptool.py：$esptoolPath"
}

python $esptoolPath --chip esp32s3 --port $Port --baud 460800 write_flash $Offset $voiceFullPath
