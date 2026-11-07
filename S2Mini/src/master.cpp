// New S2 Mini application; ATEM libraries retain their original GPL notices.
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ATEMmin.h>
#include "tally.h"

Preferences prefs;
WebServer web(80);
ATEMmin atem;
String ssid, password, atemAddress;
bool demo = false, configOpen = false, hadWifi = false;
bool radioReady = false;
uint32_t sentAt = 0, sampledAt = 0, sequence = 0, retryAt = 0, buttonAt = 0;
String formToken;
tally::Packet previous;
const uint8_t broadcastMac[6] = {255,255,255,255,255,255};

bool configClient() {
  if (configOpen && web.client().localIP() == WiFi.softAPIP()) return true;
  web.send(403, "text/plain", "Connecte-toi au Wi-Fi Tally-Setup pour configurer.");
  return false;
}

void openConfig() {
  if (configOpen) return;
  WiFi.mode(WIFI_AP_STA);
  const String apName = "Tally-Setup-" + WiFi.macAddress().substring(12);
  configOpen = WiFi.softAP(apName.c_str(), "tallysetup", 6);
  Serial.printf("CONFIG: %s / mot de passe tallysetup / http://192.168.4.1\n", apName.c_str());
}

void setupWeb() {
  formToken = String(esp_random(), HEX);
  web.on("/", HTTP_GET, [] {
    if (!configClient()) return;
    String html = R"(<meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Tally S2 Mini</title><style>body{font:18px system-ui;max-width:36em;margin:2em auto;padding:1em}input,button{font:inherit;padding:.5em;margin:.3em 0;width:95%}</style><h1>Tally S2 Mini · Master</h1><p>Wi-Fi 2,4 GHz. Mot de passe vide : conserver celui enregistre.</p><form method="post" action="/save"><label>Nom du Wi-Fi<input name="ssid" maxlength="32"></label><label>Mot de passe Wi-Fi<input type="password" name="password" maxlength="63"></label><label>IP de l'ATEM<input name="atem" placeholder="192.168.1.240"></label><label><input style="width:auto" type="checkbox" name="demo" value="1")";
    if (demo) html += " checked";
    html += "> Mode demo : simuler les cameras 1 et 2 sans ATEM</label><input type='hidden' name='token' value='" + formToken + "'><button>Enregistrer et redemarrer</button></form><p>En mode reel, renseigner les trois champs au premier demarrage.</p>";
    web.send(200, "text/html", html);
  });
  web.on("/save", HTTP_POST, [] {
    if (!configClient()) return;
    if (web.arg("token") != formToken) { web.send(403); return; }
    const bool nextDemo = web.arg("demo") == "1";
    String nextSsid = web.arg("ssid");
    String nextPassword = web.arg("password");
    String nextAtem = web.arg("atem");
    if (nextSsid.isEmpty()) nextSsid = ssid;
    if (nextPassword.isEmpty()) nextPassword = password;
    if (nextAtem.isEmpty()) nextAtem = atemAddress;
    IPAddress parsed;
    if (nextSsid.length() > 32 || nextPassword.length() > 63 ||
        (!nextDemo && (nextSsid.isEmpty() || !parsed.fromString(nextAtem) || parsed == IPAddress(0,0,0,0)))) {
      web.send(400, "text/plain", "SSID et IP ATEM valides requis hors mode demo."); return;
    }
    prefs.putString("ssid", nextSsid);
    prefs.putString("password", nextPassword);
    prefs.putString("atem", nextAtem);
    prefs.putBool("demo", nextDemo);
    web.send(200, "text/plain", "Enregistre. Redemarrage du master...");
    delay(500);
    ESP.restart();
  });
  web.begin();
}

void setup() {
  Serial.begin(115200);
  delay(500); // USB CDC; never wait for a computer to boot.
  pinMode(15, OUTPUT);
  pinMode(0, INPUT_PULLUP);
  prefs.begin("tally-master", false);
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("password", "");
  atemAddress = prefs.getString("atem", "");
  demo = prefs.getBool("demo", false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  Serial.printf("MASTER MAC %s\n", WiFi.macAddress().c_str());
  if (ssid.isEmpty() || demo) {
    openConfig();
  } else {
    WiFi.begin(ssid.c_str(), password.c_str());
  }
  // Keep normal b/g/n protocol: LR-only mode would break ordinary router Wi-Fi.
  radioReady = esp_now_init() == ESP_OK;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastMac, 6);
  peer.channel = 0;
  peer.ifidx = WIFI_IF_STA;
  radioReady = radioReady && esp_now_add_peer(&peer) == ESP_OK;
  if (!radioReady) Serial.println("ERREUR initialisation ESP-NOW");
  setupWeb();
}

void loop() {
  const uint32_t now = millis();
  if (digitalRead(0) == LOW) {
    if (!buttonAt) buttonAt = now;
    if (now - buttonAt >= 2000) openConfig();
  } else buttonAt = 0;
  web.handleClient();
  bool wifi = WiFi.status() == WL_CONNECTED;
  if (!demo && wifi) {
    if (!hadWifi) {
      IPAddress ip;
      if (ip.fromString(atemAddress)) {
        atem.begin(ip);
        atem.serialOutput(0x80);
        atem.connect();
      }
      Serial.printf("Wi-Fi OK: %s, canal %d, ATEM %s\n", WiFi.localIP().toString().c_str(), WiFi.channel(), atemAddress.c_str());
    }
    atem.runLoop(); // Library retries ATEM every 5s when no packets arrive.
  }
  if (!demo && !wifi && !ssid.isEmpty() && now - retryAt >= 15000) {
    retryAt = now;
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Nouvelle tentative Wi-Fi; BOOT 2s pour configurer.");
  }
  hadWifi = wifi;
  if (now - sampledAt < 20) { delay(1); return; }
  sampledAt = now;
  tally::Packet packet;
  packet.demo = demo;
  uint8_t channel = 1;
  wifi_second_chan_t secondary;
  esp_wifi_get_channel(&channel, &secondary);
  packet.channel = channel;
  if (demo) {
    packet.ready = 1;
    // Camera 1: 2s program, 2s preview, 2s idle. Camera 2 is the opposite.
    const uint8_t phase = (now / 2000) % 3;
    packet.flags[0] = phase == 0 ? 1 : phase == 1 ? 2 : 0;
    packet.flags[1] = phase == 0 ? 2 : phase == 1 ? 1 : 0;
  } else if (wifi && atem.isConnected() && atem.hasInitialized()) {
    const uint16_t sources = atem.getTallyByIndexSources();
    packet.ready = sources > 0 && sources <= tally::kInputs;
    if (packet.ready) for (uint8_t i = 0; i < sources; ++i)
      packet.flags[i] = atem.getTallyByIndexTallyFlags(i) & 3;
  }
  const bool changed = packet.ready != previous.ready || packet.demo != previous.demo ||
      packet.channel != previous.channel || memcmp(packet.flags, previous.flags, tally::kInputs);
  if (radioReady && (changed || now - sentAt >= tally::kHeartbeatMs)) {
    packet.sequence = ++sequence;
    const auto result = esp_now_send(broadcastMac, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    if (result != ESP_OK) Serial.printf("ESP-NOW erreur: %d\n", result);
    if (changed) Serial.printf("ATEM pret=%u demo=%u canal=%u Cam1=%u Cam2=%u\n", packet.ready, packet.demo, packet.channel, packet.flags[0], packet.flags[1]);
    previous = packet;
    sentAt = now;
  }
  digitalWrite(15, packet.ready ? HIGH : ((now / 500) % 2));
}
