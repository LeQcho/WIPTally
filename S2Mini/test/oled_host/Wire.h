#pragma once
#include <Arduino.h>
#include <vector>
#include <cstddef>

struct WireFrame { uint8_t address; std::vector<uint8_t> bytes; };
class TwoWire {
 public:
  bool starts = true;
  uint8_t screen = 0x3c;
  bool failure = false;
  unsigned timeout = 0;
  std::vector<WireFrame> frames;
  bool begin(int sda, int scl, unsigned hz) {
    return starts && sda == 33 && scl == 35 && hz == 400000;
  }
  void setTimeOut(unsigned ms) { timeout = ms; }
  void beginTransmission(uint8_t address) { current_ = {address, {}}; }
  size_t write(uint8_t value) { current_.bytes.push_back(value); return 1; }
  size_t write(const uint8_t* data, size_t length) {
    current_.bytes.insert(current_.bytes.end(), data, data + length);
    return length;
  }
  uint8_t endTransmission() {
    frames.push_back(current_);
    return failure || current_.address != screen ? 2 : 0;
  }
 private:
  WireFrame current_;
};
inline TwoWire Wire;
