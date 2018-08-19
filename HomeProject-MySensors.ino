// START MYSENSORS SETTINGS

#define MY_DEBUG // Enable debug prints to serial monitor
#define MY_GATEWAY_SERIAL // Enable serial gateway
#define MY_INCLUSION_MODE_FEATURE // Enable inclusion mode
#define MY_INCLUSION_BUTTON_FEATURE // Enable Inclusion mode button on gateway
#define MY_INCLUSION_MODE_DURATION 60 // Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_BUTTON_PIN  3 // Digital pin used for inclusion mode button
#define MY_REPEATER_FEATURE // Enable repeater functionality for this node

// END MYSENSOR SETTINGS

#include <PCF8574.h> //from https://github.com/skywodd/pcf8574_arduino_library
#include <Wire.h>
#include <SPI.h>
#include <MySensors.h>  
#include <Bounce2.h>

template <typename T, size_t N> constexpr size_t countof(T(&)[N]) { return N; }

constexpr int PCF_ADDRESSES[] = {0x38}; //TODO make sure those are real values
constexpr int PCF_COUNT = countof(PCF_ADDRESSES); //6;
constexpr int PCF_PINS = 8;
constexpr int LIGHT_COUNT = PCF_COUNT * PCF_PINS; 
constexpr int BUTTON_START_PIN = 22;

constexpr int RELAY_ON = 0;
constexpr int RELAY_OFF = 1;

PCF8574 expanders[PCF_COUNT]; 
Bounce debouncers[LIGHT_COUNT];

void setUpOutputs() {
  for(int i = 0; i < PCF_COUNT; ++i) {
    expanders[i].begin(PCF_ADDRESSES[i]);
    for(int j = 0; j < PCF_PINS; ++j) {
      expanders[i].pinMode(j, OUTPUT);
      expanders[i].digitalWrite(j, RELAY_OFF); //TODO REMOVE THIS LINE, RESTORE PREVIOUS STATE
    }
  }
}

void setUpInputs() {
  for(int i = 0; i < LIGHT_COUNT; ++i) {
    int pin = BUTTON_START_PIN + i;
    pinMode(pin, INPUT_PULLUP);
    debouncers[i].attach(pin);
    debouncers[i].interval(5);
  }
}

void before() {
  setUpOutputs();
  setUpInputs();
}

void setup() {
  

}

void relayWrite(int i, int newState) {
  int expander = i / PCF_PINS; //TODO use bitshifts (since it's 8)
  int pin = i % PCF_PINS;
  expanders[expander].digitalWrite(pin, newState ? RELAY_ON : RELAY_OFF);
  Serial.print("changing");
  Serial.print(expander);
  Serial.print(" on ");
  Serial.print(pin);
  Serial.print(" to ");
  Serial.println(newState);
}

void saveAndSet(int i, int newState) {
  saveState(i, newState);
  relayWrite(i, newState);
}

void loop() {
  for(int i = 0; i < LIGHT_COUNT; ++i) {
    if(debouncers[i].update()) {
      int value = debouncers[i].read();
      if(value == LOW) {
        boolean newState = !loadState(i);
        saveState(i, newState);
        relayWrite(i, newState);
        MyMessage msg(i, V_LIGHT);
        send(msg.set(newState));
      }
    }
  }

}

void presentation() {
  sendSketchInfo("Relay", "1.0");
  for (int i = 0; i < LIGHT_COUNT; ++i) {
    present(i, S_LIGHT);
  }
}

void receive(const MyMessage &message) {
  if (message.type == V_LIGHT) {
    saveAndSet(message.sensor, message.getBool());
  }
}


