#pragma once

#include "../ComDef.h"

namespace SenderStateManagement{
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
    uint16_t handshakeSequence;
  }
}
