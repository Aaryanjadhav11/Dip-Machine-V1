#include "MachineLink.h"

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

// Class objects
AccelStepper stepper_R(1, ROTARY_AXIS_STEP_PIN, ROTARY_AXIS_DIR_PIN);
AccelStepper stepper_Z(1, Z_AXIS_STEP_PIN, Z_AXIS_DIR_PIN);

// Global variables
const int beakerDistance[6] = {0, -340, -680, -1055, -1390, -1755};

// Function Prototype
void PID();

// =======================| Heating Handling Code |===========================
void heatingInit(void * params){
  ledcSetup(STEERING_CHANNEL, 5000, 10);
  ledcAttachPin(STEERING_MOTOR_PIN, STEERING_CHANNEL);
  ledcWrite(STEERING_CHANNEL, 0);

  pinMode(Z_AXIS_LIMIT_PIN, INPUT_PULLDOWN);
  pinMode(ROTARY_AXIS_LIMIT_PIN, INPUT_PULLDOWN);

    // Basic wdt setup
  esp_task_wdt_init(50, true);
  esp_task_wdt_add(NULL);
  unsigned long wdt_counter = millis();
  Move::home();
  while (true){
    if (WDT_TRIGGER){
      Serial.println("[heatingInit] Watchdog triggered");
      esp_task_wdt_reset();
      wdt_counter = millis();
    }
    if (currentState == MachineState::HEATING){
      currentState = MachineState::WORKING;
    }
  }
} // heatingInit

void PID(){

}


void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
  Serial.println();
}

void displaySensorAddresses() {
  int deviceCount = sensors.getDeviceCount();
  
  Serial.printf("Found %d devices.\n", deviceCount);

  for (int i = 0; i < deviceCount && i < 6; i++) {
    DeviceAddress address;
    if (sensors.getAddress(address, i)) {
      Serial.printf("Sensor %d address: ", i + 1);
      printAddress(address);
    } else {
      Serial.printf("Error: Could not find address for sensor %d!\n", i + 1);
    }
  }
}
// =======================| Motion Handling Code |===========================
namespace Move{

void dip(int duration, int rpm){
// Dips the head in solution and starts sterring
  Serial.println("[dip] Putting in...");

  const int dipDistance = -14500;
  stepper_Z.moveTo(dipDistance);
  Serial.printf("[dip] Duration: %i   RPM: %i\n", duration, rpm);
  while (stepper_Z.currentPosition() != dipDistance) {
    stepper_Z.run();
    if (RUN) {
      stepper_Z.runToNewPosition(0);
      return;
    }
  }
  // calculate rpm to pwm here
  int dutyCycle = map(rpm, 0, 600, 0, 1024);
  digitalWrite(STEERING_MOTOR_PIN, HIGH);
  ledcWrite(STEERING_CHANNEL, dutyCycle);
  unsigned long start = millis();
  while (millis() - start < duration * 1000) {
    if (RUN) {
      stepper_Z.runToNewPosition(0);
      return;
    }
  }
  // stop the steering
  ledcWrite(STEERING_CHANNEL, 0);
  stepper_Z.runToNewPosition(0);
  Serial.println("[dip] Pulling out");
} // dip

void home(){
// Homes all axis
  stepper_Z.setAcceleration(1000);
  stepper_Z.setMaxSpeed(1000);
  stepper_Z.setSpeed(1000);

  Serial.println("[home] Homing Z Axis...");

  while (!digitalRead(Z_AXIS_LIMIT_PIN)) stepper_Z.runSpeed();
  
  stepper_Z.setCurrentPosition(0);
  stepper_Z.runToNewPosition(-400);

  stepper_Z.setAcceleration(200);
  stepper_Z.setMaxSpeed(200);
  stepper_Z.setSpeed(200);

  while (!digitalRead(Z_AXIS_LIMIT_PIN)) stepper_Z.runSpeed();

  stepper_Z.setCurrentPosition(0);
  stepper_Z.runToNewPosition(-500);
  stepper_Z.setCurrentPosition(0);

  stepper_Z.setAcceleration(2000);
  stepper_Z.setMaxSpeed(2000);
  stepper_Z.setSpeed(2000);
  Serial.println("[home] Z Axis Homed.");

  Serial.println("[home] Homing Rotary Axis...");
  stepper_R.setAcceleration(400);
  stepper_R.setMaxSpeed(400);
  stepper_R.setSpeed(400);

  while (!digitalRead(ROTARY_AXIS_LIMIT_PIN)) stepper_R.runSpeed();
  stepper_R.setCurrentPosition(0);
  stepper_R.runToNewPosition(-50);

  stepper_R.setAcceleration(20);
  stepper_R.setMaxSpeed(20);
  stepper_R.setSpeed(20);

  while (!digitalRead(ROTARY_AXIS_LIMIT_PIN)) stepper_R.runSpeed();
  stepper_R.setCurrentPosition(0);
  stepper_R.runToNewPosition(-100);
  stepper_R.setCurrentPosition(0);

  stepper_R.setAcceleration(1000);
  stepper_R.setMaxSpeed(1000);
  stepper_R.setSpeed(1000);
  Serial.println("[home] R Axis Homed.");
} // next

void moveToBeaker(uint8_t beakerNum){
    // Moves the head to the given beaker
    Serial.printf("[moveToBeaker] Moving to %i", beakerNum);
    stepper_R.moveTo(beakerDistance[beakerNum]);
    while (stepper_R.currentPosition() != beakerDistance[beakerNum]) {
      stepper_R.run();
      if (RUN) {
        stepper_Z.runToNewPosition(0);
        return;
      }
    }
} // next

void present(){
// presents the result after completing
    Serial.println("[present] Presenting ..");
    clearAll();
    stepper_Z.runToNewPosition(0);
    stepper_R.runToNewPosition(-882);
    ledcWrite(STEERING_CHANNEL, 0);
    currentState == MachineState::DONE;
    Serial.println("[present] Task Competed!");

} // present

} // namespace Move
