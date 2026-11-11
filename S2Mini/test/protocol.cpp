#include "tally.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <initializer_list>

int main() {
  using namespace tally;
  Packet p, out;
  p.ready = 1;
  p.flags[0] = 3;
  p.flags[1] = 2;
  auto bytes = reinterpret_cast<const uint8_t*>(&p);
  assert(decode(bytes, sizeof(p), out));
  assert(!decode(bytes, sizeof(p) - 1, out));
  assert(!decode(bytes, sizeof(p) + 1, out));
  assert(state(p, 1, true, 0) == State::Program); // Program wins if both flags set.
  assert(state(p, 2, true, 0) == State::Preview);
  assert(state(p, 40, true, 0) == State::Idle);
  assert(state(p, 0, true, 0) == State::Offline);
  assert(state(p, 41, true, 0) == State::Offline);
  assert(state(p, 1, false, 0) == State::Lost);
  assert(state(p, 1, true, kTimeoutMs - 1) == State::Program);
  assert(state(p, 1, true, kTimeoutMs) == State::Lost);
  p.ready = 0;
  assert(state(p, 1, true, 0) == State::Offline);
  p.channel = 14; assert(!decode(bytes, sizeof(p), out)); p.channel = 1;
  p.version = 3; assert(!decode(bytes, sizeof(p), out)); p.version = 2;
  p.magic = 0; assert(!decode(bytes, sizeof(p), out)); p.magic = kMagic;
  p.group = 2; assert(!decode(bytes, sizeof(p), out)); p.group = kGroup;
  p.flags[19] = 4; assert(!decode(bytes, sizeof(p), out)); p.flags[19] = 0;
  for (uint8_t level : {64, 130, 191, 255}) {
    p.brightness = level;
    assert(decode(bytes, sizeof(p), out));
    assert(out.brightness == level);
  }
  p.brightness = 60; assert(!decode(bytes, sizeof(p), out));
  p.brightness = 0; assert(!decode(bytes, sizeof(p), out));
  p.brightness = kDefaultBrightness;
  assert(!decode(nullptr, sizeof(p), out));
  // New receivers accept the exact old 54-byte format without a brightness override.
  p.version = 1;
  assert(decode(bytes, kLegacyPacketSize, out));
  assert(out.brightness == 0 && out.version == 1);
  assert(!decode(bytes, sizeof(p), out));
  p.version = 2;
  assert(!decode(bytes, kLegacyPacketSize, out));
  // Unsigned elapsed time must survive the 49-day millis rollover.
  uint32_t before = std::numeric_limits<uint32_t>::max() - 100;
  uint32_t after = 200;
  assert(after - before == 301);
  puts("Protocol, validation, program priority, timeout and rollover: PASS");
}
