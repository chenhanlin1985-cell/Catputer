param(
    [string]$EnvName = "waveshare-amoled-18",
    [switch]$SkipBuild,
    [switch]$IncludeVoiceData,
    [int]$WaitSeconds = 35,
    [int]$PollMs = 80,
    [int]$TouchAttempts = 3,
    [switch]$AutoTouchFromCOM8,
    [switch]$AcknowledgeChecklist
)

$ErrorActionPreference = "Stop"

function Show-Checklist {
    Write-Host "================ FLASH MUST-READ ================" -ForegroundColor Yellow
    Write-Host "Required flow (Waveshare):"
    Write-Host "1) Trigger 1200bps touch on COM8 (disconnect/reconnect sound is expected)."
    Write-Host "2) Wait for log: probe ... COM9"
    Write-Host "3) Wait for log: [COM9-WAIT] COM9 detected. Starting esptool flash..."
    Write-Host "4) Verify log: Hash of data verified."
    Write-Host "5) Verify log: [COM9-WAIT] Flash completed on COM9."
    Write-Host ""
    Write-Host "Rule: DO NOT use COM8 for flashing. Flash COM9 only." -ForegroundColor Yellow
    Write-Host "Doc: docs/flash-must-read.md" -ForegroundColor DarkCyan
    Write-Host "=================================================" -ForegroundColor Yellow
}

function Has-ComPort {
    param([string]$PortName)
    return [System.IO.Ports.SerialPort]::GetPortNames() -contains $PortName
}

function Wait-ForCom9 {
    param([int]$TimeoutSeconds, [int]$PollIntervalMs)

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $probe = 0
    while ((Get-Date) -lt $deadline) {
        $probe += 1
        $ports = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
        $portsText = if ($ports.Count -gt 0) { $ports -join "," } else { "<none>" }
        Write-Host ("[COM9-WAIT] probe {0}: {1}" -f $probe, $portsText)
        if ($ports -contains "COM9") {
            return $true
        }
        Start-Sleep -Milliseconds $PollIntervalMs
    }
    return $false
}

function Wait-ForCom9Ready {
    param([int]$TimeoutSeconds = 8, [int]$PollIntervalMs = 150)

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $attempt = 0
    while ((Get-Date) -lt $deadline) {
        $attempt += 1
        try {
            $sp = [System.IO.Ports.SerialPort]::new("COM9", 115200)
            $sp.Open()
            $sp.Close()
            $sp.Dispose()
            Write-Host ("[COM9-WAIT] COM9 ready after {0} probe(s)." -f $attempt) -ForegroundColor DarkCyan
            return $true
        } catch {
            Write-Host ("[COM9-WAIT] COM9 present but busy, ready probe {0}: {1}" -f $attempt, $_.Exception.Message) -ForegroundColor DarkCyan
            Start-Sleep -Milliseconds $PollIntervalMs
        }
    }
    return $false
}

function Touch-Com8ToBootloader {
    if (-not (Has-ComPort -PortName "COM8")) {
        Write-Host "[COM9-WAIT] COM8 not present, skip auto touch." -ForegroundColor Yellow
        return
    }
    try {
        $sp = [System.IO.Ports.SerialPort]::new("COM8", 1200)
        $sp.DtrEnable = $false
        $sp.RtsEnable = $true
        $sp.Open()
        Start-Sleep -Milliseconds 250
        $sp.Close()
        $sp.Dispose()
        Write-Host "[COM9-WAIT] Auto touch sent on COM8 (1200bps)." -ForegroundColor DarkCyan
    } catch {
        Write-Host "[COM9-WAIT] Auto touch changed serial state: $($_.Exception.Message)" -ForegroundColor DarkCyan
    }
}

