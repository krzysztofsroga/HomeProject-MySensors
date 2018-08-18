#include <PCF8574.h> //from https://github.com/skywodd/pcf8574_arduino_library
#include <Wire.h>
#include <SPI.h>
#include <MySensors.h>  
#include <Bounce2.h>

// START MYSENSORS SETTINGS

#define MY_DEBUG // Enable debug prints to serial monitor
#define MY_GATEWAY_SERIAL // Enable serial gateway
#define MY_INCLUSION_MODE_FEATURE // Enable inclusion mode
#define MY_INCLUSION_BUTTON_FEATURE // Enable Inclusion mode button on gateway
#define MY_INCLUSION_MODE_DURATION 60 // Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_BUTTON_PIN  3 // Digital pin used for inclusion mode button
#define MY_REPEATER_FEATURE // Enable repeater functionality for this node

// END MYSENSOR SETTINGS

constexpr int PCF_NUMBER = 6;
constexpr int PCF_ADDRESSES[] = {0x20, 0x21, 0x22, 0x38, 0x39, 0x3A}; //TODO make sure those are real values
constexpr int PCF_PINS = 8;

constexpr int BUTTON_START_PIN = 10;
constexpr int BUTTON_NUMBER = 10; //TODO change it to real value

constexpr int SAVE_START = 0; // TODO maybe this doesn't need to be defined

PCF8574 expanders[PCF_NUMBER]; 

void setup() {
  for(int i = 0; i < PCF_NUMBER; ++i) {
    expanders[i].begin(PCF_ADDRESSES[i]);
    for(int j = 0; j < PCF_PINS; ++j) {
      expanders[i].pinMode(j, OUTPUT);    
    }

  }

}

void loop() {
  // put your main code here, to run repeatedly:

}



