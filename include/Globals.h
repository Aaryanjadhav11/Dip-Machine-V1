#pragma once

#include "Arduino.h"
#include "ArduinoJson.h"
#include "ArduinoOTA.h"
#include "ESPAsyncWebServer.h"
#include "ESPmDNS.h"
#include "Preferences.h"
#include "WiFi.h"
#include "esp_task_wdt.h"

// Enum for machine states
enum class MachineState : uint8_t {
    IDLE,
    HOMING,
    WORKING,
    HALTED,
    DONE,
    HEATING,
    ABORT
};
// Extern declaration of the current machine state
extern MachineState currentState;

// Constants
constexpr int MAX_BEAKERS = 6;
constexpr int POWER_LOSS_PIN = 34;
constexpr int PWM_FREQ = 5000;
constexpr int PWM_RESOLUTION = 8;

// Struct to hold machine information
struct MachineInfo {
    volatile bool powerLoss;
    uint8_t activeBeakers = 1;
    int setCycles;
    uint8_t storeIn;
    float setDipTemperature[MAX_BEAKERS];
    int setDipDuration[MAX_BEAKERS];
    int setDipRPM[MAX_BEAKERS];

    int timeLeft;
    uint8_t onBeaker;
    int onCycle;
    float currentTemps[MAX_BEAKERS];
};
extern MachineInfo machineInfo;
#define MACHINE_HEATING (currentState == MachineState::HEATING)
#define MACHINE_WORKING (currentState == MachineState::WORKING)
#define MACHINE_DONE (currentState == MachineState::DONE)
#define MACHINE_IDLE (currentState == MachineState::IDLE)
#define MACHINE_ABORT (currentState == MachineState::ABORT)
#define MACHINE_HOMING (currentState == MachineState::HOMING)

#define machineInfoStore "mahcineInfo"
#define WDT_TRIGGER (millis() - wdt_counter > 2000)

void appLinkInit(void * parameters);
void broadcast(String state, String error  = "");
void checkSensors();
void clearAll();