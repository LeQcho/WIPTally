#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "brightness.h"

namespace tally {
constexpr uint8_t kInputs = 40; // Limit of the bundled ATEMmin TlIn parser.
constexpr uint32_t kMagic = 0x32594C54;
constexpr uint16_t kGroup = 1; // Change in both firmwares for another rig.
constexpr uint32_t kHeartbeatMs = 250;
constexpr uint32_t kTimeoutMs = 1800;
struct __attribute__((packed)) Packet {
  uint32_t magic = kMagic;
  uint8_t version = 2;
  uint16_t group = kGroup;
  uint32_t sequence = 0;
  uint8_t ready = 0;
  uint8_t demo = 0;
  uint8_t channel = 1;
  uint8_t flags[kInputs] = {}; // bit 0 program, bit 1 preview
  uint8_t brightness = kDefaultBrightness;
};
constexpr size_t kLegacyPacketSize = offsetof(Packet, brightness);
static_assert(kLegacyPacketSize == 54 && sizeof(Packet) == 55, "Wire format changed");
inline bool decode(const uint8_t* bytes, size_t size, Packet& out) {
  if (!bytes || (size != sizeof(Packet) && size != kLegacyPacketSize)) return false;
  Packet p;
  memcpy(&p, bytes, size);
  if (size == kLegacyPacketSize) {
    if (p.version != 1) return false;
    p.brightness = 0; // An old master leaves the receiver's local fallback in use.
  } else if (p.version != 2 || !validBrightness(p.brightness)) {
    return false;
  }
  if (p.magic != kMagic || p.group != kGroup ||
      p.channel < 1 || p.channel > 13 || p.ready > 1 || p.demo > 1) return false;
  for (auto f : p.flags) if (f > 3) return false;
  out = p;
  return true;
}
enum class State { Lost, Offline, Idle, Preview, Program };
inline State state(const Packet& p, uint8_t input, bool seen, uint32_t age) {
  if (!seen || age >= kTimeoutMs) return State::Lost;
  if (!p.ready || input < 1 || input > kInputs) return State::Offline;
  const uint8_t f = p.flags[input - 1];
  return f & 1 ? State::Program : f & 2 ? State::Preview : State::Idle;
}
}
