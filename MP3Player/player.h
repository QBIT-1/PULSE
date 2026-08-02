#pragma once
#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include "pins.h"

enum RepeatMode : uint8_t { REPEAT_OFF = 0, REPEAT_ONE = 1, REPEAT_ALL = 2 };

class MP3Player {
public:
  DFRobotDFPlayerMini dfp;
  HardwareSerial mp3Serial{1};   // UART1, remapped to PIN_MP3_RX/TX

  uint16_t trackCount   = 0;     // total files detected on the SD card
  uint16_t currentTrack = 1;     // 1-based, matches DFPlayer numbering
  uint8_t  volume       = 18;    // 0-30
  RepeatMode repeat     = REPEAT_OFF;
  bool shuffle          = false;
  bool playing          = false;

  // shuffle order (a permutation of 1..trackCount), and our position in it
  uint16_t shuffleOrder[200];    // adjust size to your max expected track count
  int shufflePos = 0;

  bool begin() {
    mp3Serial.begin(9600, SERIAL_8N1, PIN_MP3_RX, PIN_MP3_TX);
    pinMode(PIN_MP3_BUSY, INPUT_PULLUP);
    if (!dfp.begin(mp3Serial, /*isACK=*/true, /*doReset=*/true)) {
      return false;
    }
    delay(200);
    trackCount = dfp.readFileCounts();
    if (trackCount == 0 || trackCount > 200) trackCount = min((int)trackCount, 200);
    dfp.volume(volume);
    buildShuffleOrder();
    return true;
  }

  void buildShuffleOrder() {
    for (uint16_t i = 0; i < trackCount; i++) shuffleOrder[i] = i + 1;
    for (int i = trackCount - 1; i > 0; i--) {
      int j = random(0, i + 1);
      uint16_t t = shuffleOrder[i]; shuffleOrder[i] = shuffleOrder[j]; shuffleOrder[j] = t;
    }
    // point shufflePos at wherever currentTrack landed, so toggling
    // shuffle mid-song doesn't jump to a random track immediately
    for (int i = 0; i < trackCount; i++) {
      if (shuffleOrder[i] == currentTrack) { shufflePos = i; break; }
    }
  }

  void playTrack(uint16_t track) {
    if (track < 1 || track > trackCount) return;
    currentTrack = track;
    dfp.play(track);
    playing = true;
  }

  void playSelected(uint16_t track) { playTrack(track); }

  void togglePlayPause() {
    if (playing) { dfp.pause(); playing = false; }
    else         { dfp.start(); playing = true; }
  }

  void stop() { dfp.stop(); playing = false; }

  void next() {
    uint16_t t = shuffle ? nextShuffleTrack() : (currentTrack % trackCount) + 1;
    playTrack(t);
  }

  void previous() {
    uint16_t t = shuffle ? prevShuffleTrack()
                          : (currentTrack == 1 ? trackCount : currentTrack - 1);
    playTrack(t);
  }

  uint16_t nextShuffleTrack() {
    shufflePos = (shufflePos + 1) % trackCount;
    if (shufflePos == 0) buildShuffleOrder(); // reshuffle each lap
    return shuffleOrder[shufflePos];
  }

  uint16_t prevShuffleTrack() {
    shufflePos = (shufflePos - 1 + trackCount) % trackCount;
    return shuffleOrder[shufflePos];
  }

  void volUp()   { if (volume < 30) volume++; dfp.volume(volume); }
  void volDown() { if (volume > 0)  volume--; dfp.volume(volume); }
  void setVolume(uint8_t v) { volume = v; dfp.volume(volume); }

  void cycleRepeat() {
    repeat = (RepeatMode)((repeat + 1) % 3);
  }

  void toggleShuffle() {
    shuffle = !shuffle;
    if (shuffle) buildShuffleOrder();
  }

  // Call this when the module reports DFPlayerPlayFinished.
  // Returns false if playback should stop entirely (no autoplay, end of
  // list, nothing left to do) so the caller can start the sleep countdown.
  bool onTrackFinished(bool autoplay) {
    if (repeat == REPEAT_ONE) { playTrack(currentTrack); return true; }
    if (repeat == REPEAT_ALL) { next(); return true; }
    if (autoplay) {
      bool lastTrack = shuffle ? false : (currentTrack == trackCount);
      if (!lastTrack || shuffle) { next(); return true; }
    }
    playing = false;
    return false;
  }
};
