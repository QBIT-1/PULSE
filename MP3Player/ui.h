#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "pins.h"
#include "player.h"
#include "config.h"

extern Adafruit_SH1106G display;

enum Tab { TAB_PLAYLIST = 0, TAB_PLAYER = 1, TAB_CONFIG = 2 };

// Two offscreen 1-bit canvases used to slide the old screen out and the
// new screen in. Whatever tab-drawing function ran last left its picture
// in `canvasB`; `canvasA` holds a snapshot of the previous frame.
GFXcanvas1 canvasA(128, 64);
GFXcanvas1 canvasB(128, 64);

// ---------- small helpers ----------
void centerText(GFXcanvas1 &c, const char *txt, int16_t y, uint8_t size) {
  c.setTextSize(size);
  int16_t x1, y1; uint16_t w, h;
  c.getTextBounds(txt, 0, y, &x1, &y1, &w, &h);
  c.setCursor((128 - w) / 2, y);
  c.print(txt);
}

void drawTabDots(GFXcanvas1 &c, Tab active) {
  for (int i = 0; i < 3; i++) {
    int x = 54 + i * 10;
    if (i == active) c.fillCircle(x, 62, 2, SH110X_WHITE);
    else             c.drawCircle(x, 62, 2, SH110X_WHITE);
  }
}

// Only a bare-numbers playlist is guaranteed from an MP3-TF-18 module
// (it can't read filenames off the card). Fill this in with your real
// track titles if you want nicer labels; index 0 = track 1.
extern const char *trackNames[];
extern uint16_t trackNameCount;

String labelFor(uint16_t track) {
  if (track >= 1 && track <= trackNameCount && trackNames[track - 1] != nullptr)
    return String(trackNames[track - 1]);
  char buf[16]; snprintf(buf, sizeof(buf), "Track %03d", track);
  return String(buf);
}

// ---------- PLAYLIST tab ----------
void drawPlaylist(GFXcanvas1 &c, uint16_t selected, uint16_t total) {
  c.fillScreen(SH110X_BLACK);
  if (total == 0) { centerText(c, "No tracks found", 30, 1); drawTabDots(c, TAB_PLAYLIST); return; }

  auto wrap = [&](int idx) -> uint16_t {
    while (idx < 1) idx += total;
    while (idx > (int)total) idx -= total;
    return (uint16_t)idx;
  };

  uint16_t prevT = wrap((int)selected - 1);
  uint16_t nextT = wrap((int)selected + 1);

  c.setTextColor(SH110X_WHITE);
  centerText(c, labelFor(prevT).c_str(), 6, 1);
  centerText(c, labelFor(nextT).c_str(), 46, 1);

  // selected item, bigger, centered — the iPod-style "focused" row
  centerText(c, labelFor(selected).c_str(), 24, 2);

  // position indicator
  char pos[16]; snprintf(pos, sizeof(pos), "%d/%d", selected, total);
  c.setTextSize(1);
  c.setCursor(2, 56); c.print(pos);

  // scrollbar on the right edge
  int barH = 40, barX = 124;
  c.drawRect(barX, 2, 3, barH, SH110X_WHITE);
  int fillY = 2 + (barH - 6) * (selected - 1) / max(1, (int)total - 1);
  c.fillRect(barX, fillY, 3, 6, SH110X_WHITE);

  drawTabDots(c, TAB_PLAYLIST);
}

// ---------- PLAYER (sound box) tab ----------
const char *repeatLabel(RepeatMode r) {
  switch (r) { case REPEAT_ONE: return "RPT:1"; case REPEAT_ALL: return "RPT:A"; default: return "RPT:-"; }
}

