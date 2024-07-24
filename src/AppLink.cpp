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
AsyncWebServer server(443);
AsyncWebSocket ws("/wss");

// Variales only for internal
struct Network {
    String ssid;
    int rssi;
    bool isOpen;
};


// ************** Function Declaration **************
void HandleWiFi();
void WiFiEvent(WiFiEvent_t event);
Network* scanNetworks(int& networkCount);
void updateWiFi(const char* ssid, const char* pass);

// ========================== Main: appLinkInit =========================
void appLinkInit(void * parameters) {
    HandleWiFi();
    while (true) {
        vTaskDelay(10);
        int count;
        Network* network = scanNetworks(count);
        delete[] network;
    }
} // appLinkInit

// =======================================| WiFi functions |================================================
void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("[WiFiEvent] Client Connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("[WiFiEvent] Client Disconnected");
            break;
        default:
            break;
    }
} // WiFiEvent

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
    HandleWiFi();
} // updateWiFi

void HandleWiFi() {
    // Initialize preferences and read stored SSID and password
    preferences.begin("wifi");
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();

    Serial.printf("[HandleWiFi] Attempting to connect to SSID: %s\n", ssid.c_str());
    WiFi.setHostname("DipMachine");
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long start = millis();
    // Attempt to connect to WiFi
    Serial.print("[HandleWiFi] Connecting");
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
        WiFi.onEvent(WiFiEvent);
        Serial.print("[HandleWiFi] Starting Access Point (AP) with IP: ");
        Serial.println(WiFi.softAPIP());
    }
} // HandleWiFi
