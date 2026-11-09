#pragma once
#include "Arduino.h"
#include <deque>
#include <vector>
class WiFiUDP {
public:
  using Bytes = std::vector<uint8_t>;
  struct Datagram { Bytes bytes; IPAddress ip; uint16_t port; };
  inline static std::deque<Datagram> incoming;
  inline static std::vector<Bytes> outgoing;
  inline static unsigned begins = 0;
  Datagram current;
  size_t offset = 0;
  Bytes tx;
  void stop() { current = {}; offset = 0; incoming.clear(); }
  int begin(uint16_t) { ++begins; stop(); return 1; }
  int parsePacket() {
    // Arduino ESP32 retains unread bytes until consumed. Faithfully model it.
    if (available()) return 0;
    if (incoming.empty()) return 0;
    current = incoming.front(); incoming.pop_front(); offset = 0;
    return current.bytes.size();
  }
  int available() { return current.bytes.size() - offset; }
  int read(uint8_t* out, size_t size) {
    const size_t count = std::min(size, size_t(available()));
    std::copy_n(current.bytes.data()+offset, count, out); offset += count; return count;
  }
  IPAddress remoteIP() { return current.ip; }
  uint16_t remotePort() { return current.port; }
  int beginPacket(IPAddress, uint16_t) { tx.clear(); return 1; }
  size_t write(const uint8_t* p, size_t n) { tx.insert(tx.end(), p, p+n); return n; }
  int endPacket() { outgoing.push_back(tx); return 1; }
  static void receive(Bytes b, IPAddress ip = IPAddress(192,168,1,196), uint16_t port=9910) {
    incoming.push_back({b,ip,port});
  }
};
