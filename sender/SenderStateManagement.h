#pragma once

#include "../ComDef.h"

namespace SenderStateManagement{
  enum ActiveOp {
    IDLE, 
    INIT
  };
  
  enum InitState{
    UNINITIALIZED,
    ERROR,
    INITIALIZED
  };

  struct InitContext{
    int initTimeout = -1;
    int initBegin = -1;
    uint16_t handshakeSequence = 0;
    InitState initState = UNINITIALIZED;
    ComDef::CommandPacket commandPacket = nullptr;
    ComDef::HandshakePacket handshakePacket = nullptr;
    bool handshakeQueued = false;
    int increment = -1;
    uint64_t currentReceiverTarget = 0;
    bool ackQueued = false;
    ComDef::Ack ack = nullptr;

    void reset(){
      *this = InitContext{};
    }
  };

  struct Receiver{
    uint8_t TransientID = 0;
    uint64_t UID = 0;
    bool initialized = false;

    void reset(){
      *this = Receiver{};
    }
  };
}
