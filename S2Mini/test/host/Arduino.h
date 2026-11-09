#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
using boolean = bool;
using byte = uint8_t;
inline uint32_t fakeMillis = 0;
inline unsigned long millis() { return fakeMillis; }
inline long random(long low, long) { static long n = 0; return low + (++n % 1000); }
inline uint16_t word(uint8_t a, uint8_t b) { return (a << 8) | b; }
inline uint8_t highByte(uint16_t x) { return x >> 8; }
inline uint8_t lowByte(uint16_t x) { return x; }
#define F(x) x
#define DEC 10
#define HEX 16
#define B00000111 7
struct SerialStub {
  template <class... T> void print(T...) {}
  template <class... T> void println(T...) {}
};
inline SerialStub Serial;
struct IPAddress {
  uint32_t value = 0;
  IPAddress() = default;
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : value((a<<24)|(b<<16)|(c<<8)|d) {}
  bool operator==(const IPAddress& b) const { return value == b.value; }
  bool operator!=(const IPAddress& b) const { return !(*this == b); }
};
