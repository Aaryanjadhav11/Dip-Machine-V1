#include "Globals.h"

Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Variales only for internal
struct Network {
    String ssid;
    int rssi;
    bool isOpen;
};
MachineState currentState = MachineState::IDLE;
MachineInfo machineInfo;
#define BROADCAST (ws.count() > 0 && MACHINE_HEATING || MACHINE_WORKING && millis() - broadcast_counter > 3000)

// ************** Function Prototypes **************
void HandleWiFi();
void scanNetworks();
void updateWiFi(const char* ssid, const char* pass);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void checkPowerLoss();
void printMachineInfo(const MachineInfo& info);
void startMachine(const JsonDocument& doc, MachineInfo& info);

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

    // Recover data stored in memory (if any)
    preferences.begin(machineInfoStore, true);
    size_t len = preferences.getBytesLength("data");
    if (len == sizeof(machineInfo)) {
      preferences.getBytes("data", &machineInfo, sizeof(machineInfo));
      printMachineInfo(machineInfo);
    }
    preferences.end();

    esp_task_wdt_init(50, true);
    esp_task_wdt_add(NULL);

    unsigned long wdt_counter = millis();
    unsigned long broadcast_counter = millis();
    while (true) {
      if (WDT_TRIGGER){
        esp_task_wdt_reset();
        wdt_counter = millis();
      }
      if (BROADCAST){
        printMachineInfo(machineInfo);
        broadcast("WORKING");
        broadcast_counter = millis();
      }
      if (MACHINE_DONE){
        broadcast("DONE");
      }
      
      ArduinoOTA.handle();
      vTaskDelay(pdMS_TO_TICKS(10));
    }
} // appLinkInit

// =======================================| WiFi functions |================================================
void scanNetworks() {
    Serial.println("[scanNetworks] Searching for avaliable networks...");
    int n = WiFi.scanNetworks();
    Network* networks = new Network[n];
    JsonDocument doc;

    for (int i = 0; i < n; i++) {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].isOpen = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);

        // Create a JSON object for each network
        JsonObject network = doc[networks[i].ssid].to<JsonObject>();
        network["rssi"] = networks[i].rssi;
        network["isOpen"] = networks[i].isOpen;
    }
    // Serialize the JSON document to a string
    String jsonString;
    serializeJson(doc, jsonString);
    // Send the JSON string to all WebSocket clients
    ws.textAll(jsonString);
    delete[] networks;
    Serial.println("[scanNetworks] Done");
} // scanNetworks

