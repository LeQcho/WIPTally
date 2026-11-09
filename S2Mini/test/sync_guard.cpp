#include "atem_sync_guard.h"
#include <cassert>
#include <cstdio>
#include <limits>

int main() {
  AtemSyncGuard guard;
  guard.reset(1000);
  // The ATEM accepts the connection and keeps sending heartbeats, but never
  // sends a usable initial tally state. This must still trigger recovery.
  for (uint32_t now = 1000; now < 31000; now += 500)
    assert(!guard.reconnectDue(now, true, false));
  assert(guard.reconnectDue(31000, true, false));
  assert(!guard.reconnectDue(31001, true, false));
  assert(guard.reconnectDue(61000, true, false));
  // A successful transfer cancels the previous failed synchronization window.
  assert(!guard.reconnectDue(62000, true, true));
  assert(!guard.reconnectDue(91000, true, false));
  assert(guard.reconnectDue(92000, true, false));
  // Do not compete with the library's existing disconnected-session retries.
  assert(!guard.reconnectDue(200000, false, false));
  assert(!guard.reconnectDue(229999, true, false));
  assert(guard.reconnectDue(230000, true, false));
  const uint32_t start = std::numeric_limits<uint32_t>::max() - 100;
  guard.reset(start);
  assert(!guard.reconnectDue(uint32_t(start + 29999), true, false));
  assert(guard.reconnectDue(uint32_t(start + 30000), true, false));
  puts("ATEM sync recovery: live heartbeats without tally, retry spacing, success, disconnect and rollover: PASS");
}
