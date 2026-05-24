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
    InitState initState = UNINITIALIZED;
    int waitStart = -1;
    ComDef::HandshakePacket handshakePacket = nullptr;
    bool handshakeQueued = false;
    uint16_t mostRecentSequence = 0;

    void reset(){
      *this = InitContext{};
    }
  };
}
