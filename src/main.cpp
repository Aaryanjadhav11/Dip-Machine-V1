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

  xTaskCreate(appLinkInit, "appLink", 4096, NULL, 1, NULL);
  xTaskCreate(heatingInit, "machineLink", 4096, NULL, 0, NULL);

  attachInterrupt(POWER_LOSS_PIN, onPowerLoss, FALLING);
}

void loop() {
  if (MACHINE_WORKING) {
    for (size_t i = machineInfo.onCycle; i < machineInfo.setCycles; i++) {
      if (RUN){
        Move::abort();
        break;
      }
      for (size_t j = machineInfo.onBeaker; j < machineInfo.activeBeakers; j++) {
        if (RUN) {
          Move::abort();
          break;
        }
        Move::moveToBeaker(j);
        Move::dip(machineInfo.setDipDuration[j], machineInfo.setDipRPM[j]);
        machineInfo.onBeaker++;
      }
      machineInfo.onBeaker = 0;
      machineInfo.onCycle++;
      if (machineInfo.onCycle == machineInfo.setCycles) {
        Move::done();
        break;
      }
    }
  }
}

// =====================| Power loss interrupt | ===========================
void IRAM_ATTR onPowerLoss() {
  unsigned long interruptTime = millis();
  // If interrupts come faster than debounceDelay, assume it's a false trigger
  if (interruptTime - lastInterruptTime > 50 && MACHINE_HEATING || MACHINE_WORKING) {
    pref.begin(machineInfoStore); // Open pref in read-write mode
    machineInfo.powerLoss = true;
    pref.putBytes("data", &machineInfo, sizeof(machineInfo));
    pref.end();

    digitalWrite(BUILTIN_LED, HIGH);
    currentState = MachineState::HALTED;
  }
  lastInterruptTime = interruptTime;
}