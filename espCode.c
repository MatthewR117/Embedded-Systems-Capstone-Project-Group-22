// Version with watthours == 0 :)

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char *WIFI_SSID     = "OnePlus 13"; // TTUguest
const char *WIFI_PASSWORD = "12345678";   // maskedraiders

// Base URL (no trailing slash)
const char *FIREBASE_HOST = "https://drjohnson0x01-default-rtdb.firebaseio.com"; //lol
// If you use an auth token, put "?auth=TOKEN" at the end of each path.
// If no auth, leave as empty string.
const char *FIREBASE_AUTH_SUFFIX = "";  // e.g. "?auth=XYZ" or ""

WiFiClientSecure g_client;

const int UART2_RX_PIN = 16;  // ESP32 RX2 (connect to STM32 TX6)
const int UART2_TX_PIN = 17;  // ESP32 TX2 (connect to STM32 RX6)

// Relay polling
unsigned long g_lastRelayPollMs = 0;
const unsigned long g_relayPollIntervalMs = 300;
unsigned char g_relayCmdByte = 0;

// Telemetry receive buffer (16 bytes from STM32)
unsigned char g_telemBuf[16];
int g_telemIndex = 0;

// Forward declarations
void connectWiFi(void);
bool fbGetInt(const char *path, long *valueOut);
bool fbPatchTelemetry(unsigned long vWord,
                      unsigned long cWord,
                      unsigned long wWord,
                      unsigned long whWord);
void handleRelay(void);
void handleTelemetry(void);

void setup() {
  Serial.begin(115200);

  Serial2.begin(115200, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);

  connectWiFi();

  g_client.setInsecure(); // allow HTTPS without cert
}

void loop() {
  handleRelay();
  handleTelemetry();
}

/*************** WiFi ***************/
void connectWiFi(void) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

/*************** Firebase helpers ***************/
bool fbGetInt(const char *path, long *valueOut) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  HTTPClient http;
  String url = String(FIREBASE_HOST) + String(path) + String(FIREBASE_AUTH_SUFFIX);

  http.begin(g_client, url);
  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  body.trim();
  if (body.length() == 0 || body == "null") {
    return false;
  }

  long v = body.toInt();
  *valueOut = v;
  return true;
}

bool fbPatchTelemetry(unsigned long vWord,
                      unsigned long cWord,
                      unsigned long wWord,
                      unsigned long whWord) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  HTTPClient http;
  String path = "/iot_data.json";  // original path
  String url  = String(FIREBASE_HOST) + path + String(FIREBASE_AUTH_SUFFIX);

  http.begin(g_client, url);
  http.addHeader("Content-Type", "application/json");

  // Send raw 32-bit words (Flutter interprets them back to floats)
  String payload = String("{\"voltage\":") + String(vWord) +
                   String(",\"current\":") + String(cWord) +
                   String(",\"watts\":") + String(wWord) +
                   String(",\"watthours\":") + String(0) +
                   String("}");
    // watthours zero, removing it without changing size of payload
  int code = http.sendRequest("PATCH", payload);

  if (code <= 0) {
    http.end();
    return false;
  }

  String resp = http.getString();
  http.end();

  return (code >= 200 && code < 300);
}

/*************** Relay handling (with debug prints) ***************/
void handleRelay(void) {
  unsigned long now = millis();
  if (now - g_lastRelayPollMs < g_relayPollIntervalMs) {
    return;
  }
  g_lastRelayPollMs = now;

  long relaysValue = 0;
  if (!fbGetInt("/iot_data/relays.json", &relaysValue)) {
    return;
  }

  g_relayCmdByte = (unsigned char)(relaysValue & 0xFF);

  Serial.print("[RELAY] Firebase relays int = ");
  Serial.println(relaysValue);
  Serial.print("[RELAY] UART cmd byte = 0x");
  if (g_relayCmdByte < 0x10) Serial.print("0");
  Serial.println(g_relayCmdByte, HEX);

  Serial2.write(g_relayCmdByte);
}

/*************** Telemetry handling (no raw UART dumps) ***************/
void handleTelemetry(void) {
  while (Serial2.available() > 0) {
    int b = Serial2.read();
    if (b < 0) {
      break;
    }

    g_telemBuf[g_telemIndex++] = (unsigned char)b;

    if (g_telemIndex >= 16) {
      unsigned long vWord = ((unsigned long)g_telemBuf[0] << 24) |
                            ((unsigned long)g_telemBuf[1] << 16) |
                            ((unsigned long)g_telemBuf[2] << 8)  |
                            ((unsigned long)g_telemBuf[3]);

      unsigned long cWord = ((unsigned long)g_telemBuf[4] << 24) |
                            ((unsigned long)g_telemBuf[5] << 16) |
                            ((unsigned long)g_telemBuf[6] << 8)  |
                            ((unsigned long)g_telemBuf[7]);

      unsigned long wWord = ((unsigned long)g_telemBuf[8] << 24) |
                            ((unsigned long)g_telemBuf[9] << 16) |
                            ((unsigned long)g_telemBuf[10] << 8) |
                            ((unsigned long)g_telemBuf[11]);

      unsigned long whWord = ((unsigned long)g_telemBuf[12] << 24) |
                             ((unsigned long)g_telemBuf[13] << 16) |
                             ((unsigned long)g_telemBuf[14] << 8)  |
                             ((unsigned long)g_telemBuf[15]);

      fbPatchTelemetry(vWord, cWord, wWord, whWord);

      g_telemIndex = 0;
    }
  }
}