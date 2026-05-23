#include <FastLED.h>
#include <RadioLib.h>
#include <Preferences.h>
#include <Arduino.h>
#include "../LedDriver.h"
#include "../AtomicOps.h"
#include "../AnimationTasks.h"
#include "../TaskHandling.h"
#include "../ComDef.h"
#include "ReceiverStateManagement.h"

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
volatile bool radioReceivedFlag = true;
ReceiverStateManagement::ActiveOP activeOp;
ReceiverStateManagement::InitContext initContext;

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
  activeOp = IDLE;
  initContext.initState = UNINITIALIZED;
  receivedFlag = false;
  
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
  radio.setPacketReceivedAction(setRadioReceivedFlag);
  radio.startReceive();

  TaskHandling::startAnimation(&AnimationTasks::bootAnimationTask, nullptr);

  Serial.println("Receiver ready");
}

void loop() {
  //first check if any commands have come in
  checkRadioBuffer();
  //then progress any active state machines
  switch(activeOp){
    case INT: 
      intializeReceiver();
      break;
  }
}

template <typename T>
bool sendPacket(const T& packet){
  int state = radio.transmit((uint8_t*)&packet, sizeof(T));

  radio.startReceive();

  return state == RADIOLIB_ERR_NONE;
}

void checkRadioBuffer(){
  if(radioReceivedFlag){
    radioReceivedFlag = false;

    uint8_t buf[64];
    size_t len = radio.getPacketLength();

    int state = radio.readData(buf, len);
    if(state == RADIOLIB_ERR_NONE){
      processRawPacket(buf, len);
    }

    radio.startReceive();
  }

}

void processRawPacket(uint8_t buf[64], size_t len){
    ComDef::PacketHeader* header = (ComDef::PacketHeader*)buf;

    if(header->packetType == HANDSHAKE){
      if(len != sizeof(ComDef::HandshakePacket)) return;
      ComDef::HandshakePacket* p = (ComDef::HandshakePacket*)buf;
      processHandshake(*p);
    }else if(header->packetType == COMMAND){
      if(len != sizeof(ComDef::CommandPacket)) return;
      ComDef::CommandPacket* p = (ComDef::CommandPacket*)buf;
      processCommand(*p);
    }
}

void setRadioReceivedFlag() {
  radioReceivedFlag = true;
}


void processHandshake(ComDef::HandshakePacket packet){
  if(packet.UID == getUID() && activeOp == INIT){
    //we care about it only if if was meant for this particular rec and if the op is currently init
    initContext.handshakePacket = packet;
    initContext.handshakeQueued = true;
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
    //stop init flow
    activeOps = IDLE;
    AtomicOps::setColorAnimation(CRGB::Black);
  }
  else if (packet.command == 1) {
    activeOps = INIT;
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

  if(activeOp = IDLE){
    activeOp = INIT;
  }

  if (initContext.initState == ERROR){
    TaskHandling::startAnimation(&AnimationTasks::errAnimationTask, nullptr);
    initContext.initState = UNINITIALIZED;
    initContext.handshakePacket = {.UID = 0, .TransientID = 0, .sender = 0}
    activeOp = IDLE;
  }
  else if(initContext.initState == UNINITIALIZED){
    //very start of init flow
    //do the first set of non blocking tasks
    //reset your transid
    TransientID = 0;

    //spin up a task to start blinking your idAssignmentAnimation. 
    TaskHandling::startAnimation(&AnimationTasks::idAssignmentAnimationTask, nullptr);
    
    initContext.handshakePacket = {.UID = getUID(), .TransientID = 0, .sender = 0};
    initContext.handshakeQueued = false;
    initContext.initState = SENDING_GUID;
    //progress to stage 2 (1 recursive call)
    //setting waitStart to be 0 so 1 call will happen right away
    initContext.waitStart = 0;
    initializeReceiver();
  }
  else if(initContext.initState == SENDING_GUID){
    if (initContext.handshakeQueued){
      initContext.handshakeQueued = false;
      initContext.initState = TID_RECEIVED;
    }
    else if (millis() - initContext.waitStart > random(20, 200)) {
      //Send the packet
      sendPacket(initContext.handshakePacket)
      initContext.waitStart = millis();
      //allow the event loop to take back over
    }
  }
  else if(initContext.initState == TID_RECEIVED){
    //process packet
    ComDef::HandshakePacket packet = initContext.handshakePacket;
    if(packet.sender != 1 || packet.UID != getUID() || packet.TransientID = 0){
      //this packet wasn't for you or is invalid
      initContext.initState = SENDING_GUID;
      initContext.waitStart = millis();
    }
    else {
      //save the transientID
      saveTransientID(packet.TransientID);
      initContext.handshakePacket.sender = 0;
      initContext.initState = SENDING_OKAY; 
      initContext.waitStart = 0;
      initializeReceiver();
    }
  }
  else if(initContext.initState == SENDING_OKAY){
    //TODO
    //
    //SO actually I think here onward is redundant. 
    //We should send 1 okay message after saving the transid. 
    //then we simply save the most recent message id, 
    //and if we hear a retransmission of that id, reack. otherwise we just assume the parent heard us
    if(initContext.handshakeQueued){
      initContext.handshakeQueued = false;
      initContext.initState = OKAY_RECEIVED;
    }
    if(millis() - initContext.waitStart > random(20, 200)) {
      //send the packet
      sendPacket(initContext.handshakePacket)
      initContext.waitStart = millis();
    }
    //next state change will be handled by a radio command coming in
  }
  else if(initContext.initState == OKAY_RECEIVED){
    ComDef::HandshakePacket packet = initContext.handshakePacket;
    if(packet.sender != 1 || packet.uid != getUID() || packet.TransientID != )
    //you're done, kill the init context
    initContext.handshakePacket = {.UID = 0, .sender = 0, .TransientID = 0};
    //kill the idassignment animation and turn off the tube
    TaskHandling::stopCurrentAnimation();
    AtomicOps::setColorAnimation(CRGB::Black);
    initContext.initState = INITIALIZED;
    activeOp = IDLE;
  }
  else if(initContext.initState == INITIALIZED){
    //this should only be reached if the ack wasn't received by the sender
    //check the message id of your new message and the ids received in the new message
    //if either match, response with an affirmative ack
    //TODO
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
