param(
    [string]$Port,
    [string]$Source = ".\sdcard\pet\backgrounds",
    [switch]$Restart
)

$argsList = @(".\scripts\windows\push_sd_backgrounds.py", "--source", $Source)
if ($Port) {
    $argsList += @("--port", $Port)
}
if ($Restart) {
    $argsList += "--restart"
}
python @argsList
