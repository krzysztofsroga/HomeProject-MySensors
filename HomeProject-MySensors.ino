#define DONT_SAVE

#if !defined(__AVR_ATmega2560__)
  #error "Arduino MEGA2560 should be used"
#endif

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
#include <Bounce2.h> // docummentation: https://github.com/thomasfredericks/Bounce2/wiki
#include <Adafruit_PWMServoDriver.h> // 1.0.2 from arduino repo

template <typename T, size_t N> constexpr size_t countof(T(&)[N]) { return N; }

//constexpr int PCF_ADDRESSES[] = {0x38}; //TODO make sure those are real values
constexpr int PCF_ADDRESSES[] = {0x20, 0x21, 0x22, 0x38, 0x39, 0x3A}; //TODO make sure those are real values
constexpr int PCF_RELAY_OFF[]  = {1,    1,    1,    0,    0,    0}; //defines which relays are Low and which High-Level-Trigger
constexpr int PCF_RELAY_ON[] = {0,    0,    0,    1,    1,    1};

constexpr int PCA_ADDRESS = 0x40;
constexpr int PWM_FREQUENCY = 480; //Hz
constexpr int PCA_LED_ON_STATE = 4095;
constexpr int PCA_LED_OFF_STATE = 0;

constexpr int PCF_COUNT = countof(PCF_ADDRESSES); //6;
constexpr int PCF_PINS = 8;
constexpr int LED_LIGHT_COUNT = 16;
constexpr int AC_LIGHT_COUNT = PCF_COUNT * PCF_PINS; 
constexpr int TOTAL_LIGHT_COUNT = LED_LIGHT_COUNT + AC_LIGHT_COUNT;
constexpr int AC_BUTTON_PINS[AC_LIGHT_COUNT] = {  
  2,  3,  4,  5,  6,  7,  8,  9,   // for 0x20
  10, 11, 14, 15, 16, 17, 18, 19,  // for 0x21
  22, 23, 24, 25, 26, 27, 28, 29,   // for 0x22
  30, 31, 32, 33, 34, 35, 36, 37,   // for 0x38
  38, 39, 40, 41, 42, 43, 44, 45,   // for 0x39
  46, 47, 48, 49, 50, 51, 52, 53    // for 0x3A
};

constexpr int LED_BUTTON_PINS[LED_LIGHT_COUNT] = {
  A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15 // for 0x40
};

constexpr int LINEAR_PWM_LED[101] = {
   0, 
   1, 3, 6, 10, 14, 19, 25, 32, 40, 48, 
   57, 67, 78, 90, 102, 116, 130, 144, 160, 177, 
   194, 212, 231, 250, 271, 292, 314, 337, 361, 385, 
   411, 437, 464, 491, 520, 549, 579, 610, 642, 674, 
   708, 742, 777, 812, 849, 886, 924, 963, 1003, 1044, 
   1085, 1127, 1170, 1214, 1258, 1304, 1350, 1397, 1445, 1493, 
   1543, 1593, 1644, 1696, 1748, 1802, 1856, 1911, 1967, 2023, 
   2081, 2139, 2198, 2258, 2318, 2380, 2442, 2505, 2569, 2633, 
   2699, 2765, 2832, 2900, 2968, 3038, 3108, 3179, 3251, 3324, 
   3397, 3471, 3547, 3622, 3699, 3777, 3855, 3934, 4014, 4095   
};

int working = 0;

#ifdef DONT_SAVE
  volatile int states[TOTAL_LIGHT_COUNT] = {0};
#endif
volatile int ledPercentages[LED_LIGHT_COUNT];

PCF8574 expanders[PCF_COUNT]; 
Adafruit_PWMServoDriver ledDriver = Adafruit_PWMServoDriver(PCA_ADDRESS); 

Bounce debouncers[TOTAL_LIGHT_COUNT];

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
  for(int i = 0; i < TOTAL_LIGHT_COUNT; ++i) {
    int pin = i < AC_LIGHT_COUNT ? AC_BUTTON_PINS[i] : LED_BUTTON_PINS[i - AC_LIGHT_COUNT];
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
  for(int i = 0; i < countof(ledPercentages); i++) {
    ledPercentages[i] = PCA_LED_ON_STATE; //TODO initialize from memory
  }
}

void ledWrite(int i, int newState) {
  int v = newState ? ledPercentages[i] : PCA_LED_OFF_STATE;
  ledDriver.setPin(i, v);
  Serial.print("Changing led ");
  Serial.print(i);
  Serial.print(" to ");
  Serial.print(newState); 
  Serial.print(", pwm set to ");
  Serial.println(v);
}

void acWrite(int i, int newState) {
  int expander = i / PCF_PINS; //TODO use bitshifts (since it's 8) EDIT: It should be optimized out anyway
  int pin = i % PCF_PINS;
  expanders[expander].digitalWrite(pin, newState ? PCF_RELAY_ON[expander] : PCF_RELAY_OFF[expander]);
  Serial.print("changing");
  Serial.print(expander);
  Serial.print(" on ");
  Serial.print(pin);
  Serial.print(" to ");
  Serial.println(newState);
}

void relayWrite(int i, int newState) {
  if (i < AC_LIGHT_COUNT) {
    acWrite(i, newState);
  } else {
    ledWrite(i - AC_LIGHT_COUNT, newState);
  }
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
  for(int i = 0; i < TOTAL_LIGHT_COUNT; ++i) {
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
  for (int i = 0; i < TOTAL_LIGHT_COUNT; ++i) {
    present(i, i < AC_LIGHT_COUNT ? S_LIGHT : S_DIMMER); 
  }
}

void receive(const MyMessage &message) {
  if (message.type == V_LIGHT || message.type == V_DIMMER) {
    if(message.type == V_DIMMER) {
      int dim = atoi(message.data);
      int pwmValue = LINEAR_PWM_LED[dim];
      ledPercentages[message.sensor - AC_LIGHT_COUNT] = pwmValue;
    }
    saveAndSet(message.sensor, message.getBool());
  } 
}
