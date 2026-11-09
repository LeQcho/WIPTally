#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "tally.h"
#ifdef TALLY_WS2812
#include <Adafruit_NeoPixel.h>
#include "rgb_status.h"
Adafruit_NeoPixel pixels(tally::kRgbPixels, tally::kRgbDataPin, NEO_RGB + NEO_KHZ800);
uint32_t displayedRgb = UINT32_MAX;
void showRgb(tally::State state, uint32_t now) {
  const auto color = tally::rgbStatus(state, now);
  const uint32_t packed = pixels.Color(color.r, color.g, color.b);
  if (packed == displayedRgb) return;
  for (uint8_t i = 0; i < tally::kRgbPixels; ++i) pixels.setPixelColor(i, packed);
  pixels.show(); // Two pixels; only refresh when the displayed color changes.
  displayedRgb = packed;
}
#endif

// Discrete LEDs: GPIO -> 330 ohm -> LED anode; cathode -> GND.
constexpr uint8_t kRed = 16, kGreen = 17, kStatus = 15;
Preferences prefs;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
tally::Packet inbox, current;
bool pending = false, seen = false;
uint8_t camera = 1, channel = 1;
uint32_t receivedAt = 0, hopAt = 0, buttonAt = 0;
bool buttonHandled = false;
// Optional master MAC lock (all zeros = accept any master in kGroup).
const uint8_t trustedMaster[6] = {0,0,0,0,0,0};

void onReceive(const uint8_t* mac, const uint8_t* bytes, int size) {
  if (trustedMaster[0] || trustedMaster[1] || trustedMaster[2] || trustedMaster[3] || trustedMaster[4] || trustedMaster[5])
    if (memcmp(mac, trustedMaster, 6)) return;
  tally::Packet packet;
  if (size < 0 || !tally::decode(bytes, size, packet)) return;
  // Callback runs in Wi-Fi task: defer LEDs, logging and flash writes to loop().
  portENTER_CRITICAL(&mux);
  inbox = packet;
  receivedAt = millis();
  pending = true;
  portEXIT_CRITICAL(&mux);
}

void setCamera(uint8_t input) {
  if (input < 1 || input > tally::kInputs) return;
  camera = input;
  prefs.putUChar("camera", camera);
  Serial.printf("Camera=%u (sauvegardee)\n", camera);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(kRed, OUTPUT); pinMode(kGreen, OUTPUT); pinMode(kStatus, OUTPUT);
  pinMode(0, INPUT_PULLUP);
  prefs.begin("tally-rx", false);
  camera = prefs.getUChar("camera", 1);
  if (camera < 1 || camera > tally::kInputs) camera = 1;
#ifdef TALLY_WS2812
  // This rig's pixels swap red/green with GRB. Allow standard GRB modules too.
  const bool rgbOrder = prefs.getUChar("rgb_order", 1) != 0;
  pixels.updateType((rgbOrder ? NEO_RGB : NEO_GRB) + NEO_KHZ800);
  pixels.begin();
  showRgb(tally::State::Lost, millis());
  Serial.printf("RGB WS2812: %u pixels, GPIO%u, intensite %u/255\n",
      tally::kRgbPixels, tally::kRgbDataPin, tally::kRgbLevel);
  Serial.printf("RGB ordre=%s fw=receiver-rgb-2\n", rgbOrder ? "RGB" : "GRB");
#endif
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  // Country CH permits channels 1..13; receiver never associates to the router.
  wifi_country_t country = {"CH", 1, 13, 20, WIFI_COUNTRY_POLICY_MANUAL};
  esp_wifi_set_country(&country);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK || esp_now_register_recv_cb(onReceive) != ESP_OK)
    Serial.println("ERREUR initialisation ESP-NOW");
  Serial.printf("RECEIVER %s Camera=%u\nCommande: cam 2 puis Entree; ou appui BOOT 1s pour camera suivante.\n", WiFi.macAddress().c_str(), camera);
  Serial.setTimeout(20);
}

void loop() {
  const uint32_t now = millis();
  uint32_t lastRx;
  bool updated;
  portENTER_CRITICAL(&mux);
  updated = pending;
  if (pending) { current = inbox; pending = false; }
  lastRx = receivedAt;
  portEXIT_CRITICAL(&mux);
  if (updated) { seen = true; channel = current.channel; }
  const auto state = tally::state(current, camera, seen, millis() - lastRx);
#ifdef TALLY_WS2812
  showRgb(state, now);
#endif
  static auto previousState = tally::State::Lost;
  if (updated && state != previousState)
    Serial.printf("Cam%u %s canal=%u seq=%lu demo=%u\n", camera,
        state == tally::State::Program ? "PROGRAM" : state == tally::State::Preview ? "PREVIEW" : state == tally::State::Idle ? "IDLE" : "ATEM OFFLINE",
        current.channel, static_cast<unsigned long>(current.sequence), current.demo);
  if (state == tally::State::Lost && previousState != state) Serial.println("LIAISON PERDUE; recherche du master");
  previousState = state;
  digitalWrite(kRed, state == tally::State::Program);
  digitalWrite(kGreen, state == tally::State::Preview);
  // Built-in single LED also lets a bare board test every state.
  const bool status = state == tally::State::Program ? true :
      state == tally::State::Preview ? ((now / 250) % 2) :
      state == tally::State::Idle ? (now % 2000 < 80) :
      state == tally::State::Offline ? (now % 1200 < 100 || (now % 1200 >= 200 && now % 1200 < 300)) :
      ((now / 100) % 2);
  digitalWrite(kStatus, status);
  if (state == tally::State::Lost && now - hopAt >= 300) {
    hopAt = now;
    channel = channel % 13 + 1;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  }
  if (digitalRead(0) == LOW) {
    if (!buttonAt) buttonAt = now;
    if (!buttonHandled && now - buttonAt >= 1000) {
      setCamera(camera % tally::kInputs + 1);
      buttonHandled = true;
    }
  } else { buttonAt = 0; buttonHandled = false; }
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.startsWith("cam ")) {
        char* end;
        const long value = strtol(line.c_str() + 4, &end, 10);
        if (*end == '\0' && value >= 1 && value <= tally::kInputs) setCamera(value);
        else Serial.println("Camera invalide; saisir cam 1 a cam 40.");
      }
      line = "";
    } else if (line.length() < 30) line += c;
  }
  delay(1);
}
