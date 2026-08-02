#pragma once
/*
  ============================================================
  ESP32-C3 SUPER MINI — PIN MAP FOR THIS BUILD
  ============================================================
  Buttons (as you already wired them):
    LEFT   -> GPIO0
    OK     -> GPIO1
    RIGHT  -> GPIO3
  All buttons: INPUT_PULLUP, active LOW (button shorts pin to GND).

  OLED (SH1106, I2C):
    SDA -> GPIO4
    SCL -> GPIO5

  MP3-TF-18 (DFPlayer-compatible, UART, uses UART1 remapped):
    ESP32 RX (GPIO6) <- module TX
    ESP32 TX (GPIO7) -> module RX  (through a ~1k resistor is recommended,
                                    module runs 3.3V logic on RX so a
                                    resistor is optional but safe)
    Module powered from 5V (or 3.3V per your amp stage), GND common.

  BUSY pin from module (optional, tells you when a track is playing):
    GPIO10

  ------------------------------------------------------------
  PINS TO AVOID ON THE SUPER MINI AND WHY
  ------------------------------------------------------------
  GPIO2, GPIO8, GPIO9  -> strapping pins (boot-mode selection).
                          GPIO9 is also the on-board BOOT button
                          (external pull-up already on the board).
                          GPIO8 drives the on-board blue LED (active LOW)
                          on most Super Mini boards.
                          Avoid driving these low at boot / using them
                          as buttons wired permanently to GND.
  GPIO11-GPIO17        -> NOT broken out on the Super Mini. These are
                          used internally for the SPI flash on the
                          embedded-flash ESP32-C3FN4. Do not use.
  GPIO18, GPIO19       -> Native USB D-/D+. Avoid if you want USB
                          flashing / USB-CDC serial to keep working.
  GPIO20, GPIO21       -> Default UART0 (RX/TX). Fine to reuse only if
                          you don't need Serial.print() over the
                          USB-serial bridge for debugging.

  Everything else (0,1,3,4,5,6,7,10,20,21) is free to use. This build
  only needs 0,1,3,4,5,6,7 and optionally 10 — well clear of every
  reserved/strapping pin, and your existing button wiring (0/1/3) is
  fine as-is.
  ============================================================
*/

// --- Buttons ---
#define PIN_BTN_LEFT   3
#define PIN_BTN_OK     1
#define PIN_BTN_RIGHT  0

// --- OLED (I2C) ---
#define PIN_OLED_SDA   4
#define PIN_OLED_SCL   5
#define OLED_ADDR      0x3C   // most SH1106 modules; try 0x3D if blank

// --- MP3-TF-18 (UART1, remapped pins) ---
#define PIN_MP3_RX     6   // ESP32 RX <- module TX
#define PIN_MP3_TX     7   // ESP32 TX -> module RX
#define PIN_MP3_BUSY   10  // optional, LOW while a track is playing

// GPIO usable for deep-sleep wake on ESP32-C3 (LP/RTC-capable: 0-5)
#define WAKE_PIN       PIN_BTN_OK
