#pragma once

#include <RadioLib.h>
#include "ComDef.h"

#define LORA_FREQ 915.0

namespace RadioHandler{

  enum RadioState{
    OP_ACTIVE,
    LISTENING,
    OFF
  };

  enum ResponseCode {
    OP_NOT_STARTED,
    OPERATION_ACTIVE,
    OP_INTERRUPTED,
    SENT_NO_ACK,
    ACK_RECEIVED,
    TIMEOUT,
    ACK_ERROR
  };

  using RadioOpStateMachineFunction = void (*)();
  using OnCompleteFunction = void (*)(ResponseCode responseCode);

  //TODO so actually this should be a stack of contexts.
  //IK that's nasty and complex, but that allows interrupted ops to come back online
  //ex, say the sender is in the middle of sending a message that needs an ack, 
  //but a rec comes online in that moment and forces it to send a tid to the rec. 
  //that send tid op requires fresh context. with the current model that's a problem. 
  //so you need a stack
  struct RadioOpContext{
    RadioOpStateMachineFunction activeSM = nullptr;
    bool opActive = false;
    uint8_t packetBytes[64];
    size_t packetSize = 0;
    uint16_t packetSequence = 0;
    ComDef::AckStatus ackStatus = ComDef::NO_RESPONSE;
    int maxRetries = -1;
    int currentRetryCount = -1;
    int timeoutEndTime = -1;
    int waitStart = -1;
    ResponseCode responseCode = OP_NOT_STARTED;
    OnCompleteFunction onCompleteFunction = nullptr;

    void reset(){
      *this = RadioOpContext{};
    }
  };

  struct RadioCallbacks {
    void (*onTIDAssignment)(ComDef::TIDAssignmentPacket p) = nullptr;
    void (*onCommand)(ComDef::CommandPacket p) = nullptr;
    void (*onUIDReport)(ComDef::UIDReportPacket p) = nullptr;

    void reset(){
      *this = RadioCallbacks{};
    }
  };

  void tickRadio();

  void sendTillAckStateMachine();

  void sendTillAckBytes(const uint8_t* bytes, size_t size, int sendCount, int timeout, uint16_t packetSequence, void (*callback)(ResponseCode responseCode));

  template <typename T>
  void sendTillAck(const T& packet, int maxRetries, int timeout, void (*callback)(ResponseCode responseCode)){
    static_assert(sizeof(T) <= 64, "Packet too large");
    
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&packet);
    size_t size = sizeof(T);
    uint16_t sequence = packet.header.sequence;
    sendTillAckBytes(bytes, size, maxRetries, timeout, sequence, callback);
  }

  void sendNTimesStateMachine();

  void sendNTimesBytes(const uint8_t* bytes, size_t size, int sendCount, int timeout, uint16_t packetSequence);

  template <typename T>
  void sendNTimes(const T& packet, int sendCount, int timeout){
    static_assert(sizeof(T) <= 64, "Packet too large");

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&packet);
    size_t size = sizeof(T);
    uint16_t sequence = packet.header.sequence;
    sendNTimesBytes(bytes, size, sendCount, timeout, sequence);
  }

  bool sendPacketBytes(const uint8_t* bytes, size_t size);

  template <typename T> 
  bool sendPacket(const T& packet){
    return sendPacketBytes((const uint8_t*)&packet, sizeof(packet));
  }

  void sendAck(ComDef::AckStatus status, uint16_t sequence);

  void initRadio();

  void checkRadioBuffer();

  void setRadioReceivedFlag();

  void processRawPacket(uint8_t buf[64], size_t len);

  void tickRadio();

  void processStop(ResponseCode endResponseCode);

  void interrupt();

  int nextSequence();
}
