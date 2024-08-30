#include <IndicatorLink/indicatorLink.h>

void indicatorLink(){
    ledcSetup(0, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(1, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(2, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(RED_PIN, 0);
    ledcAttachPin(GREEN_PIN, 1);
    ledcAttachPin(BLUE_PIN, 2);

    while (true){
        switch (currentState)
        {
        case MachineState::IDLE:
            break;
        case MachineState::HOMING:
            break;
        case MachineState::HALTED:
            break;
        case MachineState::WORKING:
            break;
        case MachineState::HEATING:
            break;
        }
    }
}

void fadeTo(uint8_t targetRed, uint8_t targetGreen, uint8_t targetBlue, unsigned long duration) {
    uint8_t startRed = ledcRead(0);
    uint8_t startGreen = ledcRead(1);
    uint8_t startBlue = ledcRead(2);
    
    unsigned long startTime = millis();
    unsigned long elapsedTime;
    
    do {
        elapsedTime = millis() - startTime;
        float progress = (float)elapsedTime / duration;
        
        uint8_t currentRed = startRed + (targetRed - startRed) * progress;
        uint8_t currentGreen = startGreen + (targetGreen - startGreen) * progress;
        uint8_t currentBlue = startBlue + (targetBlue - startBlue) * progress;
        
        ledcWrite(0, currentRed);
        ledcWrite(1, currentGreen);
        ledcWrite(2, currentBlue);
        
        vTaskDelay(pdMS_TO_TICKS(10));  // Small delay to prevent overwhelming the system
    } while (elapsedTime < duration);
}

void pulse(uint8_t red, uint8_t green, uint8_t blue, unsigned long duration) {
    unsigned long halfDuration = duration / 2;
    
    // Fade in
    fadeTo(red, green, blue, halfDuration);
    
    // Fade out
    fadeTo(0, 0, 0, halfDuration);
}

void blink(uint8_t red, uint8_t green, uint8_t blue, unsigned long onDuration, unsigned long offDuration) {
    ledcWrite(0, red);
    ledcWrite(1, green);
    ledcWrite(2, blue);
    delay(onDuration);
    
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcWrite(2, 0);
    delay(offDuration);
}
namespace indicate {


void Wifi_connecting(unsigned long duration) {
    unsigned long endTime = millis() + duration;
    while (millis() < endTime) {
        for (int i = 0; i < 3; i++) {
            blink(0, 0, 255, 200, 200);  // Three fast blue blinks
        }
        vTaskDelay(pdMS_TO_TICKS(1000));  // Pause between sets of blinks
    }
}
void Wifi_connected(){
    
}
void AP_Started(){
    
}

void homing(){

}
void idle(){

}
void heating(){

}
void working(){

}

void aborted(){

}

void done(){

}

}