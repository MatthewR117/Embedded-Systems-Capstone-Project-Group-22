//Group-22
//12-09-25
//ESP32 Firebase/STM32 Interface
//Objective: Read encoded voltage, current, power, and energy from STM32 and send it to Firebase,
//           and read relay commands from Firebase and send them back to the STM32.

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char *WIFI_SSID     = "OnePlus 13";  //define wifi ssid
const char *WIFI_PASSWORD = "12345678";    //define wifi password

const char *FIREBASE_HOST = "https://drjohnson0x01-default-rtdb.firebaseio.com"; //define firebase base url
const char *FIREBASE_AUTH_SUFFIX = "";                                           //define firebase auth suffix if needed

WiFiClientSecure g_client;                 //define secure wifi client used by http

const int UART2_RX_PIN = 16;               //define pin used as esp32 rx2 (connect to stm32 tx6)
const int UART2_TX_PIN = 17;               //define pin used as esp32 tx2 (connect to stm32 rx6)

//define variables that control how often relay commands are read from firebase
unsigned long g_lastRelayPollMs = 0;       //stores last time relay command was polled
const unsigned long g_relayPollIntervalMs = 300; //relay polling interval in ms
unsigned char g_relayCmdByte = 0;          //stores last relay command byte sent to stm32

//define buffer that stores 16 telemetry bytes from stm32 (header excluded)
unsigned char g_telemBuf[16];              //4 x 32 bit values (voltage, current, watts, watthours)

//define framing variables that track header and data bytes from stm32
static bool   g_inFrame    = false;        //flag that says we are currently inside a stm32 frame
static int    g_frameIndex = 0;            //index into g_telemBuf during frame reception

//define timeout variables for dropping partial frames
static unsigned long g_lastTelemByteMs = 0; //stores time when last telemetry byte arrived
const unsigned long  g_telemTimeoutMs  = 200; //timeout in ms before dropping partial frame

//function prototypes for wifi, firebase, relay handling, and telemetry handling
void connectWiFi(void);                    //function that connects to wifi
bool fbGetInt(const char *path, long *valueOut); //function that reads integer value from firebase
bool fbPatchTelemetry(unsigned long vWord,
                      unsigned long cWord,
                      unsigned long wWord,
                      unsigned long whWord);     //function that uploads telemetry words to firebase
void handleRelay(void);                   //function that reads relay command from firebase and sends to stm32
void handleTelemetry(void);               //function that receives telemetry from stm32 and uploads to firebase

void setup() {
  Serial.begin(115200);                   //start serial monitor

  Serial2.begin(115200, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN); //start uart2 link to stm32

  connectWiFi();                          //connect esp32 to wifi network
  g_client.setInsecure();                 //allow https without certificate
}

void loop() {
  handleRelay();                          //periodically check relay command in firebase
  handleTelemetry();                      //read telemetry frames from stm32 and send to firebase
}

/*************** WiFi ***************/
void connectWiFi(void) {
  WiFi.mode(WIFI_STA);                    //set wifi to station mode
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);   //start wifi connection

  while (WiFi.status() != WL_CONNECTED) { //wait until wifi connects
    delay(500);
  }
}

/*************** Firebase helpers ***************/
bool fbGetInt(const char *path, long *valueOut) {
  if (WiFi.status() != WL_CONNECTED) {    //if wifi is disconnected reconnect
    connectWiFi();
  }

  HTTPClient http;                        //create http client
  String url = String(FIREBASE_HOST) + String(path) + String(FIREBASE_AUTH_SUFFIX); //build full url

  http.begin(g_client, url);              //start https request
  int code = http.GET();                  //send get request

  if (code != HTTP_CODE_OK) {             //if request fails or not 200
    http.end();
    return false;
  }

  String body = http.getString();         //read reply body
  http.end();

  body.trim();                            //remove whitespace
  if (body.length() == 0 || body == "null") { //if no value stored yet
    return false;
  }

  long v = body.toInt();                  //convert string to integer
  *valueOut = v;                          //store value in output pointer
  return true;
}

