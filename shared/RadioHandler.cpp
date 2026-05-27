#include "RadioHandler.h"

extern SX1262 radio;
extern volatile bool radioReceivedFlag;
extern volatile RadioHandler::RadioState radioState;
extern RadioHandler::RadioOpContext radioOpContext;
extern RadioHandler::RadioCallbacks radioCallbacks;
extern uint16_t currentSequence;

namespace RadioHandler{

  void tickRadio(){
    checkRadioBuffer();
    if(radioOpContext.opActive && radioOpContext.activeSM){
      radioOpContext.activeSM();
    }
  }

  void sendTillAckStateMachine(){
    if(millis() > radioOpContext.timeoutEndTime || (radioOpContext.maxRetries >= 0 && radioOpContext.currentRetryCount > radioOpContext.maxRetries)){
      processStop(TIMEOUT);
    }
    else if(radioOpContext.ackStatus != ComDef::NO_RESPONSE){
      if(radioOpContext.ackStatus == ComDef::SUCCESS){
        processStop(ACK_RECEIVED);
      }
      else{
        processStop(ACK_ERROR);
      }
    }
    else{
      if(millis() > radioOpContext.waitStart + random(20,200)){
        //send again
        sendPacketBytes(radioOpContext.packetBytes, radioOpContext.packetSize);
        radioOpContext.currentRetryCount++;
        radioOpContext.waitStart = millis();
      }
    }
  }

  void sendTillAckBytes(const uint8_t* bytes, size_t size, int sendCount, int timeout, uint16_t packetSequence, void (*callback)(ResponseCode responseCode)){
    //start the state machine
    if(timeout < 1) timeout = 100000; //gotta have a max timeout of some kind
    radioOpContext.reset();
    radioOpContext.waitStart = 0;

    memcpy(radioOpContext.packetBytes, bytes, size);
    radioOpContext.packetSize = size;
    radioOpContext.packetSequence = packetSequence;

    radioOpContext.timeoutEndTime = millis() + timeout;
    radioOpContext.maxRetries = sendCount;
    radioOpContext.activeSM = sendTillAckStateMachine;
    radioOpContext.opActive = true;
    radioState = OP_ACTIVE; 
    radioOpContext.responseCode = OPERATION_ACTIVE;
    radioOpContext.onCompleteFunction = callback;
  }

  void sendNTimesStateMachine(){
    if(millis() > radioOpContext.timeoutEndTime){
      //exit err
      processStop(TIMEOUT);
    }
    else if(radioOpContext.currentRetryCount >= radioOpContext.maxRetries){
      //exit success
      processStop(SENT_NO_ACK);
    }
    else {
      if(millis() > radioOpContext.waitStart + random(20, 200)){
        //send again
        sendPacketBytes(radioOpContext.packetBytes, radioOpContext.packetSize);
        radioOpContext.currentRetryCount++;
        radioOpContext.waitStart = millis();
      }
    }
  }

  void sendNTimesBytes(const uint8_t* bytes, size_t size, int sendCount, int timeout, uint16_t packetSequence){
    //start the state machine
    if(timeout < 1) timeout = 100000; //gotta have a max timeout of some kind
    radioOpContext.reset();
    radioOpContext.waitStart = 0;

    memcpy(radioOpContext.packetBytes, bytes, size);
    radioOpContext.packetSize = size;
    radioOpContext.packetSequence = packetSequence;

    radioOpContext.maxRetries = sendCount;
    radioOpContext.currentRetryCount = 0;
    radioOpContext.timeoutEndTime = millis() + timeout;
    radioOpContext.activeSM = sendNTimesStateMachine;
    radioState = OP_ACTIVE;
    radioOpContext.opActive = true;
    radioOpContext.responseCode = OPERATION_ACTIVE;
  }

  bool sendPacketBytes(const uint8_t* bytes, size_t size){
    int state = radio.transmit(bytes, size);

    radio.startReceive();

    return state == RADIOLIB_ERR_NONE;
  }

  void sendAck(ComDef::AckStatus status, uint16_t sequence){
    ComDef::Ack ack;
    ack.header.packetType = ComDef::ACK;
    ack.header.sequence = sequence;
    ack.status = status;
    
    sendPacketBytes((const uint8_t*)&ack, sizeof(ack));  
  }

  void initRadio(){
    // SX1262 pins for Heltec V3
    // Module(NSS, DIO1, RESET, BUSY)
    radio = new Module(8, 14, 12, 13);
  
    radioReceivedFlag = false;
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
    radioOpContext.onCompleteFunction(radioOpContext.responseCode);
  }

  void interrupt(){
    processStop(OP_INTERRUPTED);
  }

  void processRawPacket(uint8_t buf[64], size_t len){
    ComDef::PacketHeader* header = (ComDef::PacketHeader*)buf; 
    Serial.print("Received packet type: ");
    Serial.println(header->packetType);

    if(header->packetType == ComDef::UID_REPORT){
      if(len != sizeof(ComDef::UIDReportPacket) || !radioCallbacks.onUIDReport) return;
      ComDef::UIDReportPacket* p = (ComDef::UIDReportPacket*)buf;
      radioCallbacks.onUIDReport(*p);
    }else if(header->packetType == ComDef::TID_ASSIGN){
      if(len != sizeof(ComDef::TIDAssignmentPacket) || !radioCallbacks.onTIDAssignment) return;
      ComDef::TIDAssignmentPacket* p = (ComDef::TIDAssignmentPacket*)buf;
      radioCallbacks.onTIDAssignment(*p);
    }else if(header->packetType == ComDef::COMMAND){
      if(len != sizeof(ComDef::CommandPacket) || !radioCallbacks.onCommand) return;
      ComDef::CommandPacket* p = (ComDef::CommandPacket*)buf;
      radioCallbacks.onCommand(*p);
    }else if(header->packetType == ComDef::ACK){
      //Validate the ack
      ComDef::Ack* p = (ComDef::Ack*)buf;
      if(radioOpContext.packetSequence > 0) {
        uint16_t sequence = radioOpContext.packetSequence;
        if(radioOpContext.opActive && radioOpContext.ackStatus == ComDef::NO_RESPONSE && p->header.sequence == sequence){
          //if ack is valid for current context, store its status
          radioOpContext.ackStatus = (ComDef::AckStatus)p->status;
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
