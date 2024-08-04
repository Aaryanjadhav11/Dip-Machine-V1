#pragma once

#include "Globals.h"
#include "AccelStepper.h"
#include "OneWire.h"
#include "DallasTemperature.h"

// Rotary axis stepper motor pins
#define ROTARY_AXIS_LIMIT_PIN 26
#define ROTARY_AXIS_STEP_PIN 12
#define ROTARY_AXIS_DIR_PIN 23

// Z-axis stepper motor pins
#define Z_AXIS_LIMIT_PIN 35
#define Z_AXIS_STEP_PIN 33
#define Z_AXIS_DIR_PIN 32

// Steering motor pins
#define STEERING_CHANNEL 4
#define STEERING_MOTOR_PIN 25

// Sensor pin
constexpr int TEMP_SENSOR_PIN = 15;

// Functions
void heatingInit(void * params);
bool checkSensors(uint8_t sensorNumber);

#define RUN (currentState != MachineState::WORKING)

namespace Move{
    void dip(int duration, int rpm);
    void moveToBeaker(uint8_t beakerNum);
    void home();
    void present();
} // namespace Move
