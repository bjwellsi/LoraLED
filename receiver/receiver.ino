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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Receiver booting");

  tubeCount = loadTubeCount();
  TransientID = loadTransientID();
  //TODO Actually init these, hardcoded for now
  TransientID = 1;
  tubeCount = 4;
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
  ComDef::CommandPacket packet;

  int state = radio.receive((uint8_t*)&packet, sizeof(packet));
  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("Received command");
    processCommand(packet);
  }
  // else ignore (no packet / timeout)
}

void sendHandshake(ComDef::Handshake handshake) {
  int state = radio.transmit((uint8_t*)handshake, sizeof(handshake));

  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("Sent handshake");
  } else {
    Serial.print("Send failed, code: ");
    Serial.println(state);
  }
}

void processCommand(ComDef::CommandPacket packet){

  //parse out the bytes
  if(packet.targetId != 0 && packet.targetId != TransientID)
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
  if (packet.command == 0) {
    //stop command
    //TODO stop init flow
    AtomicOps::setColorAnimation(CRGB::Black);
  }
  else if (packet.command == 1) {
    //TODO make this a task
    //Will require generalizing your TaskHandler better
    initializeReceiver();
  }
  else if (packet.command == 20) {
    CRGB color = {packet.p1, packet.p2, packet.p3};
    AtomicOps::setColorAnimation(color);
  }
  else if (packet.command == 21){
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

  //this is triggered by recieving an init command
  //on reception, you do the following

  //reset your transid
  TransientID = 0;

  //spin up a task to start blinking your idAssignmentAnimation. 
  TaskHandling::startAnimation(&AnimationTasks::idAssignmentAnimationTask, nullptr);

  //get your guid
  uint64_t guid = getUID();
  
  //start periodically transmitting it. 
  //between each transmission, you should wait and see if you've recieved a response including 1 your guid, and 2, your new tid. 
  //the retransmission period should be randomized to avoid collisions
  //TODO the three points above
  TransientID = 1;

  //once you get a tid, save it 
  saveTransientID(TransientID);

  //response with an okay signal 
  //TODO

  //kill the idassignment animation and turn off the tube
  TaskHandling::stopCurrentAnimation();
  AtomicOps::setColorAnimation(CRGB::Black);

  //then you're done initializing. you can go back to just listening for commands
  //right now this will be a syncronous op, but we may want to eventaully change that so we can cancel initialization attempts without a reboot 
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