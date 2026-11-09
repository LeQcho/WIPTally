#pragma once
#include "tally.h"

namespace tally {
// Pure mapping shared by both pixels. 64/255 initial brightness per channel.
struct Rgb { uint8_t r, g, b; };
constexpr uint8_t kRgbDataPin = 18;
constexpr uint8_t kRgbPixels = 2;
constexpr uint8_t kRgbLevel = 64;
inline Rgb rgbStatus(State state, uint32_t now) {
  switch (state) {
    case State::Program: return {kRgbLevel, 0, 0};
    case State::Preview: return {0, kRgbLevel, 0};
    case State::Lost: return {0, 0, kRgbLevel};
    case State::Offline: return {0, 0, uint8_t((now / 500) % 2 ? 0 : kRgbLevel)};
    case State::Idle: return {0, 0, 0};
  }
  return {0, 0, 0};
}
}
