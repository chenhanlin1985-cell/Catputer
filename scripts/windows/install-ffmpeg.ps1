$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$ToolsDir = Join-Path $RepoRoot ".tools"
$ZipPath = Join-Path $ToolsDir "ffmpeg.zip"
$ExtractDir = Join-Path $ToolsDir "ffmpeg-extract"
$TargetDir = Join-Path $ToolsDir "ffmpeg"

New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null

$url = "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip"
Write-Host "Downloading ffmpeg ..." -ForegroundColor Cyan
Invoke-WebRequest -Uri $url -OutFile $ZipPath

if (Test-Path $ExtractDir) {
    Remove-Item -Recurse -Force $ExtractDir
}
Expand-Archive -Path $ZipPath -DestinationPath $ExtractDir -Force

$inner = Get-ChildItem $ExtractDir -Directory | Select-Object -First 1
if (-not $inner) {
    throw "Failed to extract ffmpeg archive."
}

if (Test-Path $TargetDir) {
    Remove-Item -Recurse -Force $TargetDir
}
Move-Item -Path $inner.FullName -Destination $TargetDir

Write-Host "ffmpeg installed to $TargetDir" -ForegroundColor Green
Write-Host "Binary path: $TargetDir\\bin" -ForegroundColor Green
