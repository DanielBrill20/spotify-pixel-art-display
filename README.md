# Spotify Pixel Art Display

<img src="assets\flower_boy_cover.jpg" alt="Flower Boy album cover on LED matrix" height="250">
<img src="assets\game_of_life.jpg" alt="Conway's Game of Life on LED Matrix" height="250">
<img src="assets\not_all_heroes_wear_capes_cover.jpg" alt="NOT ALL HEROES WEAR CAPES album cover on LED matrix" height="250">

Bring your listening habits to your room décor with this HUB75 LED matrix display. This Wi-Fi connected art piece converts album art into pixel art, dynamically displaying whatever you’re listening to on Spotify. When Spotify isn’t playing, the matrix shifts to a fading rainbow gradient screensaver based on Conway’s Game of Life.

## Feature Snapshot
- **Live album art rendering** - Streaming Spotify cover art is resized to a 64x64 RGB matrix and displayed in under a second.
- **Generative screensaver** - A custom Conway's Game of Life animation fills downtime when nothing is playing.
- **Automatic mDNS discovery** - The Python server resolves the ESP32 by hostname, avoiding static IP friction during demos.
- **Robust networking** - The firmware exposes `/image` and `/screensaver` HTTP endpoints, handles timeouts, and protects against malformed payloads.
- **Configurable hardware layer** - Panel dimensions, brightness, and HUB75 pin mappings are configurable without touching source.

## Spotify Demo (Click to view on Vimeo)
<a href="https://vimeo.com/1147558667">
   <img src="assets\spotify_demo_thumbnail.png" alt="Click to view Spotify demo video" width="300">
</a>

## Game of Life Demo (Click to view on Vimeo)
<a href="https://vimeo.com/1147558569">
   <img src="assets\gol_demo_thumbnail.png" alt="Click to view Game of Life demo video" width="300">
</a>

## Backend Spotify API Server Overview
The core of the project is a backend Python server that repeatedly calls the Spotify Web API to get the currently playing track or podcast episode for the user. The flow is as follows:

