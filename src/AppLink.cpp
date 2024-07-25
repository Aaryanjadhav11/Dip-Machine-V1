#include "Globals.h"

/*
Bidirectional communication status codes:
0 : Start new - User
1 : Abort. If new machine - User
2 : Set new WiFi - User
3 : Power loss occurred - Machine
4 : Recover from power loss - User
5 : Error - Machine
6 : General broadcast - Machine
*/

Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Variales only for internal
struct Network {
    String ssid;
    int rssi;
    bool isOpen;
};

// ************** Function Declaration **************
void HandleWiFi();
Network* scanNetworks(int& networkCount);
void updateWiFi(const char* ssid, const char* pass);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

// ========================== Main: appLinkInit =========================
void appLinkInit(void * parameters) {
    // Connect to wifi
    HandleWiFi();

    // Start Websocket Server
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.begin();

    // Start OTA
    ArduinoOTA.begin();
    MDNS.begin("DipMachine");
    
    while (true) {
        vTaskDelay(10);
    }
} // appLinkInit

// =======================================| WiFi functions |================================================
Network* scanNetworks(int& networkCount) {
    Serial.println("[scanNetworks] Searching for avaliable networks...");
    int n = WiFi.scanNetworks();
    Network* networks = new Network[n];
    networkCount = n;

    for (int i = 0; i < n; i++) {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].isOpen = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);

        Serial.printf("%d: %s (%d)%s\n",
            i + 1,
            networks[i].ssid.c_str(),
            networks[i].rssi,
            networks[i].isOpen ? " " : "*");
    }

    Serial.println("[scanNetworks] Done");
    return networks;
} // scanNetworks

void updateWiFi(const char* ssid, const char* pass){
    Serial.println("[updateWiFi] Storing WiFi Credentials");
    preferences.begin("wifi");
    bool setSSID = preferences.putString("ssid", ssid);
    bool setPass = preferences.putString("password", pass);
    Serial.printf("[updateWiFi] Stored SSID: %s, Password: %s\n", ssid, pass);
    preferences.end();

    if (setSSID && setPass) {
        WiFi.disconnect();
        HandleWiFi();
    }
} // updateWiFi

void HandleWiFi() {
    // Initialize preferences and read stored SSID and password
    preferences.begin("wifi");
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();

    Serial.printf("[HandleWiFi] Attempting to connect to SSID: %s...", ssid.c_str());
    WiFi.setHostname("DipMachine");
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long start = millis();
    // Attempt to connect to WiFi
    while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
        Serial.print(".");
        digitalWrite(BUILTIN_LED, HIGH);
        vTaskDelay(pdMS_TO_TICKS(50));
        digitalWrite(BUILTIN_LED, LOW);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[HandleWiFi] WiFi Connected!");
        Serial.printf("\tSignal Strength: %d dBm\n", WiFi.RSSI());
        Serial.printf("\tIP Address: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[HandleWiFi] Failed to connect to WiFi. Check your credentials.");
        // Static IP configuration for the AP
        IPAddress local_ip(192, 168, 1, 5);
        IPAddress gateway(192, 168, 1, 1);
        IPAddress subnet(255, 255, 255, 0);

        // Set up WiFi in AP mode with provided credentials and static IP configuration
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(local_ip, gateway, subnet);
        WiFi.softAP("DipMachine");
        Serial.print("[HandleWiFi] Starting Access Point (AP) with IP: ");
        Serial.println(WiFi.softAPIP());
    }
} // HandleWiFi

// ========================| OTA function |=================================
void initOTA(){
  Serial.println("Initializing OTA");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
} // initOTA

// =========================| Websocket Event handling |==============================
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[onWsEvent] WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    client->text("OK: 200");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[onWsEvent] WebSocket client #%u disconnected\n", client->id());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len) {
      data[len] = '\0';  // Null-terminate the data
      Serial.printf("[onWsEvent][%s] %s\n", client->remoteIP().toString().c_str(), (char*)data);
    }
  }
} // onWsEvent
