#include <FastLED.h>
#include <RadioLib.h>
#include <Preferences.h>
#include <Arduino.h>
#include "LedDriver.h"
#include "AtomicOps.h"
#include "AnimationTasks.h"
#include "TaskHandling.h"
#include "ComDef.h"

// SX1262 pins for Heltec V3
// Module(NSS, DIO1, RESET, BUSY)
SX1262 radio = new Module(8, 14, 12, 13);

Preferences prefs;

#define DATA_PIN  5
#define LEDS_PER_TUBE 14
#define LED_TYPE  WS2811
#define COLOR_ORDER BRG
#define LORA_FREQ 915.0

CRGB* leds = nullptr;
uint8_t tubeCount;
uint8_t TransientID;
int totalLEDs;
TaskHandle_t animationTaskHandle = nullptr;
volatile bool stopAnimation = false;
volatile bool animationExited = false;

// struct FlashConfig{
//   int count;
//   int timeOn;
//   int timeOff;
//   CRGB color;
// };

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Receiver booting");

  tubeCount = loadTubeCount();
  //TODO Actually init this, hardcoded for now
  tubeCount = 4;
  TransientID = 1;
  totalLEDs = tubeCount * LEDS_PER_TUBE;
  
  Serial.print("Loaded ");
  Serial.print(totalLEDs);
  Serial.print(" LEDS across ");
  Serial.print(tubeCount);
  Serial.println(" tubes");
  

  leds = new CRGB[totalLEDs];
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, totalLEDs);
  FastLED.setBrightness(100);
  FastLED.clear(true);
  
  int state = radio.begin(LORA_FREQ);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Radio init failed, code: ");
    Serial.println(state);
    TaskHandling::startAnimation(&AnimationTasks::errAnimationTask, nullptr);
    while (true);
  }

  TaskHandling::startAnimation(&AnimationTasks::bootAnimationTask, nullptr);

  Serial.println("Receiver ready");
}

void loop() {
  ComDef::Packet packet;

  int state = radio.receive((uint8_t*)&packet, sizeof(packet));
  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("Received command");
    processCommand(packet);
  }
  // else ignore (no packet / timeout)
}

void processCommand(ComDef::Packet packet){

  //parse out the bytes
  if(packet.targetId != TransientID)
  {
    //command isn't for this reciever
    return;
  }

  //command def for now
  //0 - off
  //1 - static
    //color is defined in p1,2,3
  //2 - flash 
    //color is defined in p1,2,3, count is 4, off time is p5,6 (decomposed 16bit val. high byte first), on time is p7, 8
  if (packet.command == 0) AtomicOps::setColorAnimation(CRGB::Black);
  else if (packet.command == 1) {
    CRGB color = {packet.p1, packet.p2, packet.p3};
    AtomicOps::setColorAnimation(color);
  }
  else if (packet.command == 2){
    CRGB color = {packet.p1, packet.p2, packet.p3};
    uint8_t count = packet.p4; 
    uint16_t offTime = ((uint16_t)packet.p5 << 8) | packet.p6;
    uint16_t onTime = ((uint16_t)packet.p7 << 8) | packet.p8;
    AnimationTasks::FlashConfig* flashConf = new AnimationTasks::FlashConfig{
      .count = int(count),
      .timeOn = int(onTime),
      .timeOff = int(offTime),
      .color = color
    };
    Serial.println("Starting flash task");
    TaskHandling::startAnimation(&AnimationTasks::flashAnimationTask, flashConf);
  }
}

void initializeReceiver(){
  //so now the init flow is this
  //boot. if you have a uid/tube count, load them.
  //if you don't load defaults. 
  //when you get a command to init, start blinking and transmitting your uid. 
  TaskHandling::startAnimation(&AnimationTasks::idAssignmentAnimationTask, nullptr);
  //between transmissions, listen for a response containing your uid and a transid
  uint8_t uid = 1;
  saveTransientID(uid);
  AtomicOps::setColorAnimation(CRGB::Black);

  //now you can just listen like normal. 
  //the next step will likely be a tube count transmission signal, but this will be completely initialized via the sender.
  //what it'll do is assign you a blinking color, once the end user ids you by that color, it'll ask for tube count, they input, then it transmits.
  //but basically all that is purely sender side logic.
}

uint64_t getUID(){
  return ESP.getEfuseMac();
}

void clearMemory(){
  saveTubeCount(0);
  saveTransientID(0);
}

void saveTransientID(uint8_t id){
  prefs.begin("config", false);
  prefs.putUChar("id", id);
  prefs.end();
}

uint8_t loadTransientID(){
  prefs.begin("config", true);
  uint8_t id = prefs.getUChar("id", 0);
  prefs.end();
  return id;
}

void saveTubeCount(uint8_t count){
  prefs.begin("config", false);
  prefs.putUChar("tubes", count);
  prefs.end();
}

uint8_t loadTubeCount(){
  prefs.begin("config", true);
  uint8_t count = prefs.getUChar("tubes", 1);
  prefs.end();
  return count;
}