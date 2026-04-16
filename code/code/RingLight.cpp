#include "Lighting.hpp"

#define LDR_PIN 1     // ADC pin
#define RINGLIGHT_PIN 2     // D2

#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RES 8     // 0–255

// tuning values
#define TARGET_LIGHT 2000   // desired ADC value (tune this!)
#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 10

static uint8_t brightness = 100;

void Lighting_Init() {
    pinMode(LDR_PIN, INPUT);

    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
    ledcAttachPin(LED_PIN, PWM_CHANNEL);

    ledcWrite(PWM_CHANNEL, brightness);
}

void Lighting_Update() {
    // Read LDR
    int raw = analogRead(LDR_PIN);

    // Simple smoothing
    static int avg = 0;
    avg = (avg * 4 + raw) / 5;

    // Control error
    int error = TARGET_LIGHT - avg;

    // Proportional control (simple)
    brightness += error * 0.02;

    // Clamp
    if (brightness > MAX_BRIGHTNESS) brightness = MAX_BRIGHTNESS;
    if (brightness < MIN_BRIGHTNESS) brightness = MIN_BRIGHTNESS;

    ledcWrite(PWM_CHANNEL, brightness);
}
