param(
    [string]$EnvName = "waveshare-amoled-18",
    [string]$Target = "",
    [string]$UploadPort = "",
    [switch]$PkgList
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path

function Get-CoreBucket {
    param([string]$Name)

    if ($Name -like "waveshare-*") { return "waveshare" }
    if ($Name -like "m5stack-*") { return "cardputer" }
    if ($Name -eq "native") { return "native" }
    return ($Name -replace '[^A-Za-z0-9_.-]', '_')
}

$CoreBucket = Get-CoreBucket -Name $EnvName
$CoreDir = Join-Path $RepoRoot ".pio-core\$CoreBucket"
New-Item -ItemType Directory -Force -Path $CoreDir | Out-Null

$PreviousCoreDir = $env:PLATFORMIO_CORE_DIR
$env:PLATFORMIO_CORE_DIR = $CoreDir
$ExitCode = 1

$PioExe = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"
$PioArgs = if ($PkgList) {
    @("pkg", "list", "-e", $EnvName)
} else {
    $args = @("run", "-e", $EnvName)
    if ($Target) {
        $args += @("-t", $Target)
    }
    if ($UploadPort) {
        $args += @("--upload-port", $UploadPort)
    }
    $args
}

Push-Location $RepoRoot
try {
    Write-Host "PlatformIO env: $EnvName" -ForegroundColor Cyan
    Write-Host "PlatformIO core: $CoreDir" -ForegroundColor Cyan
    if (Test-Path $PioExe) {
        & $PioExe @PioArgs
    } else {
        python -m platformio @PioArgs
    }
    $ExitCode = $LASTEXITCODE
} finally {
    if ($null -ne $PreviousCoreDir) {
        $env:PLATFORMIO_CORE_DIR = $PreviousCoreDir
    } else {
        Remove-Item Env:PLATFORMIO_CORE_DIR -ErrorAction SilentlyContinue
    }
    Pop-Location
}

exit $ExitCode
