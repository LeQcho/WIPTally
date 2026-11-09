#pragma once

#include <esp_wifi.h>

namespace tally {
// The station still needs the ordinary 2.4 GHz protocols for the ATEM router.
inline esp_err_t enableLongRange() {
  constexpr uint8_t protocols =
      WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR;
  return esp_wifi_set_protocol(WIFI_IF_STA, protocols);
}
}
