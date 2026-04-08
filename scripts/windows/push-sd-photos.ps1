param(
    [string]$Port = "COM5",
    [string]$SourceDir = "d:\clawputer\sdcard\pet\photos"
)

$ErrorActionPreference = "Stop"
python .\scripts\windows\push_sd_photos.py --port $Port --source $SourceDir
