#pragma once
#include "tally.h"

namespace tally {
// Pure mapping shared by both pixels.
struct Rgb { uint8_t r, g, b; };
constexpr uint8_t kRgbDataPin = 18;
constexpr uint8_t kRgbPixels = 2;
constexpr uint8_t kDefaultRgbLevel = kDefaultBrightness;
inline Rgb rgbStatus(State state, uint32_t now, uint8_t level) {
  switch (state) {
    case State::Program: return {level, 0, 0};
    case State::Preview: return {0, level, 0};
    case State::Lost: return {0, 0, level};
    case State::Offline: return {0, 0, uint8_t((now / 500) % 2 ? 0 : level)};
    case State::Idle: return {0, 0, 0};
  }
  return {0, 0, 0};
}
}
