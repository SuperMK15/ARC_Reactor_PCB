# ⚡ Arc Reactor PCB Firmware (ESP32-C3)

Production-grade, real-time procedural animation firmware and web-based Tactical HUD designed for the custom **ESP32-C3-MINI-1** Arc Reactor PCB.

---

## 🛠️ Hardware Mapping & Architecture

* **Microcontroller:** ESP32-C3-MINI-1 (4MB Flash, Native USB-C CDC on `GPIO18` / `GPIO19`).
* **LED Chain (`MCU_LED_DATA`):** `GPIO3` routed through `U3` (`SN74AHCT1G125`) 5V level shifter and `R7` ($330\ \Omega$) resistor.
  * **Index `0` (`D3`):** Center Core LED.
  * **Indices `1–8` (`D4` to `D11`):** Outer Ring (8 LEDs arranged clockwise starting from top-right).
* **Power Management:** Powered via USB-C (`J3`) or Lithium Battery input (`+BATT` / `D1` / `U2` AP63205 buck converter).

---

## 🚀 Features

1. **Dynamic Wi-Fi SoftAP (`ARC_REACTOR_<UUID>`):** Automatically generates a secure Access Point using the ESP32-C3's factory eFuse MAC address (e.g., `ARC_REACTOR_A1B2C3D4`) with password `iamironman`. Includes a captive portal redirect so connecting with a phone instantly opens the control HUD.
2. **Procedural Animation Engine (60 FPS):** Powered by `Adafruit_NeoPixel`, utilizing the ESP32-C3 hardware RMT peripheral for jitter-free lighting effects:
   * *MK-I Breathing* (Subtle plasma pulse)
   * *Plasma Turbine* (High-speed sub-pixel comet rotation)
   * *Dual Helix* (Counter-rotating outer nodes)
   * *Heartbeat* (Cardiac rhythm)
   * *Arc Overload* (Unstable random electrical discharges)
   * *Synthwave Spectrum* (Smooth hue shifting)
3. **Unibeam Overdrive Mode:** Triggers a blinding maximum-power flash sequence across all 9 LEDs with a single tap from the web HUD.
4. **Non-Volatile Storage (NVS) Persistence:** Automatically saves your active mode, master brightness, animation speed, and custom color selections to flash with a 1.5-second debounce timer.
5. **Mobile Control HUD:** A fully responsive cyberpunk interface served via **LittleFS** featuring live SVG visualizer synchronization, suit armor presets (Cyan, Vibranium, Hulkbuster, Extremis, and Stealth), and an approximate LED current estimate (20 mA per color channel; MCU current excluded).

If the page does not load, connect to the ESP32 access point and open `http://192.168.4.1`. If the device reports that the web UI is missing (or the page returns 404), upload the `data/` files with `pio run --target uploadfs`, then reload the page.

---

## 📦 Flashing Instructions

Make sure you have [PlatformIO Core](https://platformio.org/) installed in your terminal or VS Code.

### 1. Initialize Project & Dependencies
```powershell
cd firmware
pio project init
```

### 2. Build and Upload LittleFS Web Assets (`data/`)

```powershell
pio run --target uploadfs
```

### 3. Build and Flash Application Firmware

Connect your board via USB-C (`J3`) and run:

```powershell
pio run --target upload
```

### 4. Upload Firmware Over Wi-Fi

The first firmware upload must be done over USB to install the OTA listener. For later firmware updates, connect your computer to the ESP32's `ARC_REACTOR_<UUID>` Wi-Fi network (password: `iamironman`), then run from the `firmware/` directory:

```powershell
pio run -e esp32-c3-ota --target upload
```

PlatformIO uploads to `192.168.4.1` and authenticates using the AP password. The ESP32 must be powered and running, and your computer must stay connected to its Wi-Fi network during the upload. The regular `esp32-c3-devkitm-1` environment remains available for USB uploads.

To upload updated LittleFS web assets over Wi-Fi, use:

```powershell
pio run -e esp32-c3-ota --target uploadfsota
```

This sends the filesystem image to the ESP32's data partition and reboots the board when the upload finishes. The initial OTA-enabled firmware must already be installed over USB. If OTA is unavailable, you can still upload the assets over USB with `pio run --target uploadfs`.

---

## 🎛️ API & WebSocket Commands

Clients communicate with the ESP32-C3 via WebSocket (`ws://192.168.4.1/ws`) using JSON payloads:

* **Set Mode:** `{"cmd": "setMode", "value": <0-7>}`
* **Set Brightness:** `{"cmd": "setBrightness", "value": <5-255>}`
* **Set Speed:** `{"cmd": "setSpeed", "value": <1-100>}`
* **Set Colors:** `{"cmd": "setColors", "core": "#A0F5FF", "ring": "#00AAFF"}`
* **Toggle Power:** `{"cmd": "setPower", "value": true/false}`
* **Trigger Unibeam:** `{"cmd": "unibeam"}`
* **Apply Preset:** `{"cmd": "preset", "value": <0-4>}`
