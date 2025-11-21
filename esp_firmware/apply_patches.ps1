Set-Location -Path $PSScriptRoot
$ErrorActionPreference = 'Stop'

try {
    git submodule update --init --recursive
    git apply patches/Adafruit_BusIO.patch --directory=components/Adafruit_BusIO
    git apply patches/Adafruit-GFX-Library.patch --directory=components/Adafruit-GFX-Library
    git apply patches/ESP32-HUB75-MatrixPanel-DMA.patch --directory=components/ESP32-HUB75-MatrixPanel-DMA
    Write-Host 'Done.'
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
