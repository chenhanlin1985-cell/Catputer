param(
    [string]$Port = "COM5",
    [string]$SourceFile = ".\\sdcard\\pet\\dialogue"
)

python .\scripts\windows\push_sd_dialogue.py --port $Port --source $SourceFile
