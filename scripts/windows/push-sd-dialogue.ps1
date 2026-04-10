param(
    [string]$Port = "COM5",
    [string]$SourceFile = ".\\sdcard\\pet\\dialogue\\dialogue.txt"
)

python .\scripts\windows\push_sd_dialogue.py --port $Port --source $SourceFile
