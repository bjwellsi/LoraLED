#include <FastLED.h>
#include <Preferences.h>
#include <Arduino.h>
#include "../LedDriver.h"
#include "../AtomicOps.h"
#include "../AnimationTasks.h"
#include "../TaskHandling.h"
#include "../ComDef.h"
#include "../RadioHandler.h"
#include "ReceiverStateManagement.h"

Preferences prefs;

#define DATA_PIN  5
#define LEDS_PER_TUBE 14
#define LED_TYPE  WS2811
#define COLOR_ORDER BRG

CRGB* leds = nullptr;
uint8_t tubeCount;
uint8_t TransientID;
int totalLEDs;
TaskHandle_t animationTaskHandle = nullptr;
volatile bool stopAnimation = false;
volatile bool animationExited = false;
volatile bool radioReceivedFlag = true;
uint16_t currentSequence;
SX1262 radio;
const RadioHandler::RadioCallBacks radioCallbacks = {.onTIDAssignment = processTIDAssignmentPacket, .onCommand = processCommand, onUIDReport = nullptr};
RadioHandler::RadioOpContext radioOpContext;
RadioHandler::RadioState radioState;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Receiver booting");

  tubeCount = loadTubeCount();
  TransientID = loadTransientID();
  //TODO Actually init this, hardcoded for now
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
  
  RadioHandler::initRadio();
  TaskHandling::startAnimation(&AnimationTasks::bootAnimationTask, nullptr);

  Serial.println("Receiver ready");
}

void loop() {
  RadioHandler::tickRadio();
}

void processTIDAssignmentPacket(ComDef::TIDAssignmentPacket tidPacket) {
  if(getUID() == tidPacket.UID){
    //for now we'll just let this be destructive. 
    //all it's gonna do is as long as the uid matches, assign the tid and ack
    saveTransientID(tidPacket.TransientID);
    RadioHandler::sendAck(SUCCESS, tidPacket.header.sequence);
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
  if (packet.command == STOP) {
    //stop command
    //stop init flow
    activeOps = IDLE;
    AtomicOps::setColorAnimation(CRGB::Black);
  }
  else if(packet.command == REPORT_UID){
    //TODO hardcoded timeouts
    reportUID(-1, 10000);
  }
  else if (packet.command == INIT) {
    activeOps = INIT;
    initializeReceiver();
  }
  else if (packet.command == SOLID_COLOR) {
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

void reportUID(int maxRetries, int timeout){
  UIDReportPacket = p {
    .header = {
      .sequence = RadioHandler::nextSequence(),
      packetType = UID_REPORT;
    }
    .UID = getUID();
  };

  RadioHandler::sendTillAck(p, maxRetries, timeout);
}

uint64_t getUID(){
  return ESP.getEfuseMac();
}

void clearMemory(){
  saveTubeCount(0);
  saveTransientID(0);
}

void saveTransientID(uint8_t id){
  TransientID = id;
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
