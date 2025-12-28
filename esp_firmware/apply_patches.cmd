@echo off
setlocal
cd /d "%~dp0"

git submodule update --init --recursive || goto :error
git apply patches/Adafruit_BusIO.patch --directory=esp_firmware/components/Adafruit_BusIO || goto :error
git apply patches/Adafruit-GFX-Library.patch --directory=esp_firmware/components/Adafruit-GFX-Library || goto :error
git apply patches/ESP32-HUB75-MatrixPanel-DMA.patch --directory=esp_firmware/components/ESP32-HUB75-MatrixPanel-DMA || goto :error

echo Done.
exit /b 0

:error
echo Failed with errorlevel %errorlevel%.
exit /b %errorlevel%
