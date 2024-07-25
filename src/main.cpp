#include "Globals.h"

// Function prototypes
void IRAM_ATTR onPowerLoss();
void printMachineInfo(const MachineInfo& info);

// Global variables
volatile unsigned long lastInterruptTime = 0;
Preferences pref;

void setup() {
  Serial.begin(115200);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);

  xTaskCreate(appLinkInit, "appLink", 4096, NULL, 0, NULL);

  pinMode(POWER_LOSS_PIN, INPUT_PULLUP);
  attachInterrupt(POWER_LOSS_PIN, onPowerLoss, FALLING);
}

void loop() {
  if (currentState == MachineState::HEATING){
    Serial.println("Shit is heating UP!");
    delay(5000);
  }
}

void IRAM_ATTR onPowerLoss() {
  unsigned long interruptTime = millis();
  // If interrupts come faster than debounceDelay, assume it's a false trigger
  if (interruptTime - lastInterruptTime > 50 && HEATING_WORKING) {
    pref.begin(machineInfoStore); // Open pref in read-write mode
    machineInfo.powerLoss = true;
    pref.putBytes("data", &machineInfo, sizeof(machineInfo));
    pref.end();

    digitalWrite(BUILTIN_LED, HIGH);
    currentState = MachineState::HALTED;
  }
  lastInterruptTime = interruptTime;
}
