#pragma once
#include <stddef.h>
#include <stdint.h>

namespace sony {
enum class State : uint8_t { Unknown, Stopped, Recording };
// Reverse engineered Sony remote notification FF02. Ignore every other event,
// especially shutter/focus events which also light the official remote's LED.
inline State decode(const uint8_t* data, size_t size, State current) {
  if (!data || size != 3 || data[0] != 0x02 || data[1] != 0xd5) return current;
  if (data[2] == 0x20) return State::Recording;
  if (data[2] == 0x00) return State::Stopped;
  return current;
}
}
