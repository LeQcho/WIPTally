#include "oled_status.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <string>

int main() {
  using namespace tally;
  OledPages pages;
  pages.begin(500);
  assert(pages.update(500) == 0);
  assert(pages.update(15499) == 0);
  assert(pages.update(15500) == 1);
  assert(pages.update(30500) == 2);
  assert(pages.update(45500) == 0);
  assert(pages.update(90500) == 0); // Multiple skipped pages preserve cadence.
  const uint32_t start = std::numeric_limits<uint32_t>::max() - 4000;
  pages.begin(start);
  assert(pages.update(start + 14999) == 0);
  assert(pages.update(start + 15000) == 1);
  assert(pages.update(start + 45000) == 0);

  OledInfo info{false, false, false, 130, "192.0.2.196", "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456"};
  auto view = oledView(0, info);
  assert(!strcmp(view.line1, "192.0.2.196"));
  assert(!strcmp(view.line2, "DECONNECTE"));
  info.wifi = info.atemReady = true;
  assert(!strcmp(oledView(0, info).line2, "CONNECTE"));
  info.demo = true;
  assert(!strcmp(oledView(0, info).line2, "MODE DEMO"));
  view = oledView(1, info);
  assert(strlen(view.line1) == 21 && strlen(view.line2) == 11);
  assert(std::string(view.line1) + view.line2 == info.ssid);
  for (auto level : {64, 130, 191, 255}) {
    info.brightness = level;
    assert(oledView(2, info).percent == brightnessPercent(level));
  }
  char text[40];
  oledText("R\xc3\xa9gie \xf0\x9f\x8e\xa5", text, sizeof(text));
  assert(!strcmp(text, "Regie ?"));
  oledText("\xc3", text, sizeof(text)); assert(!strcmp(text, "?"));
  oledText("abcdef", text, 4); assert(!strcmp(text, "abc"));
  oledText(nullptr, text, sizeof(text)); assert(!*text);

  // Missing screen: boot probes only, no further I2C work on any loop turn.
  Wire = TwoWire{}; Wire.screen = 0;
  OledStatus absent; absent.begin(0);
  assert(Wire.frames.size() == 2 && Wire.timeout == 5);
  for (uint32_t n = 0; n < 100000; n += 10) absent.tick(n, info);
  assert(Wire.frames.size() == 2);

  // Detect 0x3D, then verify a complete frame is split across loop turns.
  Wire = TwoWire{}; Wire.screen = 0x3d;
  OledStatus display; display.begin(0);
  assert(Wire.frames.size() == 2);
  display.tick(0, info);
  const auto& window = Wire.frames.back();
  assert(window.address == 0x3d);
  assert((window.bytes == std::vector<uint8_t>{0, 0x21, 0, 127, 0x22, 0, 7}));
  size_t total = 0;
  for (uint32_t now = 1; now < 100; ++now) {
    const auto count = Wire.frames.size();
    display.tick(now, info);
    assert(Wire.frames.size() <= count + 1);
    if (Wire.frames.size() == count) continue;
    const auto& frame = Wire.frames.back();
    assert(frame.bytes.size() <= 32 && frame.bytes[0] == 0x40);
    for (size_t i = 1; i < frame.bytes.size(); ++i) assert(frame.bytes[i] == 0xa5);
    total += frame.bytes.size() - 1;
  }
  assert(total == 1024);
  auto count = Wire.frames.size();
  display.tick(1000, info); // Same page and data: no unnecessary refresh.
  assert(Wire.frames.size() == count);
  display.tick(30000, info); // Brightness page.
  for (uint32_t now = 30001; now < 30100; ++now) display.tick(now, info);
  count = Wire.frames.size();
  info.brightness = 64;
  display.tick(30250, info); // Intensity changes refresh without changing page.
  assert(Wire.frames.size() == count + 1);
  Wire.failure = true; display.tick(30251, info);
  count = Wire.frames.size();
  for (uint32_t now = 30252; now < 100000; now += 10) display.tick(now, info);
  assert(Wire.frames.size() == count); // A NACK disables only the OLED.

  Wire = TwoWire{}; displayAllocationOk = false;
  OledStatus noMemory; noMemory.begin(0);
  count = Wire.frames.size(); noMemory.tick(0, info);
  assert(Wire.frames.size() == count);
  puts("OLED: pages, live status, UTF-8, wrap, absent screen, incremental I2C and failure isolation: PASS");
}
