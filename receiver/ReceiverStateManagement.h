#pragma once

#include "../ComDef.h"

namespace ReceiverStateManagement{
  enum ActiveOp {
    IDLE, 
    INIT
  };
  
  enum InitState{
    UNINITIALIZED,
    SENDING_GUID,
    TID_RECEIVED, 
    SENDING_OKAY,
    ERROR,
    INITIALIZED
  };

  struct InitContext{
    InitState initState;
    int waitStart;
    ComDef::HandshakePacket handshakePacket;
    bool handshakeQueued;
    uint16_t mostRecentSequence;
  }
}
