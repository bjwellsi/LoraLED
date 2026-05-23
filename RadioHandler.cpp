#include "RadioHandler.h"

namespace RadioHandler{

  struct RadioCallbacks {
    void (*onHandshake)(const ComDef::Handshake& p);
    void (*onCommand)(const ComDef::CommandPacket& p);
    void (*onAc)(const ComDef::Ack& p);
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
    radio.setPacketReceivedAction(setRadioReceivedFlag);
    radio.startReceive();
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
