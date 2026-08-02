#pragma once
#include <Preferences.h>

struct Settings {
  bool     autoplay        = true;   // auto-advance to next track at end of playlist
  uint8_t  maxVolume        = 25;     // 0-30, clamps the volume UI/knob
  uint16_t screenSleepSec   = 20;     // OLED turns off after this much idle time
  uint16_t deepSleepSec     = 60;     // deep-sleep this long after playback stops
                                       // (only counts down when nothing is playing
                                       // and autoplay didn't queue anything next)
};

class ConfigStore {
public:
  Settings s;
  Preferences prefs;

  void load() {
    prefs.begin("mp3player", true);
    s.autoplay      = prefs.getBool("autoplay", true);
    s.maxVolume     = prefs.getUChar("maxVol", 25);
    s.screenSleepSec = prefs.getUShort("scrSleep", 20);
    s.deepSleepSec  = prefs.getUShort("deepSleep", 60);
    prefs.end();
  }

  void save() {
    prefs.begin("mp3player", false);
    prefs.putBool("autoplay", s.autoplay);
    prefs.putUChar("maxVol", s.maxVolume);
    prefs.putUShort("scrSleep", s.screenSleepSec);
    prefs.putUShort("deepSleep", s.deepSleepSec);
    prefs.end();
  }
};

// Cycle helpers used by the Config screen
inline void cycleScreenSleep(uint16_t &v) {
  static const uint16_t steps[] = {10, 20, 30, 60, 120};
  for (int i = 0; i < 4; i++) if (v == steps[i]) { v = steps[i+1]; return; }
  v = steps[0];
}
inline void cycleDeepSleep(uint16_t &v) {
  static const uint16_t steps[] = {30, 60, 120, 300, 600};
  for (int i = 0; i < 4; i++) if (v == steps[i]) { v = steps[i+1]; return; }
  v = steps[0];
}
