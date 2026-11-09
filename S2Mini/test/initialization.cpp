#include "ATEMInitialization.h"
#include <cassert>
#include <cstdio>

int main() {
  ATEMInitialization<512> tracker;
  // A larger mixer transfer with a lost packet at the old 40-packet boundary.
  for (uint16_t packet = 1; packet < 400; ++packet)
    if (packet != 40 && packet != 300) tracker.received(packet);
  assert(tracker.firstMissing(400) == 40);
  tracker.received(40);
  assert(tracker.firstMissing(400) == 300);
  tracker.received(300);
  assert(tracker.firstMissing(400) == 0);
  // Duplicate packets and out-of-range IDs cannot corrupt the bitmap.
  tracker.received(300);
  tracker.received(512);
  tracker.received(65535);
  assert(tracker.firstMissing(400) == 0);
  assert(tracker.firstMissing(512) == 400);
  for (uint16_t packet = 400; packet < 512; ++packet) tracker.received(packet);
  assert(tracker.firstMissing(512) == 0);
  assert(tracker.firstMissing(65535) == 0);
  tracker.reset();
  assert(tracker.firstMissing(0) == 0);
  assert(tracker.firstMissing(1) == 0);
  assert(tracker.firstMissing(400) == 1);
  // Exact former boundary is also safe for the original capacity.
  ATEMInitialization<40> small;
  for (uint16_t packet = 1; packet < 40; ++packet) small.received(packet);
  small.received(40);
  assert(small.firstMissing(41) == 0);
  puts("ATEM initialization: lost packets 40/300, 16-bit IDs, bounds and reconnect reset: PASS");
}
