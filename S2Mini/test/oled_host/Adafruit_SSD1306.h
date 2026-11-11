#pragma once
#include <Wire.h>
#include <cassert>
#include <cstring>
#define SSD1306_SWITCHCAPVCC 2
#define SSD1306_WHITE 1
inline bool displayAllocationOk = true;
class Adafruit_SSD1306 {
 public:
  Adafruit_SSD1306(int w, int h, TwoWire* wire, int rst, unsigned, unsigned) {
    assert(w == 128 && h == 64 && wire == &Wire && rst == -1);
  }
  bool begin(int, uint8_t, bool reset, bool periphBegin) {
    assert(!reset && !periphBegin);
    return displayAllocationOk;
  }
  void clearDisplay() { memset(buffer_, 0xa5, sizeof(buffer_)); }
  uint8_t* getBuffer() { return buffer_; }
  template <class... T> void setTextSize(T...) {}
  template <class... T> void setTextColor(T...) {}
  template <class... T> void setTextWrap(T...) {}
  template <class... T> void setCursor(T...) {}
  template <class... T> void print(T...) {}
  template <class... T> void drawFastHLine(T...) {}
  template <class... T> void drawRect(T...) {}
  template <class... T> void fillRect(T...) {}
 private:
  uint8_t buffer_[1024];
};
