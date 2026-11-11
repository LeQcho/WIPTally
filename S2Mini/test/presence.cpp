#include "presence.h"
#include <cassert>
#include <cstdio>
#include <limits>

int main() {
  using namespace tally;
  ReceiverReport report, decoded;
  const auto* bytes = reinterpret_cast<const uint8_t*>(&report);
  assert(decodeReport(bytes, sizeof(report), decoded));
  assert(!decodeReport(nullptr, sizeof(report), decoded));
  assert(!decodeReport(bytes, sizeof(report) - 1, decoded));
  assert(!decodeReport(bytes, sizeof(report) + 1, decoded));
  report.magic = 0; assert(!decodeReport(bytes, sizeof(report), decoded)); report.magic = kPresenceMagic;
  report.version = 2; assert(!decodeReport(bytes, sizeof(report), decoded)); report.version = 1;
  report.group = 2; assert(!decodeReport(bytes, sizeof(report), decoded)); report.group = kGroup;
  report.camera = 0; assert(!decodeReport(bytes, sizeof(report), decoded));
  report.camera = 41; assert(!decodeReport(bytes, sizeof(report), decoded));
  report.camera = 40; assert(decodeReport(bytes, sizeof(report), decoded));
  Packet tally, out;
  assert(!decode(bytes, sizeof(report), out));
  assert(!decodeReport(reinterpret_cast<const uint8_t*>(&tally), sizeof(tally), decoded));

  ReceiverPresence registry;
  uint8_t a[6] = {2, 1, 2, 3, 4, 5}, b[6] = {2, 1, 2, 3, 4, 6};
  uint8_t zero[6] = {}, broadcast[6] = {255,255,255,255,255,255};
  assert(!registry.record(zero, 1, 0));
  assert(!registry.record(broadcast, 1, 0));
  assert(!registry.record(nullptr, 1, 0));
  assert(!registry.record(a, 0, 0));
  assert(registry.count(0) == 0);
  assert(registry.record(a, 1, 0));
  assert(registry.record(b, 1, 100)); // Two devices, same camera: count both.
  assert(registry.count(100) == 2);
  assert(registry.record(a, 2, 1000)); // Same device, changed camera: count once.
  assert(registry.count(1000) == 2);
  assert(registry.count(8099) == 2);
  assert(registry.count(8100) == 1);
  assert(registry.count(9000) == 0);
  assert(registry.record(a, 1, 9001)); // Returns after a power cycle.
  assert(registry.count(9001) == 1);

  const uint32_t wrap = std::numeric_limits<uint32_t>::max() - 4000;
  ReceiverPresence wrapping;
  assert(wrapping.record(a, 1, wrap));
  assert(wrapping.count(wrap + 7999) == 1);
  assert(wrapping.count(wrap + 8000) == 0);

  ReceiverPresence full;
  for (uint8_t i = 0; i < kMaxReceivers; ++i) {
    a[5] = i; assert(full.record(a, 1, 0));
  }
  assert(full.count(0) == kMaxReceivers);
  a[5] = 100;
  assert(!full.record(a, 1, 0));
  assert(full.record(a, 1, 8000)); // Expired slot is reusable without a count().
  assert(full.count(8000) == 1);

  PresenceTimer timer;
  timer.begin(250);
  assert(!timer.due(0, false));
  assert(!timer.due(100, true));
  assert(!timer.due(349, true));
  assert(timer.due(350, true));
  assert(!timer.due(2099, true));
  assert(timer.due(2100, true));
  assert(!timer.due(2200, false)); // Stop reporting if master is no longer heard.
  assert(!timer.due(50000, false));
  assert(!timer.due(51000, true));
  assert(timer.due(51250, true));
  timer.begin(0);
  assert(timer.due(wrap, true));
  assert(!timer.due(wrap + 1499, true));
  assert(timer.due(wrap + 1500, true));
  puts("Presence: validation, MAC identity, duplicate cameras, expiry, capacity, reconnect, jitter and wrap: PASS");
}
