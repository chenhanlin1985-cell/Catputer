param(
    [string]$Port,
    [string]$EnvName = "waveshare-amoled-18",
    [switch]$SkipBuild,
    [switch]$IncludeVoiceData,
    [switch]$IgnorePortBusy,
    [switch]$TryDirectReset
)

$ErrorActionPreference = "Stop"

function Find-WavesharePort {
    $ports = Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
        Where-Object { $_.PNPDeviceID -match "VID_303A&PID_1001" } |
        Select-Object -ExpandProperty DeviceID
    if ($ports) {
        return @($ports)[0]
    }
    return $null
}

function Get-WavesharePorts {
    $ports = Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
        Where-Object { $_.PNPDeviceID -match "VID_303A&PID_1001" } |
        Select-Object -ExpandProperty DeviceID
    if (-not $ports) { return @() }
    return @($ports)
}

function Find-WaveshareBootPort {
    param([string]$RunPort)

    $ports = Get-WavesharePorts
    if (-not $ports) { return $null }

    $com9 = $ports | Where-Object { $_ -eq "COM9" } | Select-Object -First 1
    if ($com9) { return $com9 }

    $nonRunPort = $ports | Where-Object { $RunPort -and $_ -ne $RunPort } | Select-Object -Last 1
    if ($nonRunPort) { return $nonRunPort }
    return $null
}

function Select-NewBootPort {
    param(
        [string[]]$BeforePorts,
        [string[]]$AfterPorts
    )

    $beforeSet = @{}
    foreach ($p in $BeforePorts) { $beforeSet[$p] = $true }
    foreach ($p in $AfterPorts) {
        if (-not $beforeSet.ContainsKey($p)) { return $p }
    }
    return $null
}

function Test-PortAvailable {
    param([string]$SerialPort)

    if (-not $SerialPort) { return $false }
    try {
        $sp = [System.IO.Ports.SerialPort]::new($SerialPort, 115200)
        $sp.ReadTimeout = 200
        $sp.WriteTimeout = 200
        $sp.Open()
        $sp.Close()
        $sp.Dispose()
        return $true
    } catch {
        return $false
    }
}

function Touch-Bootloader {
    param([string]$SerialPort)

    try {
        $sp = [System.IO.Ports.SerialPort]::new($SerialPort, 1200)
        $sp.DtrEnable = $false
        $sp.RtsEnable = $true
        $sp.Open()
        Start-Sleep -Milliseconds 200
        $sp.Close()
    } catch {
        # The port can disappear during the 1200bps touch. That is the expected
        # path when the board re-enumerates into its bootloader port.
        Write-Host "Bootloader touch changed the serial port state: $($_.Exception.Message)"
    }
}

function Invoke-Flash {
    param(
        [string]$FlashPort,
        [string]$BeforeMode,
        [string]$PenvPython,
        [string]$EspTool,
        [string]$Bootloader,
        [string]$Partitions,
        [string]$BootApp,
        [string]$Firmware,
        [string]$VoiceData,
        [switch]$IncludeVoiceData
    )

    Write-Host "Flashing Waveshare AMOLED on $FlashPort (before=$BeforeMode) ..." -ForegroundColor Cyan
    $env:PYTHONIOENCODING = "utf-8"
    $flashArgs = @(
        "--chip", "esp32s3",
        "--port", $FlashPort,
        "--baud", "460800",
        "--connect-attempts", "12",
        "--before", $BeforeMode,
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
        $flashArgs += @($voiceDataOffset, $VoiceData)
    }

    & $PenvPython $EspTool @flashArgs | Out-Host
    $FlashExitCode = $LASTEXITCODE
    Write-Host "esptool exit code: $FlashExitCode"
    return [int]$FlashExitCode
}

