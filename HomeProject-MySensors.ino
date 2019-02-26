#define DONT_SAVE

// START MYSENSORS SETTINGS

#if !defined(__AVR_ATmega2560__)
  #error "Arduino MEGA2560 should be used"
#endif

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
#include <Adafruit_PWMServoDriver.h> // 1.0.2 from arduino repo

template <typename T, size_t N> constexpr size_t countof(T(&)[N]) { return N; }

//constexpr int PCF_ADDRESSES[] = {0x38}; //TODO make sure those are real values
constexpr int PCF_ADDRESSES[] = {0x20, 0x21, 0x22, 0x38, 0x39, 0x3A}; //TODO make sure those are real values
constexpr int PCF_RELAY_OFF[]  = {1,    1,    1,    0,    0,    0}; //defines which relays are Low and which High-Level-Trigger
constexpr int PCF_RELAY_ON[] = {0,    0,    0,    1,    1,    1};

constexpr int PWM_FREQUENCY = 60; //Hz

constexpr int PCF_COUNT = countof(PCF_ADDRESSES); //6;
constexpr int PCF_PINS = 8;
constexpr int LIGHT_COUNT = PCF_COUNT * PCF_PINS; 
constexpr int BUTTON_PINS[LIGHT_COUNT] = {  
  2,  3,  4,  5,  6,  7,  8,  9,   // for 0x20
  10, 11, 14, 15, 16, 17, 18, 19,  // for 0x21
  22, 23, 24, 25, 26, 27, 28, 29,   // for 0x22
  30, 31, 32, 33, 34, 35, 36, 37,   // for 0x38
  38, 39, 40, 41, 42, 43, 44, 45,   // for 0x39
  46, 47, 48, 49, 50, 51, 52, 53    // for 0x3A
};

Adafruit_PWMServoDriver ledDriver = Adafruit_PWMServoDriver(0x40); //run PCA9685PW on 0x40 (default)

int working = 0;

//constexpr int RELAY_ON = 1;
//constexpr int RELAY_OFF = 0;

#ifdef DONT_SAVE
volatile int states[LIGHT_COUNT] = {0};
#endif

PCF8574 expanders[PCF_COUNT]; 
Bounce debouncers[LIGHT_COUNT];

void setUpOutputs() {
  for(int i = 0; i < PCF_COUNT; ++i) {
    expanders[i].begin(PCF_ADDRESSES[i]);
    for(int j = 0; j < PCF_PINS; ++j) {
      expanders[i].pinMode(j, OUTPUT);
      expanders[i].digitalWrite(j, PCF_RELAY_OFF[i]); //TODO REMOVE THIS LINE, RESTORE PREVIOUS STATE
    }
  }
  ledDriver.begin();
  ledDriver.setPWMFreq(PWM_FREQUENCY);
}

void setUpInputs() {
  for(int i = 0; i < LIGHT_COUNT; ++i) {
    int pin = BUTTON_PINS[i];
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
  expanders[expander].digitalWrite(pin, newState ? PCF_RELAY_ON[expander] : PCF_RELAY_OFF[expander]);
  Serial.print("changing");
  Serial.print(expander);
  Serial.print(" on ");
  Serial.print(pin);
  Serial.print(" to ");
  Serial.println(newState);
}

void saveAndSet(int i, int newState) {
#ifdef DONT_SAVE
  states[i] = newState;
#else
  saveState(i, newState);
#endif
  relayWrite(i, newState);
}

boolean load(int i) {
#ifdef DONT_SAVE
  return states[i];
#else
  return loadState(i);
#endif
}

void loop() {
  if((working%1000) == 0) {
    Serial.print("working");
  Serial.println(working);
  }
  working++;
  for(int i = 0; i < LIGHT_COUNT; ++i) {
    //states[i] = i % 2;
    if(debouncers[i].update()) {
      int value = debouncers[i].read();
      if(value == LOW) {
        Serial.print("old state");
        Serial.println(load(i));
        boolean newState = !load(i);
        Serial.print("new state");
        Serial.println(newState);
        saveAndSet(i, newState);
        //states[i] = newState;
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
