# Changelog – ESP8266 GPS UDP Tracker

All notable changes to this project will be documented in this file.

---

## v1.1 — October 2025
**Status:** Open-source release  
**License:** CC BY-NC 4.0  

### Added
- Configurable GPIO pin assignment for RX, TX, and LED via the web interface  
- Software **Factory Reset** function added to the web interface  
- Improved connection handling for WiFi reconnections  
- Enhanced GPS serial hot-swap stability  
- Project and license fully open-sourced under CC BY-NC 4.0  
- Developer documentation added: [How to Build and Flash (v1.1)](docs/How_to_Build_and_Flash_v1.1.md)  

### Changed
- Updated default LED behavior for better status indication  
- Web UI layout improvements and responsive scaling  
- Internal configuration stored using JSON for better reliability  

### Fixed
- Occasional reboot loop when WiFi unavailable  
- GPS freeze when serial buffer overflowed  

---

## v1.0 — October 2025 
**Status:** Closed-source firmware release  

### Added
- WiFi configuration portal (AP mode on first boot)  
- UDP broadcast of NMEA GPS data  
- LED feedback for WiFi and GPS status  

---

© 2025 Scops Owl Designs (Sc0ps)  
Released under the Creative Commons Attribution–NonCommercial 4.0 International License.
