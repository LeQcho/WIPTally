#include "rgb_status.h"
#include <cassert>
#include <cstdio>
void color(tally::State state, uint32_t time, int r, int g, int b) {
  const auto value = tally::rgbStatus(state, time);
  assert(value.r == r && value.g == g && value.b == b);
}
int main() {
  tally::Packet p;
  constexpr auto level = tally::kRgbLevel;
  color(tally::state(p, 2, false, 0), 0, 0, 0, level);
  p.ready = 1; p.flags[1] = 2;
  color(tally::state(p, 2, true, 0), 0, 0, level, 0);
  p.flags[1] = 3; // Program wins during compositions/transitions.
  color(tally::state(p, 2, true, 0), 0, level, 0, 0);
  color(tally::state(p, 2, true, tally::kTimeoutMs), 0, 0, 0, level);
  p.ready = 0;
  color(tally::state(p, 2, true, 0), 0, 0, 0, level);
  color(tally::state(p, 2, true, 0), 500, 0, 0, 0);
  p.ready = 1; p.flags[1] = 0;
  color(tally::state(p, 2, true, 0), 0, 0, 0, 0);
  puts("RGB: search, preview, program priority, loss timeout, ATEM offline and idle: PASS");
}
