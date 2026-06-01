#include "RadioOperations.h"

RadioOperations::RadioOperations(){
  initRadio();
}

  bool RadioOperations::sendPacketBytes(const uint8_t* bytes, size_t size){
    int state = radio.transmit(bytes, size);

    radio.startReceive();

    return state == RADIOLIB_ERR_NONE;
  }

void RadioOperations::receivedPacket(){
    radioReceivedFlag = true;
}

void RadioOperations::initRadio(){
  
    radioReceivedFlag = false;
    int state = radio.begin(LORA_FREQ);
  
    if (state != RADIOLIB_ERR_NONE) {
      Serial.print("Radio init failed, code: ");
      Serial.println(state);
      while (true);
    }

    radio.setPacketReceivedAction([]() {
      RadioOperations::receivedPacket();
    });
    
    radio.startReceive();
  }

  ComDef::PacketBytes RadioOperations::checkForMessages(){
    ComDef::PacketBytes ret;
    if(radioReceivedFlag){  
      radioReceivedFlag = false;

      ret.length = radio.getPacketLength();

      int state = radio.readData(ret.data, ret.length);

      radio.startReceive();
    }
    return ret;
  }