function Write-WaveshareFlashSuccessNote {
    param([string]$FlashPort)

    Write-Host "Waveshare flash completed on $FlashPort." -ForegroundColor Green
    Write-Host "Note: runtime is usually COM8 and bootloader is often COM9; after a verified COM9 write, judge by device behavior first instead of chasing a temporary COM8 log gap." -ForegroundColor DarkYellow
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$LocalConfig = Join-Path $RepoRoot "clawputer.local.ps1"
$PlatformioCoreDir = Join-Path $RepoRoot ".pio-core\waveshare"

if (Test-Path $LocalConfig) {
    . $LocalConfig
}

Push-Location $RepoRoot
try {
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

    if (-not $Port) {
        $Port = Find-WavesharePort
    }
    if (-not $Port) {
        throw "Could not auto-detect the Waveshare serial port. Re-run with -Port COMx."
    }
    Write-Host "Detected run port: $Port" -ForegroundColor DarkCyan

    $PenvPython = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\python.exe"
    $EspTool = Join-Path $PlatformioCoreDir "packages\tool-esptoolpy\esptool.py"
    $BootApp = Join-Path $PlatformioCoreDir "packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin"
    $Bootloader = Join-Path $RepoRoot ".pio\build\$EnvName\bootloader.bin"
    $Partitions = Join-Path $RepoRoot ".pio\build\$EnvName\partitions.bin"
    $Firmware = Join-Path $RepoRoot ".pio\build\$EnvName\firmware.bin"
    $VoiceData = Join-Path $RepoRoot "tools\local_tts\voice_data\esp_tts_voice_data_xiaoxin.dat"

    $requiredFlashInputs = @($PenvPython, $EspTool, $BootApp, $Bootloader, $Partitions, $Firmware)
    if ($IncludeVoiceData) {
        $requiredFlashInputs += $VoiceData
    }
    foreach ($Path in $requiredFlashInputs) {
        if (-not (Test-Path $Path)) {
            throw "Missing flash input: $Path"
        }
    }

    $exitCode = 2
    $existingBootPort = Find-WaveshareBootPort -RunPort $Port
    if ($existingBootPort) {
        Write-Host "Detected bootloader port $existingBootPort. Flashing COM9/bootloader directly before using the runtime port." -ForegroundColor Cyan
        if (-not $IgnorePortBusy -and -not (Test-PortAvailable -SerialPort $existingBootPort)) {
            throw "Bootloader port $existingBootPort is busy. Close monitor tools and retry."
        }
        $exitCode = Invoke-Flash -FlashPort $existingBootPort -BeforeMode "no_reset" `
            -PenvPython $PenvPython -EspTool $EspTool -Bootloader $Bootloader `
            -Partitions $Partitions -BootApp $BootApp -Firmware $Firmware -VoiceData $VoiceData `
            -IncludeVoiceData:$IncludeVoiceData
        if ($exitCode -ne 0) {
            throw "esptool flash failed with exit code $exitCode."
        }
        Write-WaveshareFlashSuccessNote -FlashPort $existingBootPort
        return
    }

    if (-not $IgnorePortBusy -and -not (Test-PortAvailable -SerialPort $Port)) {
        throw "Port $Port is busy. Close serial monitor/other tools first, then retry."
    }

    if ($TryDirectReset) {
        $exitCode = Invoke-Flash -FlashPort $Port -BeforeMode "default_reset" `
            -PenvPython $PenvPython -EspTool $EspTool -Bootloader $Bootloader `
            -Partitions $Partitions -BootApp $BootApp -Firmware $Firmware -VoiceData $VoiceData `
            -IncludeVoiceData:$IncludeVoiceData
        if ($exitCode -eq 0) {
            Write-WaveshareFlashSuccessNote -FlashPort $Port
            return
        }
        Write-Host "Direct reset flash failed with exit code $exitCode. Trying 1200bps auto bootloader touch ..." -ForegroundColor Yellow
    } else {
        Write-Host "No existing COM9 bootloader port found. Skipping COM8 direct reset and using 1200bps touch -> bootloader port." -ForegroundColor Yellow
    }

    $portsBeforeTouch = Get-WavesharePorts
    Touch-Bootloader -SerialPort $Port
    $BootPort = $null
    Write-Host "Waiting for COM9/bootloader port after 1200bps touch ..." -ForegroundColor DarkCyan
    for ($i = 0; $i -lt 20; $i++) {
        Start-Sleep -Milliseconds 500
        $portsNow = Get-WavesharePorts
        $portsText = if ($portsNow) { $portsNow -join "," } else { "<none>" }
        Write-Host ("  bootloader probe {0}/20: {1}" -f ($i + 1), $portsText)
        $preferredBootPort = $portsNow | Where-Object { $_ -eq "COM9" } | Select-Object -First 1
        if ($preferredBootPort) {
            $BootPort = $preferredBootPort
            break
        }
        $newPort = Select-NewBootPort -BeforePorts $portsBeforeTouch -AfterPorts $portsNow
        if ($newPort) {
            $BootPort = $newPort
            break
        }
        $candidatePort = $portsNow | Where-Object { $_ -ne $Port } | Select-Object -Last 1
        if ($candidatePort) {
            $BootPort = $candidatePort
            break
        }
    }
    if (-not $BootPort) {
        throw "Could not find COM9/bootloader serial port within 10 seconds after 1200bps touch. Stop here instead of reusing the runtime port."
    }
    if (-not (Test-PortAvailable -SerialPort $BootPort)) {
        throw "Bootloader port $BootPort is busy. Close monitor tools and retry."
    }

    $exitCode = Invoke-Flash -FlashPort $BootPort -BeforeMode "no_reset" `
        -PenvPython $PenvPython -EspTool $EspTool -Bootloader $Bootloader `
        -Partitions $Partitions -BootApp $BootApp -Firmware $Firmware -VoiceData $VoiceData `
        -IncludeVoiceData:$IncludeVoiceData
    if ($exitCode -ne 0) {
        throw "esptool flash failed with exit code $exitCode."
    }
    Write-WaveshareFlashSuccessNote -FlashPort $BootPort
} finally {
    if ($null -ne $PreviousCoreDir) {
        $env:PLATFORMIO_CORE_DIR = $PreviousCoreDir
    } else {
        Remove-Item Env:PLATFORMIO_CORE_DIR -ErrorAction SilentlyContinue
    }
    Pop-Location
}
