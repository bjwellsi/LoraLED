#pragma once

#define MAX_LEDGER_SIZE 32

#include "RadioDTO.h"
#include "RadioMacroOperations.h"
#include <queue>

class MessageTransport{
    public:
        MessageTransport();    
        void tick();
        RadioDTO::Message* nextMessage();
        RadioDTO::Message* sendMessage(RadioDTO::Message message, int resendCount, int timeout);
        void markDone(RadioDTO::Message* message, ComDef::AckResponseCode responseCode);
    private:
        std::queue<RadioDTO::Message*> inboundQueue;
        RadioDTO::Message inboundLedger[MAX_LEDGER_SIZE];
        int inboundLedgerIndex;
        RadioDTO::Message outboundLedger[MAX_LEDGER_SIZE];
        int outboundLedgerIndex;
        RadioMacroOperations radioOps;
        int currentSequence;
};