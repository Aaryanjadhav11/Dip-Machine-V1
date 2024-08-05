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
constexpr int RED_PIN = 2;
constexpr int GREEN_PIN = 14;
constexpr int BLUE_PIN = 27;

// TODO: Shitft this values in indicator file
constexpr int RED_CHANNEL = 0;
constexpr int GREEN_CHANNEL = 1;
constexpr int BLUE_CHANNEL = 2;
constexpr int PWM_FREQ = 5000;
constexpr int PWM_RESOLUTION = 8;

// Struct to hold machine information
struct MachineInfo {
    volatile bool sensorError;
    volatile bool powerLoss;
    int timeLeft;
    uint8_t activeBeakers;
    uint8_t onBeaker;
    int onCycle;
    int setCycles;
    float currentTemps[MAX_BEAKERS];
    float setDipTemperature[MAX_BEAKERS];
    int setDipDuration[MAX_BEAKERS];
    int setDipRPM[MAX_BEAKERS];
};
extern MachineInfo machineInfo;
#define MACHINE_HEATING (currentState == MachineState::HEATING)
#define MACHINE_WORKING (currentState == MachineState::WORKING)
#define MACHINE_DONE (currentState == MachineState::DONE)
#define MACHINE_IDEL (currentState == MachineState::IDLE)
#define MACHINE_ABORT (currentState == MachineState::ABORT)
#define MACHINE_HOMING (currentState == MachineState::HOMING)

#define machineInfoStore "mahcineInfo"
#define WDT_TRIGGER (millis() - wdt_counter > 2000)
// Mosfet pins array
constexpr uint8_t MOSFET_PINS[MAX_BEAKERS] = {4, 5, 18, 22, 19, 21};

void appLinkInit(void * parameters);
void clearAll();