bool fbPatchTelemetry(unsigned long vWord,
                      unsigned long cWord,
                      unsigned long wWord,
                      unsigned long whWord) {
  if (WiFi.status() != WL_CONNECTED) {    //if wifi dropped reconnect
    connectWiFi();
  }

  HTTPClient http;                        //create http client
  String path = "/iot_data.json";         //path that stores telemetry in firebase
  String url  = String(FIREBASE_HOST) + path + String(FIREBASE_AUTH_SUFFIX); //build full url

  http.begin(g_client, url);              //start https request
  http.addHeader("Content-Type", "application/json"); //set content type for json

  //build json payload using 32 bit words from stm32
  String payload = String("{\"voltage\":")   + String(vWord)  +
                   String(",\"current\":")   + String(cWord)  +
                   String(",\"watts\":")     + String(wWord)  +
                   String(",\"watthours\":") + String(whWord) +
                   String("}");

  int code = http.sendRequest("PATCH", payload); //send patch request to update telemetry fields

  if (code <= 0) {                         //if request failed
    http.end();
    return false;
  }

  String resp = http.getString();          //optional: read response (not used)
  http.end();

  return (code >= 200 && code < 300);      //return true on http 2xx
}

/*************** Relay handling ***************/
void handleRelay(void) {
  unsigned long now = millis();           //get current time in ms
  if (now - g_lastRelayPollMs < g_relayPollIntervalMs) { //if not yet time to poll relay value
    return;
  }
  g_lastRelayPollMs = now;                //update last poll time

  long relaysValue = 0;                   //stores integer read from firebase
  if (!fbGetInt("/iot_data/relays.json", &relaysValue)) { //read /iot_data/relays from firebase
    return;                               //if read fails skip this cycle
  }

  g_relayCmdByte = (unsigned char)(relaysValue & 0xFF); //use lower 8 bits as command byte to stm32

  Serial.print("[RELAY] Firebase relays int = ");
  Serial.println(relaysValue);
  Serial.print("[RELAY] UART cmd byte = 0x");
  if (g_relayCmdByte < 0x10) Serial.print("0");
  Serial.println(g_relayCmdByte, HEX);

  Serial2.write(g_relayCmdByte);          //send relay command byte to stm32 over uart2
}

/*************** Telemetry handling with header strip ***************/
//stm32 sends: 0xAA header + 16 data bytes (4 x 32 bit words)
//esp32 waits for 0xAA, buffers 16 bytes, converts to 4 unsigned long values, then uploads to firebase
void handleTelemetry(void) {
  unsigned long now = millis();           //get current time in ms

  //if inside a frame and no bytes have arrived within timeout drop partial frame
  if (g_inFrame && (now - g_lastTelemByteMs > g_telemTimeoutMs)) {
    g_inFrame    = false;                 //reset frame flag
    g_frameIndex = 0;                     //reset index
  }

  while (Serial2.available() > 0) {       //while there is data from stm32
    int b = Serial2.read();               //read next byte
    if (b < 0) {
      break;
    }
    uint8_t ub = (uint8_t)b;              //cast to unsigned byte
    g_lastTelemByteMs = now;              //update time of last telemetry byte

    if (!g_inFrame) {                     //if not currently in a frame
      if (ub == 0xAA) {                   //wait for header 0xAA
        g_inFrame    = true;              //start new frame
        g_frameIndex = 0;                 //reset index for data bytes
      }
      //ignore any bytes that are not header
    } else {
      //inside frame: store up to 16 data bytes
      g_telemBuf[g_frameIndex++] = ub;

      if (g_frameIndex >= 16) {           //once 16 bytes received we have full payload
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

        fbPatchTelemetry(vWord, cWord, wWord, whWord); //upload 4 words to firebase

        g_inFrame    = false;             //reset frame state for next packet
        g_frameIndex = 0;
      }
    }
  }
}
