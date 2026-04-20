//Jacob Holwill 10859926
//this file create the functions for the LDR reading and the ring light 

#include "RingLight.hpp"

// create counter and averaging for the ldr 
static int LDR_Count = 0;
static int LDR_Ave = 0;

Adafruit_NeoPixel Ring(NUM_LEDS, RINGLIGHT_PIN, NEO_GRB + NEO_KHZ800);

void Light_init() {

    pinMode(LDR_PIN, INPUT); // set the LDR into input mode on pin D0

    Ring.begin(); // initialise the ring light
}

void Light_Check() {
    int LDR = analogRead(LDR_PIN); // read the LDR value

    LDR_Ave += LDR; // add the LDR alue to the average int
    LDR_Count++; // add 1 to the counter

    if (LDR_Count == 10){ // every 10 counts
        LDR_Ave = LDR_Ave/10; // get the average over the 10 readings
        if (LDR_Ave < TARGET_LIGHT){ // if the average is less then the threashold
            Ring.setBrightness(LED_MAX); // set the LED brightness to max

            // set each LED of the ring to pure white
            for (int i = 0; i < NUM_LEDS; i++) { 
                Ring.setPixelColor(i, Ring.Color(0, 0, 0, 255)); 
            }
            Ring.show(); // show the set values
        }
        else{ // if above the threshold
            // turn off the LED ring
            Ring.setBrightness(LED_MIN);
            Ring.clear();
            Ring.show();
        }
        // reset the 2 values
        LDR_Ave = 0;
        LDR_Count = 0;
    }
}