void drawPlayer(GFXcanvas1 &c, MP3Player &p) {
  c.fillScreen(SH110X_BLACK);
  c.setTextColor(SH110X_WHITE);

  centerText(c, labelFor(p.currentTrack).c_str(), 4, 1);

  // play/pause glyph, center
  int cx = 64, cy = 30;
  if (p.playing) {
    c.fillTriangle(cx - 6, cy - 8, cx - 6, cy + 8, cx + 8, cy, SH110X_WHITE);
  } else {
    c.fillRect(cx - 6, cy - 8, 5, 16, SH110X_WHITE);
    c.fillRect(cx + 3, cy - 8, 5, 16, SH110X_WHITE);
  }
  // prev/next chevrons
  c.fillTriangle(20, cy - 7, 20, cy + 7, 10, cy, SH110X_WHITE);
  c.fillTriangle(108, cy - 7, 108, cy + 7, 118, cy, SH110X_WHITE);

  // volume bar
  int volW = map(p.volume, 0, 30, 0, 40);
  c.drawRect(4, 46, 42, 6, SH110X_WHITE);
  c.fillRect(4, 46, volW, 6, SH110X_WHITE);
  c.setTextSize(1); c.setCursor(48, 46); c.print("VOL");

  // repeat + shuffle status
  c.setCursor(4, 56); c.print(repeatLabel(p.repeat));
  c.setCursor(70, 56); c.print(p.shuffle ? "SHUF:ON" : "SHUF:OFF");

  drawTabDots(c, TAB_PLAYER);
}

// ---------- CONFIG tab ----------
const char *CONFIG_ITEMS[] = {"Autoplay", "Max Volume", "Screen Sleep", "Deep Sleep"};
#define CONFIG_ITEM_COUNT 4

void drawConfig(GFXcanvas1 &c, Settings &s, uint8_t cursor) {
  c.fillScreen(SH110X_BLACK);
  c.setTextColor(SH110X_WHITE);
  c.setTextSize(1);
  centerText(c, "Settings", 0, 1);

  char valBuf[16];
  for (int i = 0; i < CONFIG_ITEM_COUNT; i++) {
    int y = 14 + i * 11;
    if (i == cursor) c.fillRect(0, y - 1, 128, 10, SH110X_WHITE);
    c.setTextColor(i == cursor ? SH110X_BLACK : SH110X_WHITE);
    c.setCursor(2, y); c.print(CONFIG_ITEMS[i]);

    switch (i) {
      case 0: snprintf(valBuf, sizeof(valBuf), "%s", s.autoplay ? "ON" : "OFF"); break;
      case 1: snprintf(valBuf, sizeof(valBuf), "%d", s.maxVolume); break;
      case 2: snprintf(valBuf, sizeof(valBuf), "%ds", s.screenSleepSec); break;
      case 3: snprintf(valBuf, sizeof(valBuf), "%ds", s.deepSleepSec); break;
    }
    int16_t x1, y1; uint16_t w, h;
    c.getTextBounds(valBuf, 0, y, &x1, &y1, &w, &h);
    c.setCursor(126 - w, y); c.print(valBuf);
  }
  c.setTextColor(SH110X_WHITE);
  drawTabDots(c, TAB_CONFIG);
}

// ---------- slide transition ----------
// direction: +1 = new content slides in from the right (e.g. "next")
//            -1 = new content slides in from the left  (e.g. "previous")
void slideTransition(GFXcanvas1 &from, GFXcanvas1 &to, int direction, uint8_t steps = 8) {
  for (uint8_t i = 1; i <= steps; i++) {
    int off = (128 * i) / steps;
    display.clearDisplay();
    if (direction >= 0) {
      display.drawBitmap(-off, 0, from.getBuffer(), 128, 64, SH110X_WHITE);
      display.drawBitmap(128 - off, 0, to.getBuffer(), 128, 64, SH110X_WHITE);
    } else {
      display.drawBitmap(off, 0, from.getBuffer(), 128, 64, SH110X_WHITE);
      display.drawBitmap(off - 128, 0, to.getBuffer(), 128, 64, SH110X_WHITE);
    }
    display.display();
    delay(8);
  }
}

// push a fully-drawn canvas straight to the screen, no animation
void pushStatic(GFXcanvas1 &c) {
  display.clearDisplay();
  display.drawBitmap(0, 0, c.getBuffer(), 128, 64, SH110X_WHITE);
  display.display();
}
