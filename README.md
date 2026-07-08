# SmartFix Matrix

A modular ESP32-S3 firmware for a SmartFix Elektronikservice LED matrix sign using a Waveshare ESP32-S3 RGB Matrix board and a 64×32 HUB75 RGB LED panel.

The display shows a custom SmartFix header, scrolling service text, pixel-art effects, random animations, configurable colors, brightness, WiFi settings, and OTA firmware updates from GitHub Releases.

> Project status: active development / maker project.

---

## Preview

Add your project photos or GIFs here later, for example:

```text
assets/smartfix-matrix-front.jpg
assets/smartfix-matrix-webui.png
```

Suggested image block:

```md
![SmartFix Matrix running on a 64x32 LED panel](assets/smartfix-matrix-front.jpg)
```

---

## Features

- Modular PlatformIO / Arduino firmware for ESP32-S3
- HUB75 RGB LED matrix output via ESP32 DMA library
- 64×32 RGB LED panel support
- SmartFix header/logo text with green `Smart` and blue `Fix`
- Header animation effects:
  - Static
  - Letter-by-letter
  - Fade in / fade out
  - Slide in
  - Shimmer
  - Sparkle
  - Pulse glow
- Scrolling text for service messages
- Editable display text from the web interface
- Saved text color and scroll speed
- Brightness control
- Pixel-art display mode
- Random animation mode
- Auto demo mode
- Integrated WiFi access point for configuration
- Optional home WiFi connection
- OTA firmware update from GitHub Release asset
- Persistent settings stored in ESP32 NVS / Preferences
- Separate build outputs for:
  - USB full flash binary
  - OTA app-only binary

---

## Hardware

Tested target hardware:

- Waveshare ESP32-S3 RGB Matrix board
- Waveshare RGB Full-Color LED Matrix Panel P2.5 64×32
- HUB75 connection
- 5 V power supply for the LED matrix panel

Current display size:

```text
64 px wide × 32 px high
```

Panel expansion notes:

```text
1 × 64×32 panel  = 64×32
2 × 64×32 side-by-side = 128×32
2 × 64×32 stacked      = 64×64
4 × 64×32 as 2×2       = 128×64
```

---

## Firmware structure

The firmware is split into multiple source files so the project stays easy to maintain.

```text
src/
├── main.cpp
├── config.h
├── app_state.h
├── app_state.cpp
├── matrix_display.h
├── matrix_display.cpp
├── animations.h
├── animations.cpp
├── web_interface.h
├── web_interface.cpp
├── settings.h
├── settings.cpp
├── wifi_manager.h
├── wifi_manager.cpp
├── ota_update.h
└── ota_update.cpp
```

Main responsibilities:

```text
config.h              firmware version, panel size, default values
app_state.*           global app state and active settings
matrix_display.*      HUB75 setup, colors, SmartFix header drawing
animations.*          scrolling text, logo effects, pixel art, random FX
web_interface.*       configuration web page and routes
settings.*            Preferences / NVS load, save, factory reset
wifi_manager.*        access point, home WiFi, reconnect logic
ota_update.*          GitHub OTA update handling
main.cpp              small setup/loop entry point
```

---

## Build & flash with PlatformIO

Build the firmware:

```powershell
pio run
```

Flash over USB:

```powershell
pio run -t upload
```

Open serial monitor:

```powershell
pio device monitor
```

If upload does not start automatically, hold **BOOT**, tap **RESET**, then start upload again.

---

## Generated firmware binaries

The project can generate two different `.bin` files inside the PlatformIO build folder:

```text
.pio/build/esp32-s3-rgb-matrix/SmartFix-Matrix-ota.bin
.pio/build/esp32-s3-rgb-matrix/SmartFix-Matrix-usb.bin
```

Use them like this:

```text
SmartFix-Matrix-ota.bin = app-only firmware for OTA / web update
SmartFix-Matrix-usb.bin = full flash image for USB / esptool
```

The normal PlatformIO upload command still works:

```powershell
pio run -t upload
```

For manual full USB flashing with esptool:

```powershell
python -m esptool --chip esp32s3 --port COM7 --baud 921600 write_flash 0x0 .pio\build\esp32-s3-rgb-matrix\SmartFix-Matrix-usb.bin
```

Change `COM7` to your actual ESP32 serial port.

---

## Web configuration

On first use, the ESP32 creates a WiFi access point:

```text
SSID:     SmartFix-Matrix
Password: smartfix123
```

Connect your phone or computer to this WiFi and open:

```text
http://192.168.4.1/
```

The web interface can configure:

- Display mode
- Auto demo mode
- Brightness
- Scrolling text
- Scroll speed
- Text color
- Header/logo text
- Header/logo animation effect
- Home WiFi SSID and password
- OTA firmware URL
- Factory reset

Settings are saved on the ESP32 and restored after reboot.

---

## GitHub OTA update

OTA updates use the app-only binary:

```text
SmartFix-Matrix-ota.bin
```

Recommended GitHub Release asset name:

```text
SmartFix-Matrix-ota.bin
```

Example OTA URL:

```text
https://github.com/acothebraco/SmartFix-Matrix/releases/latest/download/SmartFix-Matrix-ota.bin
```

Important:

- The repository or release asset must be publicly reachable, or OTA needs authentication.
- Do not upload the full USB image as OTA firmware.
- OTA needs the app-only `.bin` file.

---

## Repository layout

```text
.github/workflows/     optional GitHub Actions build/release workflows
scripts/               helper scripts, binary export scripts
src/                   firmware source code
platformio.ini         PlatformIO project configuration
README.md              project documentation
```

---

## Development workflow

Typical local workflow:

```powershell
git status
pio run
pio run -t upload
git add .
git commit -m "Describe your change"
git push
```

Create a release tag:

```powershell
git tag v1.2.0
git push origin v1.2.0
```

---

## Roadmap

Planned / possible future improvements:

- GitHub Actions release workflow
- Automatic upload of OTA and USB binaries to GitHub Releases
- Browser-based ESP Web Tools flasher
- More SmartFix header fonts
- More pixel-art animations
- Multi-panel support for 128×32 or 64×64
- Improved SmartFix shield/wrench pixel logo
- Night mode / schedule-based brightness

---

## Notes

This is a hobby / maker firmware project for the SmartFix Elektronikservice LED sign.

Do not commit private WiFi passwords, tokens, certificates, or other secrets to the repository.

---

## License

MIT License is recommended if you want other makers to use or fork the project.

Add a `LICENSE` file to the repository when you are ready.
