#include "RadioHandler.h"

extern SX1262 radio;
extern volatile bool receivedFlag;
extern volatile RadioState radioState;
extern RadioOpContext radioOpContext;
extern RadioCallBacks radioCallbacks;
extern uint16_t currentSequence;

namespace RadioHandler{

  void tickRadio(){
    checkRadioBuffer();
    if(radioOpContext.opActive && radioOpContext.activeSM){
      radioOpContext.activeSM();
    }
  }

  template <typename T>
  void sendTillAck(const T& packet, int maxRetries, int timeout){
    //start the state machine
    if(timeout < 1) timeout = 100000; //gotta have a max timeout of some kind
    radioOpContext.reset();
    radioOpContext.waitStart = 0;
    radioOpContext.packet = &packet;
    radioOpContext.timeoutEndTime = millis() + timeout;
    radioOpContext.maxRetries = maxRetries;
    radioOpContext.activeSM = sendTillAckStateMachine;
    radioOpContext.opActive = true;
    radioState = OP_ACTIVE; 
    radioOpContext.responseCode = OP_ACTIVE;
  }

  static void sendTillAckStateMachine(){
    if(millis() > radioOpContext.timeouEndTime || (radioOpContext.maxRetries >= 0 && radioOpContext.currentRetryCount > radioOpContext.maxRetries)){
      radioOpContext.processStop(TIMEOUT);
    }
    else if(radioOpContext.ackStatus != NO_RESPONSE){
      if(radioOpContext.ackStatus == SUCCESS){
        radioOpContext.processStop(ACK_RECEIVED);
      }
      else{
        radioOpContext.processStop(ACK_ERROR);
      }
    }
    else{
      if(millis() > radioOpContext.waitStart + random(20,200)){
        //send again
        sendPacket(radioOpContext.packet);
        radioOpContext.retryCount++;
        radioOpContext.waitStart = millis();
      }
    }
  }

  template <typename T>
  void sendNTimes(const T& packet, int sendCount, int timeout){
    //start the state machine
    if(timeout < 1) timeout = 100000; //gotta have a max timeout of some kind
    radioOpContext.reset();
    radioOpContext.waitStart = 0;
    radioOpContext.packet = &packet;
    radioOpContext.maxRetries = sendCount;
    radioOpContext.retryCount = 0;
    radioOpContext.timeoutEndTime = millis() + timeout;
    radioOpContext.activeSM = sendNTimesStateMachine;
    radioState = OP_ACTIVE;
    radioOpContext.opActive = true;
    radioOpContext.responseCode = OP_ACTIVE;
  }

  static void sendNTimesStateMachine(){
    if(millis() > radioOpContext.timeoutEndTime){
      //exit err
      radioOpContext.processStop(TIMOUT);
    }
    else if(radioOpContext.retryCount >= radioOpContext.maxRetries){
      //exit success
      radioOpContext.processStop(SENT_NO_ACK);
    }
    else {
      if(millis() > radioOpContext.waitStart + random(20, 200)){
        //send again
        sendPacket(radioOpContext.packet);
        radioOpContext.retryCount++;
        radioOpContext.waitStart = millis();
      }
    }
  }

  template <typename T>
  bool sendPacket(const T& packet){
    int state = radio.transmit((uint8_t*)&packet, sizeof(T));

    radio.startReceive();

    return state == RADIOLIB_ERR_NONE;
  }

  void sendAck(ComDef::AckStatus status, uint16_t sequence){
    ComDef::Ack ack = {.header = {.packetType = ACK, .sequence = sequence} .status = status}
    sendPacket(ack);
  }
  
  void initRadio(){
    // SX1262 pins for Heltec V3
    // Module(NSS, DIO1, RESET, BUSY)
    SX1262 radio = new Module(8, 14, 12, 13);
  
    receivedFlag = false;
    currentSequence = 0;
    int state = radio.begin(LORA_FREQ);
  
    if (state != RADIOLIB_ERR_NONE) {
      Serial.print("Radio init failed, code: ");
      Serial.println(state);
      while (true);
    }
    radioOpContext.reset();
    radio.setPacketReceivedAction(setRadioReceivedFlag);
    radio.startReceive();
    radioState = LISTENING;
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

  void setRadioReceivedFlag() {
    radioReceivedFlag = true;
  }

  void processStop(ResponseCode endResponseCode){
    radioState = LISTENING;
    radioOpContext.opActive = false;
    radioOpContext.responseCode = endResponseCode;
    radioOpContext.onCompleteFunction(radioOpContext.ResponseCode);
  }

  void interrupt(){
    processStop(OP_INTERRUPTED);
  }

  void processRawPacket(uint8_t buf[64], size_t len){
    ComDef::PacketHeader* header = (ComDef::PacketHeader*)buf; 

    if(header->packetType == UID_REPORT){
      if(len != sizeof(ComDef::UIDReportPacket) || !radioCallbacks.onUIDReport) return;
      ComDef::UIDReportPacket* p = (ComDef::UIDReportPacket*)buf;
      radioCallbacks.onUIDReport(*p);
    }else if(header->packetType == TID_ASSIGN){
      if(len != sizeof(ComDef::TIDAssignmentPacket) || !radioCallbacks.onTIDAssignment) return;
      ComDef::TIDAssignmentPacket* p = (ComDef::TIDAssignmentPacket*)buf;
      radioCallbacks.onTIDAssignment(*p);
    }else if(header->packetType == COMMAND){
      if(len != sizeof(ComDef::CommandPacket) || !radioCallbacks.onCommand) return;
      ComDef::CommandPacket* p = (ComDef::CommandPacket*)buf;
      radioCallbacks.onCommand(*p);
    }else if(header->packetType == ACK){
      //Validate the ack
      ComDef::Ack* p = (ComDef::Ack*)buf;
      if(radioOpContext.packet) {
        uint16_t sequence = radioOpContext.packet->header.sequence;
        if(radioOpContext.opActive && radioOpContext.ackStatus == NO_RESPONSE && p.header.sequence == sequence){
          //if ack is valid for current context, store its status
          radioOpContext.ackStatus = p.status;
        }
      }
    }
  }

  int nextSequence(){
    //TODO should probably not return sequence if currently sending op
    //really packet building needs to be officially owned by someone, current interface is leaky
    currentSequence++;
    return currentSequence;
  }
}
