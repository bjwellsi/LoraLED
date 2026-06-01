#pragma once

#define MAX_QUEUE_SIZE 32

#include "RadioDTO.h"

class RadioMessageTransport{
    public:
        RadioMessageTransport();    
        void tick();
        RadioDTO::Message* nextMessage();
        RadioDTO::Message* sendMessage(RadioDTO::Message message, int resendCount, int timeout);
        void markDone(RadioDTO::Message* message, RadioDTO::AckResponseCode responseCode);
    private:
        RadioDTO::Message inboundQueue[MAX_QUEUE_SIZE];
        RadioDTO::Message inboundLedger[MAX_QUEUE_SIZE];
        int inboundQueueSize;
        int inboundLedgerSize;
        RadioDTO::Message outboundLedger[MAX_QUEUE_SIZE];
        int outboundLedgerSize;
};