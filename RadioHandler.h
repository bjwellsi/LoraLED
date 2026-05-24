#pragma once

#include <RadioLib.h>

#define LORA_FREQ 915.0

namespace RadioHandler{

  using RadioOpStateMachineFunction = void (*)();
  using OnCompleteFunction = void (*)(ResponseCode responseCode);

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
    void (*onHandshake)(const ComDef::Handshake& p) = nullptr;
    void (*onCommand)(const ComDef::CommandPacket& p) = nullptr;
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
}
