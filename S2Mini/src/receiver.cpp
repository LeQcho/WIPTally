#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "tally.h"
#include "espnow_radio.h"
#include "presence.h"
#ifdef TALLY_WS2812
#include <Adafruit_NeoPixel.h>
#include "rgb_status.h"
Adafruit_NeoPixel pixels(tally::kRgbPixels, tally::kRgbDataPin, NEO_RGB + NEO_KHZ800);
uint32_t displayedRgb = UINT32_MAX;
uint8_t rgbLevel = tally::kDefaultRgbLevel;
void showRgb(tally::State state, uint32_t now) {
  const auto color = tally::rgbStatus(state, now, rgbLevel);
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
uint8_t inboxMaster[6] = {}, currentMaster[6] = {}, reportPeer[6] = {};
tally::PresenceTimer presenceTimer;
bool reportPeerReady = false, reportRadioReady = false;
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
  memcpy(inboxMaster, mac, 6);
  receivedAt = millis();
  pending = true;
  portEXIT_CRITICAL(&mux);
}

void sendPresence(uint32_t now, bool linked) {
  if (!reportRadioReady || !presenceTimer.due(now, linked)) return;
  if (!reportPeerReady || memcmp(reportPeer, currentMaster, 6)) {
    if (reportPeerReady) esp_now_del_peer(reportPeer);
    reportPeerReady = false;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, currentMaster, 6);
    peer.ifidx = WIFI_IF_STA;
    peer.channel = 0; // Same channel as the tally packet just received.
    if (esp_now_add_peer(&peer) != ESP_OK) return;
    memcpy(reportPeer, currentMaster, 6);
    reportPeerReady = true;
  }
  tally::ReceiverReport report;
  report.camera = camera;
  const auto result = esp_now_send(reportPeer, reinterpret_cast<const uint8_t*>(&report), sizeof(report));
  if (result != ESP_OK) Serial.printf("Presence ESP-NOW refusee (%d)\n", result);
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
  digitalWrite(kStatus, LOW); // Internal LED stays off inside the enclosure.
  pinMode(0, INPUT_PULLUP);
  prefs.begin("tally-rx", false);
  camera = prefs.getUChar("camera", 1);
  if (camera < 1 || camera > tally::kInputs) camera = 1;
#ifdef TALLY_WS2812
  // This rig's pixels swap red/green with GRB. Allow standard GRB modules too.
  const bool rgbOrder = prefs.getUChar("rgb_order", 1) != 0;
  rgbLevel = prefs.getUChar("rgb_level", tally::kDefaultRgbLevel);
  if (rgbLevel == 0) rgbLevel = tally::kDefaultRgbLevel;
  pixels.updateType((rgbOrder ? NEO_RGB : NEO_GRB) + NEO_KHZ800);
  pixels.begin();
  showRgb(tally::State::Lost, millis());
  Serial.printf("RGB WS2812: %u pixels, GPIO%u, intensite %u/255\n",
      tally::kRgbPixels, tally::kRgbDataPin, rgbLevel);
  Serial.printf("RGB ordre=%s fw=receiver-rgb-1.7.0\n", rgbOrder ? "RGB" : "GRB");
#endif
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  // Country CH permits channels 1..13; receiver never associates to the router.
  wifi_country_t country = {"CH", 1, 13, 20, WIFI_COUNTRY_POLICY_MANUAL};
  esp_wifi_set_country(&country);
  const auto protocol = tally::enableLongRange();
  if (protocol == ESP_OK) Serial.println("ESP-NOW: reception Long Range et standard configuree");
  else Serial.printf("ESP-NOW: protocole Long Range refuse (%d)\n", protocol);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  reportRadioReady = esp_now_init() == ESP_OK && esp_now_register_recv_cb(onReceive) == ESP_OK;
  if (!reportRadioReady)
    Serial.println("ERREUR initialisation ESP-NOW");
  else if (protocol == ESP_OK) {
    const auto rate = esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_LORA_250K);
    if (rate == ESP_OK) Serial.println("ESP-NOW presence: Long Range 250 kbit/s configure");
    else {
      const auto fallback = esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_1M_L);
      Serial.printf("Presence: debit Long Range refuse (%d), secours standard=%d\n", rate, fallback);
    }
  }
  presenceTimer.begin(esp_random());
  Serial.println("RECEIVER fw=1.7.0; LED interne eteinte; retour de presence actif si radio initialisee");
  Serial.printf("RECEIVER %s Camera=%u\nCommande: cam 2 puis Entree; ou appui BOOT 1s pour camera suivante.\n", WiFi.macAddress().c_str(), camera);
  Serial.setTimeout(20);
}

void loop() {
  const uint32_t now = millis();
  uint32_t lastRx;
  bool updated;
  portENTER_CRITICAL(&mux);
  updated = pending;
  if (pending) { current = inbox; memcpy(currentMaster, inboxMaster, 6); pending = false; }
  lastRx = receivedAt;
  portEXIT_CRITICAL(&mux);
  if (updated) {
    seen = true;
    channel = current.channel;
#ifdef TALLY_WS2812
    if (current.brightness != 0) rgbLevel = current.brightness;
#endif
  }
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
  sendPresence(millis(), state != tally::State::Lost);
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