1.	The user authenticates Spotify, following the [Authorization Code Flow]( https://developer.spotify.com/documentation/web-api/tutorials/code-flow).
2.	Once authenticated, a browser window will pop up. The user pastes this link to the terminal.
3.	Credentials get saved in a `.cache` file. From there, the server handles token renewal.
4.	Every 0.5 seconds, poll the Spotify Wen API to fetch the user’s currently playing song or episode.
5.	On song change, get the album cover.
6.	If needed, resize the image. Resizing is based on:
      - The LED matrix panel resolution
      - The desired pixel art resolution
         - Note that this means that the pixel art resolution is customizable. While the default is a 64x64 image on a 64x64 matrix, you can also opt for a more pixelated effect by drawing a 32x32 grid on a 64x64 matrix. In this example, each pixel of the image would be represented by 4 physical LEDs (2x2 LEDs) on the matrix.
7.	Send the image bytes via HTTP POST request to the ESP32.
8.	If Spotify stops playing, send an intent to switch to “screensaver mode” to the ESP32.

## ESP-IDF HUB75 LED Matrix Firmware Overview
For HUB75 LED matrix control, I use an ESP32-S3 (my dev board of choice is a ESP32-S3-DevKitC-1 N8R8, although you can use any module as long as the MCU has enough memory to handle large DMA buffers). This project utilizes a double DMA buffer for rapid and smooth drawing of images. While one image is being displayed with data in the front buffer, data can be loaded into the back buffer. Once the back buffer image is ready, the buffers are swapped for immediate frame transition. Otherwise, the user would see images being drawn line by line. The firmware is primarily coded in C using the ESP-IDF framework. The matrix drawing is done in C++ using the [ESP32-HUB75-MatrixPanel-DMA library]( https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA). The flow of the firmware is as follows:
1.	Initialize the LED matrix driver. This includes setting up custom pinouts, matrix panel dimensions, clearing the screen, etc.
2.	Initialize the Wi-Fi manager. This includes authenticating the Wi-Fi access point connection, handling disconnects and retries, registering an mDNS hostname for easy backend server communication, etc.
3.	Initialize the HTTP server that communicates with the backend server by registering the `/image` and `/screensaver` endpoints.

### `/image` Endpoint
This is a POST endpoint to receive image bytes from the backend server. The server verifies that the correct amount of bytes was received, reads all bytes into a static buffer, and passes them on to the `matrix_driver` module. From there, this module:
1.	Takes the data from the image buffer
2.	Performs array math to map bytes to pixels
3.	Draws each pixel
4.	Flips the DMA buffers once drawing is complete, immediately displaying the new image

### `/screensaver` Endpoint
This is a POST endpoint that accepts no data, but rather simply tells the `matrix_driver` module to enter screensaver mode. Currently, that means displaying the Conway’s Game of Life screensaver.

### Conway’s Game of Life Screensaver
This screensaver is simply [Conway’s Game of Life]( https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) with an added relaxing rainbow gradient effect. Each tick, or generation, lasts for 300 milliseconds before generating the next one. The starting seed is randomly generated using the `esp_random` function, which generates true random numbers by sampling noise from the active Wi-Fi module ([documentation]( https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/random.html)). The canvas is also continuous, meaning that it loops in the x and y directions, so X<sub>min</sub> == X<sub>max</sub> + 1.

## Dependencies
### Backend Server
-	Spotipy
-	Pillow
-	Other common ones, see `requirements.txt`

### ESP32 Firmware
-	[ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA)
-	[Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)
-	[Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO)
-	[arduino-esp32](https://github.com/espressif/arduino-esp32)
-	Other ESP components for Wi-Fi, mDNS, etc.

`Adafruit-GFX` is a dependency of `ESP32-HUB75-MatrixPanel-DMA`, and `Adafruit_BusIO` is a dependency of `Adafruit-GFX`. It is recommended to install these as git submodules, as there are currently no native ESP components.
`arduino-esp32` (a dependency of the DMA library), on the other hand, can be [installed as a component](https://components.espressif.com/components/espressif/arduino-esp32). Once this is done, several updates will need to be made to the various components `CMakeLists.txt` files. This can be done quickly with supplied git patches.

## Firmware Setup
1. Install ESP-IDF and add the tools to your PATH.
2. Clone this repo (include submodules) and open a shell in esp_firmware/:
   ```
   git clone --recurse-submodules https://github.com/DanielBrill20/spotify-pixel-art-display.git
   # or
   git clone https://github.com/DanielBrill20/spotify-pixel-art-display.git
   git submodule update --init --recursive
   ```
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

## Spotify API Server Setup (Python)
1. Move into the backend folder.
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
7. A browser window will prompt for Spotify authorization the first time.

## Hardware
1.	**HUB75 LED matrix** (a square aspect ratio looks best considering album art is, well, square!)
      - I found mine on Amazon. Here’s the [spec sheet](https://www.waveshare.com/wiki/RGB-Matrix-P2.5-64x64).
2. **ESP32-S3 dev board** with sufficient RAM
3.	**5V 4A (minimum) PSU**
      - **Note:** LED panels draw quite a lot of current, and they struggle when powered by an inconsistent or overloaded PSU. Between the panel, ESP32, potentially other peripherals in the future, and wanting to operate at about 50% max amperage, I opted for a 5V 9A (45W) PSU. Specifically, I’m using the [PEAMD72-10-B2]( https://www.ttelectronics.com/TTElectronics/media/ProductFiles/Datasheet/PEAMD72.pdf) from TT Electronics.
4.	**Cables** for data and power (often come with the matrix)
      - **Note:** Not all wires are created equal! For any component undergoing current, it’s good to understand what’s the maximum current the component can safely withstand. Too many projects I see ignore this, using flimsy jumper wires to carry large amounts of current. The potential downsides? Damaged wires, melting plastic, fire? I’m no electrical engineer, but get thick enough wires! Here is a handy [Wiki and wire ampacity chart](https://en.wikipedia.org/wiki/American_wire_gauge).
5.	**Screw terminal DC barrel jack connector**
6.	**Computer (and power supply, storage, etc.)** to host the backend server. I’m using my Raspberry Pi Zero 2 W, which is the hub for all my projects.
7.	**Optional: A diffuser screen.** I use a piece of translucent black acrylic that I cut to size. This diffuses the LEDs and improves contrast
8.	**Optional: A small fuse (0.75 – 1 A) and fuse holder**
      - For a clean assembly, I power the matrix and ESP with the same PSU. An ESP should never draw more current than it needs. However, due to the high amperage of the PSU, I figured it couldn’t hurt to add a safeguard between the PSU and ESP.

### Hardware Images
<img src="assets\panel_back.jpg" alt="Back of LED panel" height="350">
<img src="assets\panel_power_wires.png" alt="Wires to power panel and ESP" height="350">
<img src="assets\esp32_pinout.png" alt="ESP32 pinout" height="350">
<img src="assets\hardware_construction.jpg" alt="Panel with all wires connected" height="350">

### Pinout
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

## Portfolio Spotlight
Since this is a portfolio project, here’s a bit more in-depth about some unique features and challenges I conquered along the way.

### Image Data is Stored in Static Buffers
Image bytes from the backend HTTP request are read into an uninitialized static buffer in the firmware code. As an uninitialized static variable, this buffer lives in the .bss (Block Started by Symbol) segment of memory. This was a conscious choice influenced by a few factors:
1.	Displayable image data must always be `PANEL_WIDTH * PANEL_HEIGHT * 3` bytes, the 3 representing the three channels in RGB color. The display logic expects bytes for each LED (even if that means `(0, 0, 0)`), so anything less could cause distorted images. This constant size is perfect for a static buffer.
2.	If we were to read the HTTP POST content into a local variable, it would live in the stack. For a 64 x 64 panel, this is `12288` bytes, or a bit over 12 KB. By default in ESP-IDF, the main task stack size is usually around 4 KB. This would cause stack overflow unless you increased the stack size, which there’s no reason to do.
3.	The other option would be to dynamically allocate and free the image bytes memory. Repeated large dynamic allocations risk heap fragmentation, especially with a project meant to run continuously.

### Backend Image Fetching is Efficient and Handles Malformed Data
For songs and podcasts episodes, the Spotify Web API typically includes the three album cover images of various sizes. The typical output looks something like:
```
"images": [
  {
    "height": 640,
    "url": "https://i.scdn.co/image/ab67616d0000b273bd5f03953f9f1d3b833369c0",
    "width": 640
  },
  {
    "height": 300,
    "url": "https://i.scdn.co/image/ab67616d00001e02bd5f03953f9f1d3b833369c0",
    "width": 300
  },
  {
    "height": 64,
    "url": "https://i.scdn.co/image/ab67616d00004851bd5f03953f9f1d3b833369c0",
    "width": 64
  }
]
```
As you can see, the last image is often 64 x 64. If you have a 64 x 64 LED panel, this is perfect! To minimize image processing slowdowns and unnecessary resizes, the server starts with the smallest image, checks its size (provided by the JSON response), and only resizes if needed based on the panel dimensions and desired pixel art resolution. In essence, it uses the smallest usable image. It will never attempt to scale a picture up to dimensions.

Furthermore, in testing, some album covers either crashed the firmware code or caused distorted images on the matrix. A couple of problematic albums I found were JAY-Z’s The Black Album and Gunna’s a Gift & a Curse. The problem? These albums, despite dimensions stated by the API response, are not actually square. For a “64 x 64” image, dimensions were typically a few pixels short in one dimension. The fix? To avoid stretching the album art, I opted for padding the edges with off pixels and centering the image.

To explain the observed issues, the firmware would crash because it was attempting to read more bytes from the HTTP request than where sent. In the few cases of distorted, almost “twisted” looking images, images were too narrow. Image bytes are stored in a 1D array, not 2D. Images are drawn from left to right, top down. If an image is 60 pixels wide, but being drawn to the 64 x 64 matrix, the image bytes are just read sequentially from the buffer. So, the last 4 pixels on the first row will be filled with data meant for the first 4 pixels on the second row. This will leave 8 would-be-empty pixels on the second row (4 from the narrow image, 4 from the bytes “moved” to the first row). These 8 pixels will be filled with the data from the first 8 pixels of the third row. As you can see, this effect compounds, and the image appears to twist.

### Backend Server Uses a Single Requests Session for ESP32 Communication
The server uses Python’s `requests` library for inter-device communication. By using only one persistent requests session, TCP/TLS handshakes and mDNS lookups happen once at the start of the connection between devices. Each subsequent POST request can avoid these time-intensive operations, significantly reducing data transfer time and improving matrix responsiveness by roughly 10x.

### Firmware Defaults to a Negative Clock Phase
In initial matrix testing, my display had significant ghosting and was occasionally shifted by 1 pixel. My specific hardware has a negative clock edge, which is easily addressed by the DMA library. In fact, [here’s their much better documentation on the issue]( https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA?tab=readme-ov-file#:~:text=Clock%20Phase,is%20commented%20out).

### Wi-Fi Initialization is Blocking
This is true for any ESP-IDF project. However, by default, the various steps required in Wi-Fi AP configuration happen asynchronously. Since I immediately set up an HTTP server after, which relies on a connected Wi-Fi network, server setup can silently fail if Wi-Fi initialization is not complete. For this reason, this project uses [FreeRTOS `xEventGroupWaitBits`](https://freertos.org/Documentation/02-Kernel/04-API-references/12-Event-groups-or-flags/04-xEventGroupWaitBits) to make Wi-Fi initialization blocking.

## Future Improvements
1.	Migrate from an HTTP server to WebSocket for communication between the backend server and ESP32 server. Response time is already near-immediate, but WebSocket will improve error handling and detecting when one device is offline. For example, there is no need to poll the Spotify Web API if the ESP and matrix are powered off.
2.	More screensavers
3.	A nice wooden or 3D printed enclosure
