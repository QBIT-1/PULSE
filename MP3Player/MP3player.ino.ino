/*
  ESP32-C3 Super Mini MP3 Player
  Hardware: MP3-TF-18 (DFPlayer-compatible), SH1106 128x64 OLED (I2C),
            3 tactile buttons (Left / OK / Right).

  Required libraries (Arduino Library Manager):
    - Adafruit GFX Library
    - Adafruit SH110X
    - DFRobotDFPlayerMini  (by DFRobot — works with MP3-TF-18 clones,
      they speak the same YX5200/GD3200B serial protocol)
  Board core: esp32 by Espressif Systems (tested against 3.x)
  Board setting: "ESP32C3 Dev Module"

  See pins.h for the full pin map and the pins-to-avoid list.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <esp_sleep.h>

#include "pins.h"
#include "logo.h"
#include "buttons.h"
#include "player.h"
#include "config.h"
#include "ui.h"

Adafruit_SH1106G display(128, 64, &Wire, -1);
Buttons     buttons;
MP3Player   mp3;
ConfigStore configStore;

// Optional: put your real track titles here in SD-card play order.
// Leave empty ({}) to just show "Track 001", "Track 002", ...
const char *trackNames[] = {};
uint16_t trackNameCount = 0;

Tab      currentTab   = TAB_PLAYLIST;
uint16_t playlistSel   = 1;     // 1-based track index, playlist cursor
uint8_t  configCursor  = 0;

uint32_t lastActivity  = 0;
bool     screenAsleep  = false;

bool     waitingForDeepSleep = false;
uint32_t deepSleepStart = 0;

// -------- drawing dispatch for the current tab --------
void renderCurrentTab(GFXcanvas1 &c) {
  switch (currentTab) {
    case TAB_PLAYLIST: drawPlaylist(c, playlistSel, mp3.trackCount); break;
    case TAB_PLAYER:    drawPlayer(c, mp3); break;
    case TAB_CONFIG:    drawConfig(c, configStore.s, configCursor); break;
  }
}

#define CANVAS_BYTES 1024  // (128/8) * 64

void redrawStatic() {
  renderCurrentTab(canvasB);
  pushStatic(canvasB);
  memcpy(canvasA.getBuffer(), canvasB.getBuffer(), CANVAS_BYTES); // keep A in sync
}

void redrawWithSlide(int direction) {
  memcpy(canvasA.getBuffer(), canvasB.getBuffer(), CANVAS_BYTES); // A = what's on screen
  renderCurrentTab(canvasB);                                     // B = the new frame
  slideTransition(canvasA, canvasB, direction);
}

void wakeScreenIfAsleep() {
  if (screenAsleep) {
    screenAsleep = false;
    redrawStatic();
  }
}

void switchTab(int dir /* +1 forward, -1 back — we only ever go forward */) {
  if (currentTab == TAB_PLAYER) {
    mp3.stop();                 // leaving the player screen stops playback
  }
  currentTab = (Tab)((currentTab + 1) % 3);
  redrawWithSlide(dir);
}

void startDeepSleepCountdown() {
  waitingForDeepSleep = true;
  deepSleepStart = millis();
}