void updateWiFi(const char* ssid, const char* pass){
    Serial.println("[updateWiFi] Storing WiFi Credentials");
    preferences.begin("wifi");
    bool setSSID = preferences.putString("ssid", ssid);
    bool setPass = preferences.putString("password", pass);
    Serial.printf("[updateWiFi] Stored SSID: %s, Password: %s\n", ssid, pass);
    preferences.end();

    if (setSSID) {
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

    unsigned long counter = millis();
    // Attempt to connect to WiFi
    while (WiFi.status() != WL_CONNECTED && millis() - counter < 5000) {
        Serial.print(".");
        digitalWrite(BUILTIN_LED, HIGH);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(BUILTIN_LED, LOW);
        vTaskDelay(pdMS_TO_TICKS(100));
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
      esp_task_wdt_reset(); // Feed the watchdog timer
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

// =======================| Checking function |=============================
void checkPowerLoss(){
  if (machineInfo.powerLoss){
    Serial.println("[checkPowerLoss] PowerLoss Detected");
    broadcast("POWERLOSS");
  } else broadcast("IDEL");
} // checkPowerLoss

// =========================| State Changing Functions |========================
void startMachine(const JsonDocument& doc, MachineInfo& info) {
  Serial.println("[startMachine] Started Heating");
  info.activeBeakers = doc["activeBeakers"];
  info.setCycles = doc["setCycles"];

  JsonArrayConst setDipTemperature = doc["setDipTemperature"];
  for (int i = 0; i < info.activeBeakers; i++) {
      info.setDipTemperature[i] = setDipTemperature[i];
  }

  JsonArrayConst setDipDuration = doc["setDipDuration"];
  for (int i = 0; i < info.activeBeakers; i++) {
      info.setDipDuration[i] = setDipDuration[i];
  }

  JsonArrayConst setDipRPM = doc["setDipRPM"];
  for (int i = 0; i < info.activeBeakers; i++) {
      info.setDipRPM[i] = setDipRPM[i];
  }
  broadcast("HOMING");
  currentState = MachineState::HOMING;
} // startMachine

// =========================| Websocket Event handling |==============================
void processClientMessage(char* message){
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("[processClientMessage] deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  } else Serial.println(message);

  String status = doc["status"];
  // Handle wifi functions
  if (status == "scanWiFi") { // scan for wifi
      Serial.println("[processClientMessage] scanning wifi");
      scanNetworks();
      return;
  }
  if (status == "setWiFi") { // Set wifi
      Serial.println("[processClientMessage] setting wifi");
      String ssid = doc["ssid"];
      String password = doc["password"];
      updateWiFi(ssid.c_str(), password.c_str());
      return;
  }
  // Handle new start
  if (status == "new"){ // Start new machine
    Serial.println("[processClientMessage] Setting new machine");
    clearAll();
    broadcast("IDEL");
    currentState = MachineState::IDLE;
    return;
  }
  if (status == "start"){ // Start new machine
    Serial.println("[processClientMessage] Starting machine");
    startMachine(doc, machineInfo);
    return;
  }
  // Check again
  if(status == "checkAgain"){
    checkSensors();
    return;
  }
 // handle recovery from power loss
  if (status == "recover"){
    if (!machineInfo.powerLoss){
      ws.textAll("No power loss Detected, Recovery attempt failed!");
      Serial.println("[processClientMessage] Recovery attempt failed!");
      return;
    }
    Serial.println("[processClientMessage] Recovering from powerloss");
    ws.textAll("[processClientMessage] Recovering from powerloss");
    machineInfo.powerLoss = false;
    currentState = MachineState::HOMING;
    return;
  }
// handle abortion
  if (status == "abort"){ // abort current operation
    Serial.println("[processClientMessage] Aborting");
    clearAll();
    currentState = MachineState::ABORT;
    broadcast("ABORTED");
    currentState = MachineState::IDLE;
    broadcast("IDEL");
    return;
  } else ws.textAll("Can't Abort if machine ain't started!");
} // processClientMessage

void clearAll() {
  preferences.begin(machineInfoStore, false);
  preferences.clear(); // Clear sotrage
  preferences.end(); // Close the preferences
  Serial.println("[clearAll] machineInfoStore & machineInfo cleared!");

  memset(&machineInfo, 0, sizeof(MachineInfo)); //
  
} // clearAll

void clientConnected(){
  if (MACHINE_IDEL || MACHINE_HOMING){
    checkPowerLoss(); // sends POWERLOSS if true
    checkSensors(); // sends HALTED if true
  }  
  else if (MACHINE_ABORT){
    broadcast("IDEL");
  }
  else if (MACHINE_HEATING || MACHINE_WORKING){
    broadcast("WORKING");
  }
  else if (MACHINE_DONE){
    broadcast("DONE");
    currentState = MachineState::IDLE;
    vTaskDelay(pdMS_TO_TICKS(5000));
    broadcast("IDEL");
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[onWsEvent][%s] Client connected\n", client->remoteIP().toString().c_str());
    clientConnected();
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("[onWsEvent] Client disconnected");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len) {
      data[len] = '\0';  // Null-terminate the data
      Serial.printf("[onWsEvent][%s] %s\n", client->remoteIP().toString().c_str(), (char*)data);
      processClientMessage((char*)data);
    }
  }
} // onWsEvent

void broadcast(String state, String error){
  JsonDocument doc;
  // Serialize the MachineInfo struct to JSON
  doc["state"] = state;
  doc["timeLeft"] = machineInfo.timeLeft;
  doc["activeBeakers"] = machineInfo.activeBeakers;
  doc["onBeaker"] = machineInfo.onBeaker;
  doc["onCycle"] = machineInfo.onCycle;
  doc["setCycles"] = machineInfo.setCycles;
  if (error.length() > 0) doc["error"] = error;

  JsonArray currentTemp = doc["currentTemp"].to<JsonArray>();
  JsonArray setDipDuration = doc["setDipDuration"].to<JsonArray>();
  JsonArray setDipRPM = doc["setDipRPM"].to<JsonArray>();
  JsonArray setDipTemp = doc["setDipTemperature"].to<JsonArray>();

  for (size_t i = 0; i < machineInfo.activeBeakers; i++){
    currentTemp.add(machineInfo.currentTemps[i]);
    setDipDuration.add(machineInfo.setDipDuration[i]);
    setDipRPM.add(machineInfo.setDipRPM[i]);
    setDipTemp.add(machineInfo.setDipTemperature[i]);
  }
  String output;
  serializeJson(doc, output);
  ws.textAll(output);
} // broadcast

// ==================| Debughing code |================
void printMachineInfo(const MachineInfo& info) {
  Serial.println("----------------|printMachineInfo|---------------------");
  Serial.printf("Machine Information:\n");
  Serial.printf("Power Loss: %s\n", info.powerLoss ? "Yes" : "No");
  Serial.printf("Machine state: %d\n", int(currentState));
  Serial.printf("Time Left: %d\n", info.timeLeft);
  Serial.printf("Active Beakers: %d\n", info.activeBeakers);
  Serial.printf("On Beaker: %d\n", info.onBeaker);
  Serial.printf("On Cycle: %d\n", info.onCycle);
  Serial.printf("Set Cycles: %d\n", info.setCycles);

  Serial.printf("Current Temperatures: ");
  for (int i = 0; i < MAX_BEAKERS; i++) {
      Serial.printf("Beaker %d: %.2f ", i, info.currentTemps[i]);
  }
  Serial.printf("\n");

  Serial.printf("Set Dip Temperatures: ");
  for (int i = 0; i < MAX_BEAKERS; i++) {
      Serial.printf("Beaker %d: %.2f ", i, info.setDipTemperature[i]);
  }
  Serial.printf("\n");

  Serial.printf("Set Dip Durations: ");
  for (int i = 0; i < MAX_BEAKERS; i++) {
      Serial.printf("Beaker %d: %d ", i, info.setDipDuration[i]);
  }
  Serial.printf("\n");

  Serial.printf("Set Dip RPMs: ");
  for (int i = 0; i < MAX_BEAKERS; i++) {
      Serial.printf("Beaker %d: %d ", i, info.setDipRPM[i]);
  }
  Serial.printf("\n");
  Serial.println("-------------------------------------");
} // printMachineInfo
