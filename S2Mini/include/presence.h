#pragma once
#include "tally.h"

namespace tally {
constexpr uint32_t kPresenceMagic = 0x31525854;
constexpr uint32_t kPresenceExpiryMs = 8000;
constexpr uint8_t kMaxReceivers = 64;

// Separate message type: the 55-byte master tally packet stays unchanged.
struct __attribute__((packed)) ReceiverReport {
  uint32_t magic = kPresenceMagic;
  uint8_t version = 1;
  uint16_t group = kGroup;
  uint8_t camera = 1;
};
static_assert(sizeof(ReceiverReport) == 8, "Presence wire format changed");
inline bool decodeReport(const uint8_t* bytes, size_t size, ReceiverReport& out) {
  if (!bytes || size != sizeof(ReceiverReport)) return false;
  ReceiverReport report;
  memcpy(&report, bytes, size);
  if (report.magic != kPresenceMagic || report.version != 1 ||
      report.group != kGroup || report.camera < 1 || report.camera > kInputs) return false;
  out = report;
  return true;
}

class ReceiverPresence {
 public:
  bool record(const uint8_t* mac, uint8_t camera, uint32_t now) {
    if (!mac || (mac[0] & 1) || camera < 1 || camera > kInputs) return false;
    uint8_t any = 0;
    for (uint8_t i = 0; i < 6; ++i) any |= mac[i];
    if (!any) return false;
    Entry* available = nullptr;
    for (auto& entry : entries_) {
      if (entry.used && !memcmp(entry.mac, mac, 6)) {
        entry.lastSeen = now;
        return true;
      }
      if (!entry.used || now - entry.lastSeen >= kPresenceExpiryMs) available = &entry;
    }
    if (!available) return false;
    memcpy(available->mac, mac, 6);
    available->lastSeen = now;
    available->used = true;
    return true;
  }
  uint8_t count(uint32_t now) {
    uint8_t total = 0;
    for (auto& entry : entries_) {
      if (entry.used && now - entry.lastSeen >= kPresenceExpiryMs) entry.used = false;
      if (entry.used) ++total;
    }
    return total;
  }
 private:
  struct Entry { uint8_t mac[6] = {}; uint32_t lastSeen = 0; bool used = false; };
  Entry entries_[kMaxReceivers];
};

class PresenceTimer {
 public:
  void begin(uint32_t seed) { jitter_ = seed % 500; linked_ = false; }
  bool due(uint32_t now, bool linked) {
    if (!linked) { linked_ = false; return false; }
    if (!linked_) { linked_ = true; since_ = now; wait_ = jitter_; }
    if (now - since_ < wait_) return false;
    since_ = now;
    wait_ = 1500 + jitter_;
    return true;
  }
 private:
  uint32_t since_ = 0;
  uint16_t wait_ = 0, jitter_ = 0;
  bool linked_ = false;
};
}