void enterDeepSleep() {
  display.clearDisplay();
  display.display();
  pinMode(WAKE_PIN, INPUT_PULLUP);
  // ESP32-C3 deep-sleep GPIO wake (not the classic ext0/ext1 API).
  esp_deep_sleep_enable_gpio_wakeup(1ULL << WAKE_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
  // never returns
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

  display.begin(OLED_ADDR, true);
  display.clearDisplay();
  display.drawBitmap(0, 0, logo_bitmap, LOGO_WIDTH, LOGO_HEIGHT, SH110X_WHITE);
  display.display();
  delay(1500);

  buttons.begin();
  configStore.load();

  if (!mp3.begin()) {
    display.clearDisplay();
    centerText(canvasB, "MP3 module", 20, 1);
    centerText(canvasB, "not found!", 34, 1);
    pushStatic(canvasB);
    while (true) delay(1000); // halt — check wiring / SD card
  }
  mp3.setVolume(min((uint8_t)mp3.volume, configStore.s.maxVolume));

  randomSeed(esp_random());
  lastActivity = millis();
  redrawStatic();
}

void loop() {
  // --- MP3 module events (track-finished notifications) ---
  if (mp3.dfp.available()) {
    uint8_t type = mp3.dfp.readType();
    if (type == DFPlayerPlayFinished) {
      bool keepGoing = mp3.onTrackFinished(configStore.s.autoplay);
      if (currentTab == TAB_PLAYER) redrawStatic();
      if (!keepGoing) startDeepSleepCountdown();
    }
  }
  if (mp3.playing) waitingForDeepSleep = false;

  // --- deep sleep countdown ---
  if (waitingForDeepSleep &&
      millis() - deepSleepStart > (uint32_t)configStore.s.deepSleepSec * 1000) {
    enterDeepSleep();
  }

  // --- screen sleep on inactivity ---
  if (!screenAsleep &&
      millis() - lastActivity > (uint32_t)configStore.s.screenSleepSec * 1000) {
    screenAsleep = true;
    display.clearDisplay();
    display.display();
  }

  // --- buttons ---
  ButtonEvent ev = buttons.poll();
  if (ev == EV_NONE) return;

  lastActivity = millis();
  if (screenAsleep) {           // first press after sleep only wakes up
    wakeScreenIfAsleep();
    return;
  }
  if (ev == EV_ANY_ACTIVITY) return;

  if (ev == EV_COMBO_SWITCH_TAB) { switchTab(+1); return; }

  switch (currentTab) {

    case TAB_PLAYLIST: {
      uint16_t total = mp3.trackCount;
      if (total == 0) break;
      if (ev == EV_LEFT_SHORT || ev == EV_LEFT_LONG_REPEAT) {
        playlistSel = (playlistSel == 1) ? total : playlistSel - 1;
        redrawStatic();
      } else if (ev == EV_RIGHT_SHORT || ev == EV_RIGHT_LONG_REPEAT) {
        playlistSel = (playlistSel % total) + 1;
        redrawStatic();
      } else if (ev == EV_OK_DOUBLE) {
        mp3.playSelected(playlistSel);
        currentTab = TAB_PLAYER;
        redrawWithSlide(+1);
      }
      break;
    }

    case TAB_PLAYER: {
      if (ev == EV_LEFT_SHORT) {
        mp3.previous(); redrawWithSlide(-1);
      } else if (ev == EV_RIGHT_SHORT) {
        mp3.next(); redrawWithSlide(+1);
      } else if (ev == EV_LEFT_LONG || ev == EV_LEFT_LONG_REPEAT) {
        mp3.volDown();
        if (mp3.volume > configStore.s.maxVolume) mp3.setVolume(configStore.s.maxVolume);
        redrawStatic();
      } else if (ev == EV_RIGHT_LONG || ev == EV_RIGHT_LONG_REPEAT) {
        if (mp3.volume < configStore.s.maxVolume) mp3.volUp();
        redrawStatic();
      } else if (ev == EV_OK_SHORT) {
        mp3.togglePlayPause(); redrawStatic();
      } else if (ev == EV_OK_DOUBLE) {
        mp3.cycleRepeat(); redrawStatic();
      } else if (ev == EV_OK_LONG) {
        mp3.toggleShuffle(); redrawStatic();
      }
      break;
    }

    case TAB_CONFIG: {
      Settings &s = configStore.s;
      if (ev == EV_LEFT_SHORT) {
        configCursor = (configCursor == 0) ? CONFIG_ITEM_COUNT - 1 : configCursor - 1;
        redrawStatic();
      } else if (ev == EV_RIGHT_SHORT) {
        configCursor = (configCursor + 1) % CONFIG_ITEM_COUNT;
        redrawStatic();
      } else if (ev == EV_OK_SHORT) {
        switch (configCursor) {
          case 0: s.autoplay = !s.autoplay; break;
          case 1: s.maxVolume = (s.maxVolume >= 30) ? 5 : s.maxVolume + 5; break;
          case 2: cycleScreenSleep(s.screenSleepSec); break;
          case 3: cycleDeepSleep(s.deepSleepSec); break;
        }
        configStore.save();
        redrawStatic();
      } 
      break;
    }
  }
}
