#pragma once

#include <RadioLib.h>

#define LORA_FREQ 915.0

extern SX1262 radio;
extern volatile bool receivedFlag;

namespace RadioHandler{
  struct RadioCallbacks {
    void (*onHandshake)(const ComDef::Handshake& p);
    void (*onCommand)(const ComDef::CommandPacket& p);
    void (*onAc)(const ComDef::Ack& p);
  }

  template <typename T>
  bool sendPacket(const T& packet);

  void sendAck(ComDef::AckStatus status, uint16_t sequence);

  void initRadio();

  void checkRadioBuffer(RadioCallbacks& cb);

  void setRadioReceivedFlag();

  void processRawPacket(uint8_t buf[64], size_t len, RadioCallbacks& cb);

}
