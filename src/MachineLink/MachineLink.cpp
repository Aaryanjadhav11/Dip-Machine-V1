#include "MachineLink.h"

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

// Class objects
AccelStepper stepper_R(1, ROTARY_AXIS_STEP_PIN, ROTARY_AXIS_DIR_PIN);
AccelStepper stepper_Z(1, Z_AXIS_STEP_PIN, Z_AXIS_DIR_PIN);

// Global variables
const int beakerDistance[6] = {0, -350, -695, -1055, -1420, -1755};
// Hardcoded sensor addresses
DeviceAddress sensorAddresses[MAX_BEAKERS] = {
  { 0x28, 0x12, 0xCC, 0x16, 0xA8, 0x01, 0x3C, 0x13 }, // 1
  { 0x28, 0xDB, 0x09, 0x16, 0xA8, 0x01, 0x3C, 0xEA }, // 2
  { 0x28, 0xAC, 0x2F, 0x16, 0xA8, 0x01, 0x3C, 0x93 }, // 3
  { 0x28, 0x51, 0x18, 0x16, 0xA8, 0x01, 0x3C, 0x7E }, // 4
  { 0x28, 0x0E, 0x12, 0x16, 0xA8, 0x01, 0x3C, 0xC9 }, // 5
  { 0x28, 0x4B, 0xFF, 0x16, 0xA8, 0x01, 0x3C, 0x61 }  // 6
};

// Function Prototype
void getTemp();
void heatingLoop();
// =======================| Heating Handling Code |===========================
void heatingInit(void * params){
  ledcSetup(STEERING_CHANNEL, 5000, 10);
  ledcAttachPin(STEERING_MOTOR_PIN, STEERING_CHANNEL);
  ledcWrite(STEERING_CHANNEL, 0);

  for (size_t i = 3; i < 9; i++){
    ledcSetup(i, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOSFET_PINS[i-3], i);
    ledcWrite(i, 255);
    vTaskDelay(pdMS_TO_TICKS(100));
  }  
  for (size_t i = 3; i < 9; i++){
    ledcWrite(i, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  
  pinMode(Z_AXIS_LIMIT_PIN, INPUT_PULLDOWN);
  pinMode(ROTARY_AXIS_LIMIT_PIN, INPUT_PULLDOWN);

    // Basic wdt setup
  esp_task_wdt_init(50, true);
  esp_task_wdt_add(NULL);
  unsigned long wdt_counter = millis();
  currentState = MachineState::HOMING;
  Move::home();
  currentState = MachineState::IDLE;

  while (true){
    if (WDT_TRIGGER){
      esp_task_wdt_reset();
      wdt_counter = millis();
    }
    else if (MACHINE_HEATING){
      heatingLoop();
      broadcast("WORKING");
      currentState = MachineState::WORKING;
    }
    else if (MACHINE_ABORT){
      ledcWrite(STEERING_CHANNEL, 0);
      currentState == MachineState::IDLE;
    }
    else if (MACHINE_WORKING){
      checkSensors();
      getTemp();
      heatingLoop();
    }
  }
} // heatingInit

// =======================| Heater Handling Code |===========================
// Handle PID calculation for the provided beaker number
void PID(uint8_t beakerNum){

}

// Runs in loop to maintain temps. Use once to detect heatup.
void heatingLoop(){
}

// =======================| Temperature Sensors Handling Code |===========================
// Function to check if a specific sensor address is connected
bool isSensorConnected(const DeviceAddress& address) {
  oneWire.reset();
  oneWire.select(address);
  oneWire.write(0x44, 1); // Start temperature conversion
  oneWire.reset(); // Reset bus
  oneWire.select(address);
  oneWire.write(0xBE); // Read Scratchpad

  // Read Scratchpad to check if the device responds correctly
  byte scratchpad[9];
  oneWire.read_bytes(scratchpad, sizeof(scratchpad));
  
  // Check if the first byte is valid
  return (scratchpad[0] != 0xFF); // If the first byte is 0xFF, it's likely a disconnected device
} // isSensorConnected

void checkSensors() {
  bool allConnected = true;
  String message = "Disconnected sensors: ";
  for (size_t i = 0; i < MAX_BEAKERS; ++i) {
    if (isSensorConnected(sensorAddresses[i])) {
      Serial.printf("Sensor %d Connected\n", i + 1);
    } else {
      Serial.printf("Sensor %d NOT CONNECTED!\n", i + 1);
      message += String(i + 1) + " ";
      allConnected = false;
    }
  }
  if (!allConnected) {
    broadcast("HALTED", message);
    currentState = MachineState::HALTED;
  }
}

void getTemp(){
  for (size_t i = 0; i < machineInfo.activeBeakers; i++){
    machineInfo.currentTemps[i] = sensors.getTempCByIndex(i);
  }
}
// =======================| Motion Handling Code |===========================
namespace Move{
const int dipDistance = -13500;

void dip(int duration, int rpm){
// Dips the head in solution and starts sterring
  Serial.println("[dip] Putting in...");

  stepper_Z.moveTo(dipDistance);
  Serial.printf("[dip] Duration: %i   RPM: %i\n", duration, rpm);
  while (stepper_Z.currentPosition() != dipDistance) {
    stepper_Z.run();
    if (RUN) {
      abort();
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
      abort();
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

  stepper_Z.setAcceleration(3000);
  stepper_Z.setMaxSpeed(3000);
  stepper_Z.setSpeed(3000);
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
  stepper_R.runToNewPosition(-80);
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
        abort();
        return;
      }
    }
} // next

void done(){
// presents the result after completing
    Serial.println("[Done] Presenting ..");
    stepper_Z.runToNewPosition(0);
    if (machineInfo.storeIn == 0){ // In air selected
      stepper_R.runToNewPosition(-877);
      Serial.printf("[Done] Storing the strip In Air;");
    } else {
      Serial.printf("Storing the strip in beaker: %d", machineInfo.storeIn - 1);
      stepper_R.runToNewPosition(beakerDistance[machineInfo.storeIn - 1]);
      stepper_Z.runToNewPosition(dipDistance);
    }
    ledcWrite(STEERING_CHANNEL, 0);
    currentState = MachineState::DONE;
    broadcast("DONE");
    vTaskDelay(pdMS_TO_TICKS(5000));
    currentState = MachineState::IDLE;
    broadcast("IDLE");
} // Done

void abort(){
  stepper_Z.runToNewPosition(0);
}
} // namespace Move
