#pragma once

#include <RadioLib.h>

#define LORA_FREQ 915.0

namespace RadioHandler{

  using RadioOpStateMachineFunction = void (*)();
  using OnCompleteFunction = void (*)(ResponseCode responseCode);

  //TODO so actually this should be a stack of contexts.
  //IK that's nasty and complex, but that allows interrupted ops to come back online
  //ex, say the sender is in the middle of sending a message that needs an ack, 
  //but a rec comes online in that moment and forces it to send a tid to the rec. 
  //that send tid op requires fresh context. with the current model that's a problem. 
  //so you need a stack
  template <typename T>
  struct RadioOpContext{
    RadioOpStateMachineFunction activeSM = nullptr;
    bool opActive = false;
    const T* packet = nullptr;
    ComDef::AckStatus ackStatus = NO_RESPONSE;
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

  enum RadioState{
    OP_ACTIVE,
    LISTENING,
    OFF
  };

  enum ResponseCode {
    OP_NOT_STARTED,
    OP_ACTIVE,
    OP_INTERRUPTED,
    SENT_NO_ACK,
    ACK_RECEIVED,
    TIMEOUT,
    ACK_ERROR
  };

  struct RadioCallbacks {
    void (*onTIDAssignment)(const ComDef::TIDAssignmentPacket& p) = nullptr;
    void (*onCommand)(const ComDef::CommandPacket& p) = nullptr;
    void (*onUIDReport)(const ComDef::UIDReportPacket& p) = nullptr;
  };

  void tickRadio();

  template <typename T>
  ResponseCode sendTillAck(const T& packet, int maxRetries, int timeout); 

  template <typename T>
  ResponseCode sendNTimes(const T& packet, int sendCount);

  template <typename T>
  bool sendPacket(const T& packet);

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