function Invoke-Com9Flash {
    param(
        [string]$PenvPython,
        [string]$EspTool,
        [string]$Bootloader,
        [string]$Partitions,
        [string]$BootApp,
        [string]$Firmware,
        [string]$VoiceData,
        [switch]$IncludeVoiceData
    )

    $args = @(
        "--chip", "esp32s3",
        "--port", "COM9",
        "--baud", "460800",
        "--connect-attempts", "12",
        "--before", "no_reset",
        "--after", "hard_reset",
        "write_flash", "-z",
        "--flash_mode", "dio",
        "--flash_freq", "80m",
        "--flash_size", "16MB",
        "0x0", $Bootloader,
        "0x8000", $Partitions,
        "0xe000", $BootApp,
        "0x10000", $Firmware
    )
    $voiceDataOffset = "0x890000"
    if ($IncludeVoiceData) {
        $args += @($voiceDataOffset, $VoiceData)
    }

    Write-Host "[COM9-WAIT] COM9 detected. Starting esptool flash..." -ForegroundColor Cyan
    & $PenvPython $EspTool @args | Out-Host
    return [int]$LASTEXITCODE
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$LocalConfig = Join-Path $RepoRoot "clawputer.local.ps1"
$PlatformioCoreDir = Join-Path $RepoRoot ".pio-core\waveshare"

if (Test-Path $LocalConfig) {
    . $LocalConfig
}

Push-Location $RepoRoot
try {
    Show-Checklist
    if (-not $AcknowledgeChecklist) {
        throw "Checklist not acknowledged. Re-run with -AcknowledgeChecklist after reading docs/flash-must-read.md."
    }
    if (-not $PSBoundParameters.ContainsKey("AutoTouchFromCOM8")) {
        $AutoTouchFromCOM8 = $true
    }

    New-Item -ItemType Directory -Force -Path $PlatformioCoreDir | Out-Null
    $PreviousCoreDir = $env:PLATFORMIO_CORE_DIR
    $env:PLATFORMIO_CORE_DIR = $PlatformioCoreDir

    if (-not $SkipBuild) {
        $PioExe = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"
        if (Test-Path $PioExe) {
            & $PioExe run -e $EnvName
        } else {
            python -m platformio run -e $EnvName
        }
        if ($LASTEXITCODE -ne 0) {
            throw "PlatformIO build failed with exit code $LASTEXITCODE."
        }
    }

    $PenvPython = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\python.exe"
    $EspTool = Join-Path $PlatformioCoreDir "packages\tool-esptoolpy\esptool.py"
    $BootApp = Join-Path $PlatformioCoreDir "packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin"
    $Bootloader = Join-Path $RepoRoot ".pio\build\$EnvName\bootloader.bin"
    $Partitions = Join-Path $RepoRoot ".pio\build\$EnvName\partitions.bin"
    $Firmware = Join-Path $RepoRoot ".pio\build\$EnvName\firmware.bin"
    $VoiceData = Join-Path $RepoRoot "tools\local_tts\voice_data\esp_tts_voice_data_xiaoxin.dat"

    $required = @($PenvPython, $EspTool, $BootApp, $Bootloader, $Partitions, $Firmware)
    if ($IncludeVoiceData) { $required += $VoiceData }
    foreach ($path in $required) {
        if (-not (Test-Path $path)) {
            throw "Missing flash input: $path"
        }
    }

    if ($AutoTouchFromCOM8) {
        if (-not (Has-ComPort -PortName "COM9")) {
            $touchOk = $false
            for ($i = 1; $i -le $TouchAttempts; $i++) {
                Write-Host ("[COM9-WAIT] Auto touch attempt {0}/{1}: COM8 -> bootloader..." -f $i, $TouchAttempts) -ForegroundColor Yellow
                Touch-Com8ToBootloader
                Write-Host "[COM9-WAIT] Waiting briefly for COM9 after touch..." -ForegroundColor Yellow
                if (Wait-ForCom9 -TimeoutSeconds 8 -PollIntervalMs $PollMs) {
                    $touchOk = $true
                    break
                }
                Write-Host "[COM9-WAIT] COM9 did not appear after this touch attempt." -ForegroundColor DarkYellow
                Start-Sleep -Milliseconds 400
            }
            if (-not $touchOk) {
                throw "COM9 did not appear after $TouchAttempts COM8 touch attempt(s). Stop here; do not flash COM8."
            }
        } else {
            Write-Host "[COM9-WAIT] COM9 already present, skip COM8 touch." -ForegroundColor DarkCyan
        }
    }
    Write-Host ("[COM9-WAIT] Confirming COM9 for up to {0}s." -f $WaitSeconds) -ForegroundColor Yellow
    if (-not (Wait-ForCom9 -TimeoutSeconds $WaitSeconds -PollIntervalMs $PollMs)) {
        throw "COM9 did not appear within $WaitSeconds seconds after COM8 touch. Stop here; do not flash COM8."
    }
    if (-not (Wait-ForCom9Ready)) {
        throw "COM9 appeared but stayed busy. Close any serial monitor and retry."
    }

    $exitCode = Invoke-Com9Flash -PenvPython $PenvPython -EspTool $EspTool `
        -Bootloader $Bootloader -Partitions $Partitions -BootApp $BootApp `
        -Firmware $Firmware -VoiceData $VoiceData -IncludeVoiceData:$IncludeVoiceData
    if ($exitCode -ne 0) {
        throw "esptool flash failed on COM9 with exit code $exitCode."
    }

    Write-Host "[COM9-WAIT] Flash completed on COM9." -ForegroundColor Green
} finally {
    if ($null -ne $PreviousCoreDir) {
        $env:PLATFORMIO_CORE_DIR = $PreviousCoreDir
    } else {
        Remove-Item Env:PLATFORMIO_CORE_DIR -ErrorAction SilentlyContinue
    }
    Pop-Location
}
