#include "RadioHandler.h"

extern SX1262 radio;
extern volatile bool receivedFlag;
extern volatile RadioState radioState;
extern RadioOpContext radioOpContext;
extern RadioCallBacks radioCallbacks;

namespace RadioHandler{

  void tickRadio(){
    //TODO
    //so this is a little different than the old strat. 
    //instead of simply listening and forwarding requests, this needs to handle things a little differently. 
    //what it should do is 
    //every loop, check the buffer. 
    //if a req has come in, cast it
    //if it's a command or a handshake, pass it along to the callback provided. 
    //if it's an ack, PROCESS IT yourself
    //basically see if you have an active req waiting for an ack and if the ack you received is for you/the current request
    //if so, then you can save the ack and call the ack callback
    //otherwise tick the current state machine and end the tick
  }

  template <typename T>
  void sendTillAck(const T& packet, int maxRetries, int timeout){
    //start the state machine
    if(timeout < 1) timeout = 100000; //gotta have a max timeout of some kind
    radioOpContext.reset();
    radioOpContext.waitStart = 0;
    radioOpContext.packet = packet;
    radioOpContext.timeoutEndTime = millis() + timeout;
    radioOpContext.maxRetries = maxRetries;
    radioOpContext.activeSM = sendTillAckStateMachine;
    radioOpContext.opActive = true;
    radioState = OP_ACTIVE; 
    radioOpContext.responseCode = OP_ACTIVE;
  }

  static void sendTillAckStateMachine(){
    if(millis() > radioOpContext.timeouEndTime || (radioOpContext.maxRetries >= 0 && radioOpContext.currentRetryCount > radioOpContext.maxRetries)){
      radioOpContext.responseCode = TIMEOUT;
      radioOpContext.opActive = false;
      radioState = LISTENING;
    }
    else if(1=){
      //check if ack received 
      //TODO
      if(1=1){
        //check ack success
        //TODO
        radioOpContext.responseCode = ACK_RECEIVED;
      }
      else {
        //else ack err
        radioOpContext.responseCode = ACK_ERROR;
      }
      radioOpContext.opActive = false;
      radioState = LISTENING;
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
    radioOpContext.packet = packet;
    radioOpContext.maxRetries = sendCount;
    radioOpContext.retryCount = 0;
    radioOpContext.timeoutEndTime = millis() + timeout;
    radioOpContext.activeSM = sendNTimesStateMachine;
    radioState = OP_ACTIVE;
    radioOpContext.opActive = true;
    radioOpContext.responseCode = OP_ACTIVE;
  }

  template <typename T>
  static void sendNTimesStateMachine(){
    if(millis() > radioOpContext.timeoutEndTime){
      //exit err
      radioOpContext.responseCode = TIMEOUT;
      radioOpContext.opActive = false;
      radioState = LISTENING;
    }
    else if(radioOpContext.retryCount >= radioOpContext.maxRetries){
      //exit success
      radioOpContext.responseCode = SENT_NO_ACK;
      radioOpContext.opActive = false;
      radioState = LISTENING;
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

  void checkRadioBuffer(RadioCallbacks& cb){
    if(radioReceivedFlag){
      radioReceivedFlag = false;

      uint8_t buf[64];
      size_t len = radio.getPacketLength();

      int state = radio.readData(buf, len);
      if(state == RADIOLIB_ERR_NONE){
        processRawPacket(buf, len, cb);
      }

      radio.startReceive();
    }
  }

  void setRadioReceivedFlag() {
    radioReceivedFlag = true;
  }

  void processRawPacket(uint8_t buf[64], size_t len, RadioCallbacks& cb){

    ComDef::PacketHeader* header = (ComDef::PacketHeader*)buf; 
    if(header->packetType == HANDSHAKE){
      if(len != sizeof(ComDef::HandshakePacket) || !cb.onHandshake) return;
      ComDef::HandshakePacket* p = (ComDef::HandshakePacket*)buf;
      cb.onHandshake(*p);
    }else if(header->packetType == COMMAND){
      if(len != sizeof(ComDef::CommandPacket) || !cb.onCommand) return;
      ComDef::CommandPacket* p = (ComDef::CommandPacket*)buf;
      cb.onCommand(*p);
    }else if(header->packetType == ACK){

      if(len != sizeof(ComDef::Ack) || !cb.onAck) return;
      ComDef::Ack* p = (ComDef::Ack*)buf;
      cb.onAck(*p);
    }
  }
}
