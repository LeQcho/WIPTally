#pragma once
#include <stdint.h>

namespace tally {
constexpr uint8_t kDefaultBrightness = 130;

inline bool validBrightness(uint8_t level) {
  return level == 64 || level == 130 || level == 191 || level == 255;
}

inline uint8_t nextBrightness(uint8_t level) {
  switch (level) {
    case 64: return 130;
    case 130: return 191;
    case 191: return 255;
    case 255: return 64;
    default: return kDefaultBrightness;
  }
}

inline uint8_t brightnessPercent(uint8_t level) {
  switch (level) {
    case 64: return 25;
    case 130: return 50;
    case 191: return 75;
    case 255: return 100;
    default: return 0;
  }
}

class BrightnessButton {
 public:
  void begin(bool pressed, uint32_t now) {
    sample_ = stable_ = pressed;
    changedAt_ = now;
  }

  bool update(bool pressed, uint32_t now) {
    if (pressed != sample_) {
      sample_ = pressed;
      changedAt_ = now;
    }
    if (sample_ == stable_ || now - changedAt_ < 40) return false;
    stable_ = sample_;
    return stable_;
  }

 private:
  bool sample_ = false;
  bool stable_ = false;
  uint32_t changedAt_ = 0;
};
}
