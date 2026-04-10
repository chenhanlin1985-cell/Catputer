param(
    [string]$Port = "COM5",
    [string]$SourceDir = ".\\sdcard\\pet\\ime"
)

$ErrorActionPreference = "Stop"
Set-Location (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location ..\..

python .\scripts\windows\generate_ime_buckets.py --source ".\sdcard\pet\ime\pinyin.txt" --target-dir $SourceDir
python .\scripts\windows\push_sd_ime.py --port $Port --source-dir $SourceDir
