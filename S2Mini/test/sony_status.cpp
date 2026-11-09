#include <assert.h>
#include <initializer_list>
#include "sony_status.h"
int main() {
  using sony::State;
  const uint8_t rec[] = {2, 0xd5, 0x20}, stop[] = {2, 0xd5, 0};
  const uint8_t shutter[] = {2, 0xa0, 0x20}, focus[] = {2, 0x3f, 0x20};
  const uint8_t invalid[] = {2, 0xd5, 1}, oversized[] = {2, 0xd5, 0, 0};
  assert(sony::decode(rec, 3, State::Unknown) == State::Recording);
  assert(sony::decode(stop, 3, State::Recording) == State::Stopped);
  for (auto previous : {State::Unknown, State::Stopped, State::Recording}) {
    assert(sony::decode(nullptr, 3, previous) == previous);
    assert(sony::decode(rec, 2, previous) == previous);
    assert(sony::decode(oversized, 4, previous) == previous);
    assert(sony::decode(shutter, 3, previous) == previous);
    assert(sony::decode(focus, 3, previous) == previous);
    assert(sony::decode(invalid, 3, previous) == previous);
  }
}
