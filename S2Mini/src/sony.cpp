// Experimental, receive-only Sony REC tally. Never writes to FF01 (REC control).
#include <Arduino.h>
#include <Preferences.h>
#include <NimBLEDevice.h>
#include <Adafruit_NeoPixel.h>
#include <atomic>
#include "sony_status.h"

std::atomic<sony::State> recState{sony::State::Unknown};
std::atomic<bool> forgetRequested{false};
String cameraName, configuredMac;
uint8_t ledPin = 4, ledCount = 2, brightness = 20;
bool rgbOrder = true;
Adafruit_NeoPixel pixels;

void consume(const uint8_t* bytes, size_t size) {
  const auto before = recState.load();
  const auto after = sony::decode(bytes, size, before);
  recState.store(after);
  if (after != before)
    Serial.println(after == sony::State::Recording ? "SONY REC confirme" : "SONY STOP confirme");
}

void onNotify(NimBLERemoteCharacteristic*, uint8_t* bytes, size_t size, bool) {
  consume(bytes, size);
}

class ClientCallbacks : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient*) override {
    recState.store(sony::State::Unknown);
    Serial.println("SONY liaison perdue; etat REC inconnu");
  }
  void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
    if (!desc->sec_state.encrypted) {
      recState.store(sony::State::Unknown);
      auto* client = NimBLEDevice::getClientByID(desc->conn_handle);
      if (client) client->disconnect();
    }
  }
} clientCallbacks;

// BLE operations may block. They run separately so lost/unknown always blinks
// and a disconnect immediately clears a previously red REC indication.
void bluetoothTask(void*) {
  Preferences bond;
  bond.begin("sony-bond", false);
  String remembered = bond.getString("address", "");
  auto* client = NimBLEDevice::createClient();
  client->setClientCallbacks(&clientCallbacks, false);
  client->setConnectTimeout(8);
  client->setConnectionParams(24, 40, 0, 100);
  auto* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(80);
  scan->setWindow(60);
  NimBLERemoteCharacteristic* status = nullptr;
  for (;;) {
    if (forgetRequested.exchange(false)) {
      recState.store(sony::State::Unknown);
      if (client->isConnected()) client->disconnect();
      NimBLEDevice::deleteAllBonds();
      bond.clear(); remembered = ""; status = nullptr;
      Serial.println("Appairage efface; activer Appairage sur la camera");
    }
    if (client->isConnected()) {
      // Use live notifications only. FF02 is an event channel: an undocumented
      // cached read must not overwrite a more recent REC notification.
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    recState.store(sony::State::Unknown); status = nullptr;
    auto found = scan->start(3, false);
    const String target = configuredMac.length() ? configuredMac : remembered;
    NimBLEAdvertisedDevice candidate;
    unsigned matches = 0;
    for (int i = 0; i < found.getCount(); ++i) {
      auto device = found.getDevice(i);
      const String address(device.getAddress().toString().c_str());
      const bool match = target.length() ? address.equalsIgnoreCase(target) :
          String(device.getName().c_str()) == cameraName;
      if (match) { candidate = device; ++matches; }
    }
    if (matches != 1) {
      if (matches > 1) Serial.println("Plusieurs Sony detectes: renseigner l'adresse Bluetooth dans Atelier");
      scan->clearResults(); vTaskDelay(pdMS_TO_TICKS(1000)); continue;
    }
    Serial.printf("Connexion Sony %s; confirmer l'appairage sur la camera si demande\n",
                  candidate.getAddress().toString().c_str());
    bool ready = client->connect(&candidate);
    if (ready) ready = client->secureConnection();
    auto* service = ready ? client->getService("8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF") : nullptr;
    status = service ? service->getCharacteristic(NimBLEUUID(uint16_t(0xff02))) : nullptr;
    ready = status && status->canNotify() && status->subscribe(true, onNotify);
    if (ready && client->isConnected()) {
      remembered = client->getPeerAddress().toString().c_str();
      bond.putString("address", remembered);
      Serial.println("Sony connecte; attente d'un etat REC confirme (profil experimental)");
    } else {
      if (client->isConnected()) client->disconnect();
      recState.store(sony::State::Unknown); status = nullptr;
      Serial.println("Sony non pret; verifier Bluetooth et commande a distance Bluetooth");
    }
    scan->clearResults(); vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);
  Preferences config; config.begin("tally-sony", true);
  cameraName = config.getString("name", "ILCE-7M4");
  configuredMac = config.getString("target", "");
  ledPin = config.getUChar("pin", 4);
  if (ledPin != 3 && ledPin != 4 && ledPin != 5 && ledPin != 6 && ledPin != 7 && ledPin != 10) ledPin = 4;
  ledCount = config.getUChar("pixels", 2);
  if (ledCount < 1 || ledCount > 16) ledCount = 2;
  brightness = config.getUChar("level", 20);
  if (brightness < 1 || brightness > 64) brightness = 20;
  rgbOrder = config.getUChar("rgb_order", 1) != 0;
  config.end();
  pixels.updateLength(ledCount); pixels.setPin(ledPin);
  pixels.updateType((rgbOrder ? NEO_RGB : NEO_GRB) + NEO_KHZ800);
  pixels.begin(); pixels.clear(); pixels.show();
  pinMode(9, INPUT_PULLUP); // BOOT on LOLIN C3 Mini.
  NimBLEDevice::init("Tally Sony");
  NimBLEDevice::setSecurityAuth(true, false, true); // Bond, no input/output.
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  Serial.printf("TALLY SONY C3 experimental; cible=%s LED GPIO%u x%u\n", cameraName.c_str(), ledPin, ledCount);
  xTaskCreate(bluetoothTask, "sony-ble", 8192, nullptr, 1, nullptr);
}

void loop() {
  const uint32_t now = millis();
  const auto state = recState.load();
  const uint32_t color = state == sony::State::Recording ? pixels.Color(brightness, 0, 0) :
      state == sony::State::Stopped ? 0 :
      now % 1000 < 300 ? pixels.Color(0, 0, brightness) : 0;
  static uint32_t previous = UINT32_MAX;
  if (color != previous) { pixels.fill(color); pixels.show(); previous = color; }
  static uint32_t heldAt = 0;
  static bool handled = false;
  if (digitalRead(9) == LOW) {
    if (!heldAt) heldAt = now;
    if (!handled && now - heldAt >= 4000) { forgetRequested.store(true); handled = true; }
  } else { heldAt = 0; handled = false; }
  delay(10);
}
