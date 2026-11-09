// New S2 Mini application; ATEM libraries retain their original GPL notices.
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ATEMmin.h>
#include "tally.h"
#include "atem_sync_guard.h"
#include "espnow_radio.h"

Preferences prefs;
WebServer web(80);
ATEMmin atem;
String ssid, password, atemAddress;
uint8_t cameraInputs = tally::kInputs;
bool demo = false, configOpen = false, hadWifi = false;
bool radioReady = false;
bool longRangeEnabled = false;
uint32_t sentAt = 0, sampledAt = 0, sequence = 0, retryAt = 0, buttonAt = 0;
uint32_t statusAt = 0;
AtemSyncGuard syncGuard;
uint32_t syncRetries = 0;
uint32_t radioQueued = 0, radioErrors = 0;
String formToken;
tally::Packet previous;
const uint8_t broadcastMac[6] = {255,255,255,255,255,255};

void configureTallyRate() {
  if (!radioReady || !longRangeEnabled) return;
  const auto result = esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_LORA_250K);
  if (result == ESP_OK) {
    Serial.println("ESP-NOW: Long Range 250 kbit/s configure");
    return;
  }
  Serial.printf("ESP-NOW: debit Long Range refuse (%d)\n", result);
  const auto fallback = esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_1M_L);
  if (fallback == ESP_OK) Serial.println("ESP-NOW: debit standard 1 Mbit/s");
  else Serial.printf("ESP-NOW: debit standard refuse (%d)\n", fallback);
}

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
  cameraInputs = prefs.getUChar("inputs", tally::kInputs);
  if (cameraInputs < 1 || cameraInputs > tally::kInputs) cameraInputs = tally::kInputs;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  const auto protocol = tally::enableLongRange();
  longRangeEnabled = protocol == ESP_OK;
  if (!longRangeEnabled) Serial.printf("ESP-NOW: protocole Long Range refuse (%d)\n", protocol);
  Serial.printf("MASTER MAC %s\n", WiFi.macAddress().c_str());
  if (ssid.isEmpty() || demo) {
    openConfig();
  } else {
    WiFi.begin(ssid.c_str(), password.c_str());
  }
  radioReady = esp_now_init() == ESP_OK;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastMac, 6);
  peer.channel = 0;
  peer.ifidx = WIFI_IF_STA;
  radioReady = radioReady && esp_now_add_peer(&peer) == ESP_OK;
  if (!radioReady) Serial.println("ERREUR initialisation ESP-NOW");
  configureTallyRate();
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
      configureTallyRate();
      IPAddress ip;
      if (ip.fromString(atemAddress)) {
        atem.begin(ip);
        atem.serialOutput(1); // Connection events; avoid one ACK log per packet.
        // runLoop() performs the first handshake. A second immediate connect()
        // would abandon that session and can lose the initial state transfer.
        syncGuard.reset(now);
      }
      Serial.printf("Wi-Fi OK: %s, canal %d, ATEM %s\n", WiFi.localIP().toString().c_str(), WiFi.channel(), atemAddress.c_str());
    }
    atem.runLoop(); // Library retries ATEM every 5s when no packets arrive.
    const uint16_t inputs = atem.getTallyByIndexSources();
    const bool ready = atem.streamHealthy() &&
        atem.hasInitialized() && inputs > 0 && inputs <= tally::kInputs;
    if (syncGuard.reconnectDue(now, atem.isConnected(), ready)) {
      ++syncRetries;
      Serial.println("ATEM synchronisation incomplete depuis 30s : nouvelle session.");
      atem.connect();
    }
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
  } else if (wifi && atem.streamHealthy() && atem.hasInitialized()) {
    const uint16_t sources = atem.getTallyByIndexSources();
    packet.ready = sources > 0 && sources <= tally::kInputs;
    if (packet.ready) for (uint8_t i = 0; i < sources && i < cameraInputs; ++i)
      packet.flags[i] = atem.getTallyByIndexTallyFlags(i) & 3;
  }
  // The configured physical inputs limit the broadcast, never ATEM's topology.
  for (uint8_t i = cameraInputs; i < tally::kInputs; ++i) packet.flags[i] = 0;
  const bool changed = packet.ready != previous.ready || packet.demo != previous.demo ||
      packet.channel != previous.channel || memcmp(packet.flags, previous.flags, tally::kInputs);
  if (radioReady && (changed || now - sentAt >= tally::kHeartbeatMs)) {
    packet.sequence = ++sequence;
    const auto result = esp_now_send(broadcastMac, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    if (result != ESP_OK) {
      ++radioErrors;
      Serial.printf("ESP-NOW erreur: %d\n", result);
    } else ++radioQueued;
    if (changed) Serial.printf("ATEM pret=%u demo=%u canal=%u Cam1=%u Cam2=%u\n", packet.ready, packet.demo, packet.channel, packet.flags[0], packet.flags[1]);
    previous = packet;
    sentAt = now;
  }
  digitalWrite(15, packet.ready ? HIGH : ((now / 500) % 2));
  if (now - statusAt >= 5000) {
    statusAt = now;
    Serial.printf("STATUS fw=local-app-2 wifi=%u ip=%s atem=%s connected=%u rejected=%u initialized=%u tally_inputs=%u ready=%u demo=%u canal=%u sync_retries=%lu uptime_s=%lu\n",
        wifi, WiFi.localIP().toString().c_str(), atemAddress.c_str(),
        atem.isConnected(), atem.isRejected(), atem.hasInitialized(),
        atem.getTallyByIndexSources(), packet.ready, packet.demo, packet.channel,
        static_cast<unsigned long>(syncRetries), static_cast<unsigned long>(now / 1000));
    Serial.printf("CONFIG camera_inputs=%u\n", cameraInputs);
    const auto& d = atem.diagnostics;
    Serial.printf("LINK rssi=%d sessions=%lu rx=%lu payloads=%lu resend=%lu gaps=%lu malformed=%lu cmd_errors=%lu socket_errors=%lu incm=%u last_id=%u age_ms=%lu espnow=%u queued=%lu tx_errors=%lu\n",
        WiFi.RSSI(), (unsigned long)d.sessions, (unsigned long)d.received,
        (unsigned long)d.payloads, (unsigned long)d.retransmits, (unsigned long)d.gaps,
        (unsigned long)d.malformed, (unsigned long)d.commandErrors,
        (unsigned long)d.socketErrors, d.inCm, atem.getATEM_lastRemotePacketId(),
        (unsigned long)atem.lastContactAge(), radioReady,
        (unsigned long)radioQueued, (unsigned long)radioErrors);
  }
}
