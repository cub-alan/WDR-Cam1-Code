//Jacob Holwill 10859926
//the point of this file is to be able to use it to call Ring Light functions in main

#ifndef RINGLIGHT_HPP
#define RINGLIGHT_HPP
//include nessesary librarys
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// set the pins and values
#define LDR_PIN D0
#define RINGLIGHT_PIN D2
#define NUM_LEDS  16
#define TARGET_LIGHT 2000
#define LED_MAX 255
#define LED_MIN 10


extern Adafruit_NeoPixel Ring; // set the ring light

//initialise the light functions
void Light_init();
void Light_Check();

#endif