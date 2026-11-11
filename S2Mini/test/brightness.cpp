#include "brightness.h"
#include <cassert>
#include <cstdio>
#include <limits>

int main() {
  using namespace tally;
  assert(nextBrightness(64) == 130);
  assert(nextBrightness(130) == 191);
  assert(nextBrightness(191) == 255);
  assert(nextBrightness(255) == 64);
  assert(brightnessPercent(64) == 25);
  assert(brightnessPercent(130) == 50);
  assert(brightnessPercent(191) == 75);
  assert(brightnessPercent(255) == 100);

  BrightnessButton button;
  button.begin(false, 0);
  assert(!button.update(true, 10));
  assert(!button.update(false, 15)); // Switch bounce.
  assert(!button.update(true, 20));
  assert(!button.update(true, 59));
  assert(button.update(true, 60));
  assert(!button.update(true, 5000)); // A hold is one click.
  assert(!button.update(false, 5010));
  assert(!button.update(true, 5015));
  assert(!button.update(false, 5020));
  assert(!button.update(false, 5060));
  assert(!button.update(true, 5100));
  assert(button.update(true, 5140)); // A fresh click after a stable release.

  button.begin(true, 0); // Held during power-up: wait for a release.
  assert(!button.update(true, 1000));
  assert(!button.update(false, 1001));
  assert(!button.update(false, 1041));
  assert(!button.update(true, 1100));
  assert(button.update(true, 1140));

  const uint32_t wrap = std::numeric_limits<uint32_t>::max();
  button.begin(false, wrap - 25);
  assert(!button.update(true, wrap - 20));
  assert(!button.update(true, 18));
  assert(button.update(true, 19));
  puts("Brightness: cycle, debounce, held button, startup and millis rollover: PASS");
}
