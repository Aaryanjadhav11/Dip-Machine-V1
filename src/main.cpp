#include "Globals.h"
#include "MachineLink/MachineLink.h"
// Function prototypes
void IRAM_ATTR onPowerLoss();
void printMachineInfo(const MachineInfo& info);

volatile unsigned long lastInterruptTime = 0;
Preferences pref;

void setup() {
  Serial.begin(115200);
  
  pinMode(POWER_LOSS_PIN, INPUT_PULLUP);

  xTaskCreate(appLinkInit, "appLink", 4096, NULL, 0, NULL);
  xTaskCreate(heatingInit, "machineLink", 4096, NULL, 1, NULL);

  attachInterrupt(POWER_LOSS_PIN, onPowerLoss, FALLING);
  Move::home();
}

void loop() {
  if (currentState == MachineState::WORKING){
    
  }
}

// =====================| Power loss interrupt | ===========================
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
