#include <FastLED.h>
#include <Preferences.h>
#include <Arduino.h>
#include "LedDriver.h"
#include "AtomicOps.h"
#include "AnimationTasks.h"
#include "TaskHandling.h"
#include "ComDef.h"
#include "MessageTransport.h"

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
MessageTransport messageTransport;

uint8_t loadTubeCount();
void clearMemory();
uint64_t getUID();
void saveTransientID(uint8_t id);
uint8_t loadTransientID();
void saveTubeCount(uint8_t count);
uint8_t loadTubeCount();
void handleMessage(RadioDTO::Message *incoming);

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
  
  messageTransport.assignTID(TransientID);
  messageTransport.assignUID(getUID());
  TaskHandling::startAnimation(&AnimationTasks::bootAnimationTask, nullptr);
  
  delay(1000);
  Serial.println("Receiver ready");
}

void loop() {
  messageTransport.tick();
  if(messageTransport.messageWaiting()){
    RadioDTO::Message *message = messageTransport.nextMessage();

    Serial.print("Handling message: ");
    Serial.print(message->sequenceID);
    Serial.print(" from ");
    Serial.println(message->senderId.idKind == ComDef::IDKind::TID ? message->senderId.TID : message->senderId.UID);
    handleMessage(message);
  }
}

void handleMessage(RadioDTO::Message *incoming){
  //TODO
  switch (incoming->messageType) {
    case RadioDTO::MessageType::STOP_COMMAND:
      AtomicOps::setColorAnimation(CRGB::Black);
      messageTransport.markDone(incoming, ComDef::AckResponseCode::SUCCESS);
    break;
    case RadioDTO::MessageType::UID_CHIRP_COMMAND:
      delay(20);
      TaskHandling::startAnimation(&AnimationTasks::idAssignmentAnimationTask, nullptr);
      RadioDTO::Message uidReport;
      uidReport.expectsAck = true;
      uidReport.senderId.idKind = ComDef::IDKind::UID;
      uidReport.senderId.UID = getUID();
      uidReport.targetId = incoming->senderId;

      messageTransport.sendMessage(uidReport, -1, 3000);
      messageTransport.markDone(incoming, ComDef::AckResponseCode::SUCCESS);
    break;
    case RadioDTO::MessageType::FLASH_COLOR:
      AnimationTasks::FlashConfig* flashConf = new AnimationTasks::FlashConfig{
        .count = incoming->flashColor.count,
        .timeOn = incoming->flashColor.onTime,
        .timeOff = incoming->flashColor.offTime,
        .color = incoming->flashColor.color
      };

      Serial.println("Starting flash task");
      TaskHandling::startAnimation(&AnimationTasks::flashAnimationTask, flashConf);
      messageTransport.markDone(incoming, ComDef::AckResponseCode::SUCCESS);
    break;
    case RadioDTO::MessageType::SOLID_COLOR:
      AtomicOps::setColorAnimation(incoming->solidColor.color);
      messageTransport.markDone(incoming, ComDef::AckResponseCode::SUCCESS);
    break;
    case RadioDTO::MessageType::TID_ASSIGN:
      saveTransientID(incoming -> tidAssign.newTID);
      messageTransport.markDone(incoming, ComDef::AckResponseCode::SUCCESS);
    break;
  }
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