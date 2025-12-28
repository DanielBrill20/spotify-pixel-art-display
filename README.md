# Spotify Pixel Art Display

<img src="assets\flower_boy_cover.jpg" alt="Flower Boy album cover on LED matrix" height="250">
<img src="assets\game_of_life.jpg" alt="Conway's Game of Life on LED Matrix" height="250">
<img src="assets\not_all_heroes_wear_capes_cover.jpg" alt="NOT ALL HEROES WEAR CAPES album cover on LED matrix" height="250">

Bring music data into the physical world with a Wi-Fi connected LED matrix that turns Spotify album art into animated pixel art. This project spans embedded systems, networked services, and creative tooling, purpose-built to showcase end-to-end experience design for creative technologist roles.

## Highlights for Creative Technologists
- Blends cloud APIs, edge computing, and real-time graphics into a single storytelling artifact.
- Demonstrates ownership of hardware bring-up, firmware architecture, and supporting backend services.
- Presents a tangible user journey: connect to Wi-Fi, press play in Spotify, watch the matrix react.

## Feature Snapshot
- **Live album art rendering** - Streaming Spotify cover art is resized to a 64x64 RGB matrix and displayed in under a second.
- **Generative screensaver** - A custom Conway's Game of Life animation fills downtime when nothing is playing.
- **Automatic mDNS discovery** - The Python server resolves the ESP32 by hostname, avoiding static IP friction during demos.
- **Robust networking** - The firmware exposes `/image` and `/screensaver` HTTP endpoints, handles timeouts, and protects against malformed payloads.
- **Configurable hardware layer** - Panel dimensions, brightness, and HUB75 pin mappings are tweakable without touching source.

## Spotify Demo (Click to view on Vimeo)
<a href="https://vimeo.com/1147558667">
   <img src="assets\spotify_demo_thumbnail.png" alt="Click to view Spotify demo video" width="300">
</a>

## Game of Life Demo (Click to view on Vimeo)
<a href="https://vimeo.com/1147558569">
   <img src="assets\gol_demo_thumbnail.png" alt="Click to view Game of Life demo video" width="300">
</a>

## Temp Holder
<img src="assets\panel_back.jpg" alt="Back of LED panel" height="350">
<img src="assets\panel_power_wires.png" alt="Wires to power panel and ESP" height="350">
<img src="assets\esp32_pinout.png" alt="ESP32 pinout" height="350">
<img src="assets\hardware_construction.jpg" alt="Panel with all wires connected" height="350">

| Signal | ESP32-S3 Pin |
|--------|--------------|
| R1     | 4            |
| G1     | 5            |
| B1     | 6            |
| R2     | 17           |
| G2     | 18           |
| B2     | 8            |
| A      | 10           |
| B      | 11           |
| C      | 12           |
| D      | 13           |
| E      | 9            |
| LAT    | 47           |
| OE     | 21           |
| CLK    | 16           |

## System Overview
1. The ESP32 firmware (ESP-IDF plus Arduino component) boots, connects to Wi-Fi, hosts a lightweight HTTP server, and drives the HUB75 LED matrix through DMA.
2. The companion Python app authenticates with Spotify, polls the currently playing endpoint, and keeps a session open to the ESP.
3. When the track changes, the server fetches album art, converts it into raw RGB bytes, and posts them to the microcontroller.
4. When playback pauses or stops, the server tells the firmware to run the Game of Life screensaver until music resumes.

## Hardware
- ESP32 board with PSRAM (ESP32-S3 recommended for HUB75 DMA throughput)
- 64x64 (or chained) HUB75 RGB LED matrix
- 5V power supply sized for the panel's peak draw
- Level shifting or ribbon cable adapter, depending on the panel

## Firmware Setup (ESP-IDF)
1. Install ESP-IDF and add the tools to your PATH.
2. Clone this repo (include submodules) and open a shell in esp_firmware/.
3. Apply component patches so the bundled libraries build correctly:
   ```bash
   ./apply_patches.sh             # macOS / Linux
   # or
   .\apply_patches.ps1            # Windows PowerShell
   ```
4. Select the correct silicon target for your board:
   ```bash
   idf.py set-target esp32s3      # choose esp32, esp32s3, esp32s2, etc.
   ```
5. Configure Wi-Fi credentials, mDNS hostname, panel geometry, and HUB75 pins:
   ```bash
   idf.py menuconfig              # Spotify Pixel Art Display Configuration
   ```
6. Build, flash, and monitor:
   ```bash
   idf.py build
   idf.py flash
   idf.py monitor                 # Press Ctrl+] to exit
   ```

### Firmware Behavior Notes
- matrix_driver.cpp initializes the HUB75 panel, manages double buffering, and exposes display_image() and display_screensaver() for incoming requests.
- wifi_manager.c wraps the full station workflow: NVS init, event loop wiring, WPA2 configuration, and mDNS registration.
- http_server.c hosts two POST endpoints, streams binary image payloads safely, and delegates rendering to the matrix driver.
- life_screensaver.cpp runs the Game of Life on a periodic timer and cycles colors with a rainbow transition.

## Spotify API Server Setup (Python)
1. Move into the backend folder:
   ```bash
   cd spotify_api_server
   ```
2. Create and activate a virtual environment:
   ```bash
   python -m venv .venv
   source .venv/bin/activate       # macOS / Linux
   # or
   .\.venv\Scripts\Activate.ps1    # Windows PowerShell
   ```
3. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```
4. Create a Spotify application and capture the client credentials and redirect URI.
5. Create a new file named .env and add the required variables:
   ```dotenv
   CLIENT_ID=your_spotify_client_id
   CLIENT_SECRET=your_spotify_client_secret
   REDIRECT_URI=http://localhost:8888/callback
   ```
6. Run the server:
   ```bash
   python main.py
   ```
7. A browser window will prompt for Spotify authorization the first time. Once authenticated, leave the script running; it polls every 0.5 seconds.

### Server Behavior Notes
- Keeps persistent requests.Session handles for Spotify and the ESP32 to minimize connection overhead.
- Resizes images intelligently: high-quality Lanczos downscaling to ART_RESOLUTION, nearest-neighbor scaling to panel size, final RGB byte dump for the microcontroller.
- Pads unexpected aspect ratios with black borders to avoid stretched art.
- Falls back to the screensaver when Spotify returns podcasts, ads, or silence.

## Running the Experience
1. Power the ESP32 and LED matrix after flashing the firmware. Wait for the "Matrix initialized" log in the monitor.
2. Start the Python service. Confirm the logs show a resolved mDNS hostname.
3. Play any track or podcast in Spotify. Within the next polling cycle, the matrix will refresh with the new cover art.
4. Pause playback to watch the Game of Life animation take over.

## Troubleshooting
- Matrix stays dark: Verify the HUB75 pin assignments under Spotify Pixel Art Display Configuration -> Pin Mappings match your wiring.
- Wi-Fi fails to connect: Double-check WIFI_SSID and WIFI_PASSWORD in menuconfig; the firmware retries up to MAX_CONNECTION_RETRIES before enabling the screensaver.
- mDNS resolution fails: The server falls back to LedMatrix.local, but you can hardcode an IP in main.py for unreliable networks.
- Spotify auth loop: Delete the .cache file generated by Spotipy and rerun to refresh credentials.
