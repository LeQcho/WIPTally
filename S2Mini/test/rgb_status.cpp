#include "rgb_status.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
void color(tally::State state, uint32_t time, uint8_t level, int r, int g, int b) {
  const auto value = tally::rgbStatus(state, time, level);
  assert(value.r == r && value.g == g && value.b == b);
}
void checkLevel(uint8_t level) {
  tally::Packet p;
  color(tally::state(p, 2, false, 0), 0, level, 0, 0, level);
  p.ready = 1; p.flags[1] = 2;
  color(tally::state(p, 2, true, 0), 0, level, 0, level, 0);
  p.flags[1] = 3; // Program wins during compositions/transitions.
  color(tally::state(p, 2, true, 0), 0, level, level, 0, 0);
  color(tally::state(p, 2, true, tally::kTimeoutMs), 0, level, 0, 0, level);
  p.ready = 0;
  color(tally::state(p, 2, true, 0), 0, level, 0, 0, level);
  color(tally::state(p, 2, true, 0), 500, level, 0, 0, 0);
  p.ready = 1; p.flags[1] = 0;
  color(tally::state(p, 2, true, 0), 0, level, 0, 0, 0);
}

int main() {
  assert(tally::kDefaultRgbLevel == 130);
  for (uint8_t level : {64, 130, 191, 255}) checkLevel(level);
  puts("RGB: four levels, search, preview, program priority, timeout, offline and idle: PASS");
}
