#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "brightness.h"

namespace tally {
constexpr uint8_t kOledPageCount = 4;
constexpr uint8_t kDefaultOledSeconds = 5;
inline uint8_t oledSeconds(uint8_t seconds) {
  return seconds >= 1 && seconds <= 120 ? seconds : kDefaultOledSeconds;
}
constexpr uint32_t kOledRefreshMs = 250;

class OledPages {
 public:
  void begin(uint32_t now, uint8_t atem = kDefaultOledSeconds,
      uint8_t wifi = kDefaultOledSeconds, uint8_t led = kDefaultOledSeconds,
      uint8_t receivers = kDefaultOledSeconds) {
    const uint8_t seconds[] = {atem, wifi, led, receivers};
    cycle_ = 0;
    for (uint8_t i = 0; i < kOledPageCount; ++i) {
      durations_[i] = uint32_t(oledSeconds(seconds[i])) * 1000;
      cycle_ += durations_[i];
    }
    since_ = now; page_ = 0;
  }
  uint8_t update(uint32_t now) {
    since_ += ((now - since_) / cycle_) * cycle_;
    while (now - since_ >= durations_[page_]) {
      since_ += durations_[page_];
      page_ = (page_ + 1) % kOledPageCount;
    }
    return page_;
  }
 private:
  uint32_t since_ = 0;
  uint8_t page_ = 0;
  uint32_t durations_[kOledPageCount] = {5000, 5000, 5000, 5000};
  uint32_t cycle_ = 20000;
};

// The built-in font is ASCII. Keep common French accents readable; consume
// other UTF-8 codepoints as one '?' rather than showing raw UTF-8 bytes.
inline void oledText(const char* source, char* target, size_t capacity) {
  if (!capacity) return;
  size_t n = 0;
  const auto* p = reinterpret_cast<const unsigned char*>(source ? source : "");
  while (*p && n + 1 < capacity) {
    unsigned char c = *p++;
    if (c >= 32 && c <= 126) { target[n++] = c; continue; }
    char replacement = '?';
    if (c == 0xc3 && *p >= 0x80 && *p <= 0xbf) {
      const unsigned char accent = *p;
      if (accent >= 0xa0 && accent <= 0xa5) replacement = 'a';
      else if (accent == 0xa7) replacement = 'c';
      else if (accent >= 0xa8 && accent <= 0xab) replacement = 'e';
      else if (accent >= 0xac && accent <= 0xaf) replacement = 'i';
      else if (accent >= 0xb2 && accent <= 0xb6) replacement = 'o';
      else if (accent >= 0xb9 && accent <= 0xbc) replacement = 'u';
      else if (accent >= 0x80 && accent <= 0x85) replacement = 'A';
      else if (accent == 0x87) replacement = 'C';
      else if (accent >= 0x88 && accent <= 0x8b) replacement = 'E';
    }
    if (c >= 0xc0) while ((*p & 0xc0) == 0x80) ++p;
    target[n++] = replacement;
  }
  target[n] = '\0';
}

struct OledInfo {
  bool wifi, atemReady, demo;
  uint8_t brightness;
  const char* atemIp;
  const char* ssid;
  uint8_t receivers;
  bool presenceReady;
};

struct OledView {
  uint8_t page = 0, percent = 0;
  char title[22] = {}, line1[22] = {}, line2[22] = {}, detail[22] = {};
  bool operator==(const OledView& other) const {
    return page == other.page && percent == other.percent &&
        !strcmp(title, other.title) && !strcmp(line1, other.line1) &&
        !strcmp(line2, other.line2) && !strcmp(detail, other.detail);
  }
};

inline OledView oledView(uint8_t page, const OledInfo& info) {
  OledView view;
  view.page = page % kOledPageCount;
  switch (view.page) {
    case 0:
      strcpy(view.title, "ATEM");
      oledText(info.atemIp && *info.atemIp ? info.atemIp : "IP non configuree", view.line1, sizeof(view.line1));
      strcpy(view.line2, info.demo ? "MODE DEMO" : info.atemReady ? "CONNECTE" : "DECONNECTE");
      strcpy(view.detail, info.demo ? "ATEM simule" : !info.wifi ? "Wi-Fi indisponible" : info.atemReady ? "Tally disponible" : "Attente de l'ATEM");
      break;
    case 1: {
      strcpy(view.title, "WI-FI");
      char name[33];
      oledText(info.ssid && *info.ssid ? info.ssid : "Non configure", name, sizeof(name));
      // 21 characters at 6 pixels each; a 32-byte SSID fits on two lines.
      const size_t first = strlen(name) < 21 ? strlen(name) : 21;
      memcpy(view.line1, name, first);
      strcpy(view.line2, name + first);
      strcpy(view.detail, info.wifi ? "CONNECTE" : "DECONNECTE");
      break;
    }
    case 2:
      strcpy(view.title, "LUMINOSITE LED");
      view.percent = brightnessPercent(info.brightness);
      snprintf(view.line1, sizeof(view.line1), "%u %%", view.percent);
      strcpy(view.detail, "Reglage du master");
      break;
    default:
      strcpy(view.title, "RECEPTEURS");
      if (info.presenceReady) snprintf(view.line1, sizeof(view.line1), "%u", info.receivers);
      else strcpy(view.line1, "--");
      strcpy(view.line2, info.presenceReady ? "ACTIFS" : "INDISPONIBLE");
      strcpy(view.detail, "ESP-NOW");
      break;
  }
  return view;
}
}
