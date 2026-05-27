#pragma once

#include "ComDef.h"

namespace SenderStateManagement{
  enum ActiveOp {
    IDLE, 
    ASSIGNING_TIDS
  };
  
  enum InitState{
    UNINITIALIZED,
    ERROR,
    INITIALIZED
  };

  struct AssignMissingTIDsContext{
    int currentReceiverIndex = 0;
    bool awaitingAck = false;
    
    void reset(){
      *this = AssignMissingTIDsContext{};
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
