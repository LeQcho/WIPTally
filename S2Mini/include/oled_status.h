#pragma once
#include <initializer_list>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "oled_pages.h"

namespace tally {
constexpr uint8_t kOledSda = 33, kOledScl = 35;

class OledStatus {
 public:
  void begin(uint32_t now, uint8_t atem = kDefaultOledSeconds,
      uint8_t wifi = kDefaultOledSeconds, uint8_t led = kDefaultOledSeconds,
      uint8_t receivers = kDefaultOledSeconds) {
    pages_.begin(now, atem, wifi, led, receivers);
    if (!Wire.begin(kOledSda, kOledScl, 400000)) return;
    Wire.setTimeOut(5);
    for (uint8_t address : {uint8_t(0x3c), uint8_t(0x3d)}) {
      Wire.beginTransmission(address);
      if (Wire.endTransmission() == 0) { address_ = address; break; }
    }
    if (!address_) { Serial.println("OLED absent; tally actif sans ecran"); return; }
    // Wire is already configured. No RESET pin and no splash screen transfer.
    active_ = display_.begin(SSD1306_SWITCHCAPVCC, address_, false, false);
    if (active_) Serial.printf("OLED SSD1306 128x64 adresse=0x%02X SDA=%u SCL=%u\n", address_, kOledSda, kOledScl);
    else Serial.println("OLED: initialisation impossible; tally actif sans ecran");
  }

  void tick(uint32_t now, const OledInfo& info) {
    if (!active_) return;
    if (sending_) { sendChunk(); return; }
    if (rendered_ && now - refreshedAt_ < kOledRefreshMs) return;
    refreshedAt_ = now;
    const OledView view = oledView(pages_.update(now), info);
    if (rendered_ && view == previous_) return;
    previous_ = view;
    rendered_ = true;
    render(view);
    // SSD1306 horizontal addressing: a full frame, sent over later loop turns.
    const uint8_t window[] = {0x00, 0x21, 0, 127, 0x22, 0, 7};
    Wire.beginTransmission(address_);
    Wire.write(window, sizeof(window));
    if (!finish()) return;
    offset_ = 0;
    sending_ = true;
  }

 private:
  Adafruit_SSD1306 display_{128, 64, &Wire, -1, 400000, 400000};
  OledPages pages_;
  OledView previous_;
  uint32_t refreshedAt_ = 0;
  uint16_t offset_ = 0;
  uint8_t address_ = 0;
  bool active_ = false, sending_ = false, rendered_ = false;

  bool finish() {
    if (Wire.endTransmission() == 0) return true;
    // A failed/disconnected screen is disabled until reboot, rather than
    // imposing repeated I2C timeouts on the radio/ATEM loop.
    active_ = sending_ = false;
    Serial.println("OLED: erreur I2C; affichage arrete, tally actif");
    return false;
  }

  void sendChunk() {
    const uint16_t count = 1024 - offset_ < 31 ? 1024 - offset_ : 31;
    Wire.beginTransmission(address_);
    Wire.write(uint8_t(0x40));
    Wire.write(display_.getBuffer() + offset_, count);
    if (!finish()) return;
    offset_ += count;
    sending_ = offset_ < 1024;
  }

  void text(uint8_t y, const char* value, uint8_t size = 1) {
    display_.setTextSize(size);
    display_.setCursor(0, y);
    display_.print(value);
  }

  void render(const OledView& view) {
    display_.clearDisplay();
    display_.setTextColor(SSD1306_WHITE);
    display_.setTextWrap(false);
    text(0, view.title);
    display_.drawFastHLine(0, 12, 128, SSD1306_WHITE);
    if (view.page >= 2) {
      text(18, view.line1, 3);
      if (view.page == 2) {
        display_.drawRect(0, 46, 128, 7, SSD1306_WHITE);
        display_.fillRect(2, 48, 124 * view.percent / 100, 3, SSD1306_WHITE);
      } else text(46, view.line2);
    } else {
      text(20, view.line1);
      text(33, view.line2);
      text(46, view.detail);
    }
    text(57, view.page >= 2 ? view.detail : "TALLY MASTER");
    display_.setCursor(110, 57);
    display_.print(view.page + 1);
    display_.print("/4");
  }
};
}
