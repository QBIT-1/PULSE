#pragma once
#include <Arduino.h>
#include "pins.h"

#define DEBOUNCE_MS      25
#define LONG_PRESS_MS     500
#define LONG_REPEAT_MS    150
#define DOUBLE_PRESS_MS   350
#define COMBO_HOLD_MS     600

enum ButtonEvent {
  EV_NONE, EV_LEFT_SHORT, EV_LEFT_LONG, EV_LEFT_LONG_REPEAT,
  EV_RIGHT_SHORT, EV_RIGHT_LONG, EV_RIGHT_LONG_REPEAT,
  EV_OK_SHORT, EV_OK_LONG, EV_OK_DOUBLE,
  EV_COMBO_SWITCH_TAB,      // LEFT+RIGHT held together
  EV_ANY_ACTIVITY           // any raw press, for the idle/sleep timer
};

class Buttons {
public:
  struct Btn {
    uint8_t pin;
    bool    state = false;      // debounced, true = pressed
    bool    raw   = false;
    uint32_t lastChange = 0;
    uint32_t pressedAt  = 0;
    bool    longFired   = false;
    uint32_t lastRepeat = 0;
    uint32_t lastReleaseAt = 0;
    bool    awaitingDouble = false;
  } left, ok, right;

  bool comboActive = false;
  uint32_t comboStart = 0;
  bool comboFired = false;

  void begin() {
    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(PIN_BTN_OK,   INPUT_PULLUP);
    pinMode(PIN_BTN_RIGHT,INPUT_PULLUP);
    left.pin = PIN_BTN_LEFT; ok.pin = PIN_BTN_OK; right.pin = PIN_BTN_RIGHT;
  }

  // Debounce one button, return true if its *stable* state changed this call
  bool updateRaw(Btn &b) {
    bool reading = (digitalRead(b.pin) == LOW); // active LOW
    if (reading != b.raw) { b.raw = reading; b.lastChange = millis(); }
    if (millis() - b.lastChange > DEBOUNCE_MS && b.state != b.raw) {
      b.state = b.raw;
      return true;
    }
    return false;
  }

  // Poll everything, return at most one event per call (call this every loop)
  ButtonEvent poll() {
    bool changedL = updateRaw(left);
    bool changedO = updateRaw(ok);
    bool changedR = updateRaw(right);
    uint32_t now = millis();

    // --- Combo: LEFT + RIGHT held together => tab switch ---
    if (left.state && right.state) {
      if (!comboActive) { comboActive = true; comboStart = now; comboFired = false; }
      if (!comboFired && now - comboStart > COMBO_HOLD_MS) {
        comboFired = true;
        return EV_COMBO_SWITCH_TAB;
      }
      return (changedL || changedO || changedR) ? EV_ANY_ACTIVITY : EV_NONE;
    } else {
      comboActive = false;
    }

    ButtonEvent ev = handleButton(left, changedL, /*isOK=*/false, true);
    if (ev != EV_NONE) return ev;
    ev = handleButton(right, changedR, false, false);
    if (ev != EV_NONE) return ev;
    ev = handleButton(ok, changedO, true, false);
    if (ev != EV_NONE) return ev;

    if (changedL || changedO || changedR) return EV_ANY_ACTIVITY;
    return EV_NONE;
  }

private:
  // isOK selects OK-specific events (double/long/short); the two bools
  // isLeft/false pick LEFT vs RIGHT event codes when isOK is false.
  ButtonEvent handleButton(Btn &b, bool changed, bool isOK, bool isLeft) {
    uint32_t now = millis();

    if (changed && b.state) {           // just pressed
      b.pressedAt = now;
      b.longFired = false;
    }

    if (b.state && !b.longFired && now - b.pressedAt > LONG_PRESS_MS) {
      b.longFired = true;
      b.lastRepeat = now;
      if (isOK) return EV_OK_LONG;
      return isLeft ? EV_LEFT_LONG : EV_RIGHT_LONG;
    }

    if (b.state && b.longFired && !isOK && now - b.lastRepeat > LONG_REPEAT_MS) {
      b.lastRepeat = now;
      return isLeft ? EV_LEFT_LONG_REPEAT : EV_RIGHT_LONG_REPEAT;
    }

    if (changed && !b.state) {          // just released
      bool wasLong = b.longFired;
      b.longFired = false;
      if (!wasLong) {
        if (isOK) {
          if (b.awaitingDouble && now - b.lastReleaseAt < DOUBLE_PRESS_MS) {
            b.awaitingDouble = false;
            return EV_OK_DOUBLE;
          }
          b.awaitingDouble = true;
          b.lastReleaseAt = now;
          // Don't fire OK_SHORT immediately — we wait a beat to see if a
          // second press turns it into OK_DOUBLE. Resolved a few lines down.
        } else {
          return isLeft ? EV_LEFT_SHORT : EV_RIGHT_SHORT;
        }
      }
    }

    // Resolve a pending single OK press once the double-press window expires
    if (isOK && b.awaitingDouble && now - b.lastReleaseAt >= DOUBLE_PRESS_MS) {
      b.awaitingDouble = false;
      return EV_OK_SHORT;
    }

    return EV_NONE;
  }
};
