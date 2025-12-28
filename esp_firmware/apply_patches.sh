#!/bin/bash
cd "$(dirname "$0")"
set -e
git submodule update --init --recursive
git apply patches/Adafruit_BusIO.patch --directory=esp_firmware/components/Adafruit_BusIO
git apply patches/Adafruit-GFX-Library.patch --directory=esp_firmware/components/Adafruit-GFX-Library
git apply patches/ESP32-HUB75-MatrixPanel-DMA.patch --directory=esp_firmware/components/ESP32-HUB75-MatrixPanel-DMA
