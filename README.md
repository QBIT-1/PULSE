# PULSE

**A DIY lossless MP3 player, built from scratch.**

---------------------------------------------------

PULSE is a DIY MP3 player made for audio and electronics enthusiasts, built around an MP3 module controlled by an ESP32-C3 Super Mini, with an SH1106 OLED for the UI and 3-button navigation. It runs on a 1000mAh battery charged via TP4056, and works well both as a hobby build and as a real daily-use MP3 player with a few modifications.

---

## Features

- 🎧 **Lossless audio playback** (WAV/FLAC)
- 🖥️ **1.3" monochrome OLED** display
- 🔋 **Up to 9 hours** of playback per charge
- 🔌 **3.5mm audio output**
- 💾 **microSD card** storage
- 🎛️ **3-button tactile UI** for navigation
- ⚡ **USB-C charging** via TP4056

---

## Hardware

| Component | Part |
|---|---|
| MCU | ESP32-C3 Super Mini |
| Display | 1.3" SH1106 monochrome OLED (I2C) |
| Audio | PAM8304 amplifier |
| Storage | microSD card |
| Charging | TP4056 (USB-C, default 1A charge current) |
| Battery | 1000mAh LiPo pouch cell |
| Input | 3x tactile pushbuttons |

### Display wiring (I2C)

| OLED Pin | ESP32-C3 |
|---|---|
| GND | GND |
| VCC | 3V3 |
| SCL | I2C SCL |
| SDA | I2C SDA |

### Button mapping

| Button | GPIO |
|---|---|
| Left | GPIO0 |
| OK / Select | GPIO1 |
| Right | GPIO3 |

---

## Battery & Charging

- **Battery:** 1000mAh LiPo pouch cell
- **Charger:** TP4056, default configuration (~1A charge current)
- **Charge time:** ~1 hour to ~80%, ~1.5 hours for a full charge (CV taper)
- **Playback time:** up to 9 hours

---

## Gallery



![PULSE front](pulse_front.jpg)




![PULSE internals](pulse_internals.jpg)



---

## Status

🚧 Actively in development — UI, playlist system, and audio pipeline are being iterated on.

---

## License

MIT — build your own, remix it, make it yours.