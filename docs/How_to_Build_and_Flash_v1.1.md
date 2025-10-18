# How to Build and Flash Firmware (v1.1).

This document explains how to build and flash the ESP8266 GPS UDP Tracker v1.1 firmware, including the integrated web interface stored in LittleFS.

---

## Requirements

- VS Code with the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)  
  or [PlatformIO Core CLI](https://docs.platformio.org/en/latest/core/installation.html)
- Python 3.8+ with [esptool](https://github.com/espressif/esptool)
- ESP8266 board (tested on Wemos D1 Mini, compatible with NodeMCU and similar boards)

---

## Build the Firmware

1. Clone or download this repository:
   ```bash
   git clone https://github.com/sc0ps/ESP8266-GPS-UDP-tracker.git
   cd ESP8266-GPS-UDP-tracker
   ```

2. Open the project in VS Code with PlatformIO  
   or navigate to the folder in your terminal.

3. Build the firmware:
   ```bash
   pio run
   ```

After building, the combined binary will be created at:
```text
firmware/ESP8266_GPS_UDP_Tracker_v1.1.bin
```

This file already contains both:
- the firmware code
- the web interface (LittleFS)

No separate `uploadfs` step is required.

---

## Flash to Your ESP8266

Connect your ESP via USB and replace the port name as needed.

**Windows**
```bash
esptool.py --port COM3 write_flash 0x00000 firmware/ESP8266_GPS_UDP_Tracker_v1.1.bin
```

**macOS / Linux**
```bash
esptool.py --port /dev/ttyUSB0 write_flash 0x00000 firmware/ESP8266_GPS_UDP_Tracker_v1.1.bin
```

When flashing completes, the ESP will reboot automatically.

---

## First-Time Setup

Connect to the Wi-Fi network:
```text
gps_tracker_setup
```
(default password: 12345678)

Open your browser and go to:
```text
http://192.168.4.1
```

Configure your:
- Wi-Fi SSID and password
- UDP broadcast IP and port
- GPIO pins for LED, GPS RX, and TX

Click **Save Configuration**.  
The tracker will reboot and connect to your selected Wi-Fi network.

---

## Notes

- Default filesystem layout: `eagle.flash.4m2m.ld` (2 MB app / 2 MB LittleFS)
- Output binary built automatically via `combine_raw.py`.
- Tested with PlatformIO 6.x and Arduino framework 3.1.2.

---

© 2025 Scops Owl Designs (Sc0ps)  
Licensed under the [GNU General Public License v3.0](./LICENSE.md).  
You are free to use, modify, and distribute this software under the same license.  
For details, see the full license text in the repository.

