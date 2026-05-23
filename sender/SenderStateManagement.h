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
    uint16_t handshakeSequence;
    InitState initState;
    ComDef::CommandPacket commandPacket;
    ComDef::HandshakePacket handshakePacket;
    bool handshakeQueued;
    int increment;
  }
}
