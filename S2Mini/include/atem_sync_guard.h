// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>

// Traffic alone does not prove that the initial ATEM state transfer completed.
class AtemSyncGuard {
  uint32_t since_ = 0;
public:
  static constexpr uint32_t timeoutMs = 30000;
  void reset(uint32_t now) { since_ = now; }
  bool reconnectDue(uint32_t now, bool connected, bool ready) {
    if (!connected || ready) {
      since_ = now;
      return false;
    }
    if (uint32_t(now - since_) < timeoutMs) return false;
    since_ = now;
    return true;
  }
};
