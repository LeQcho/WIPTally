// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#include <string.h>

// Tracks received initial state packets. Packet zero is the hello handshake.
template <uint16_t Capacity>
class ATEMInitialization {
  static_assert(Capacity > 1 && Capacity <= 32768, "Invalid packet capacity");
  uint8_t received_[(Capacity + 7) / 8] = {};
public:
  void reset() { memset(received_, 0, sizeof(received_)); }
  void received(uint16_t packet) {
    if (packet < Capacity) received_[packet >> 3] |= uint8_t(1U << (packet & 7));
  }
  uint16_t firstMissing(uint16_t endExclusive) const {
    const uint16_t end = endExclusive < Capacity ? endExclusive : Capacity;
    for (uint16_t packet = 1; packet < end; ++packet)
      if (!(received_[packet >> 3] & uint8_t(1U << (packet & 7)))) return packet;
    return 0;
  }
};
