#pragma once

#include "RadioDTO.h"
#include "RadioMacroOperations.h"
#include <queue>
#include <unordered_map>

class MessageTransport{
    public:
        MessageTransport();    
        void tick();
        RadioDTO::Message* nextMessage();
        RadioDTO::Message* sendMessage(RadioDTO::Message message, int resendCount, int timeout);
        void markDone(RadioDTO::Message* message, ComDef::AckResponseCode responseCode);
        void assignUID(uint64_t uid);
        void assignTID(uint8_t tid);
    private:
        std::queue<RadioDTO::Message*> inboundQueue;
        std::unordered_map<RadioDTO::MessageKey, RadioDTO::Message, RadioDTO::MessageKeyHash> inboundLedger;
        std::unordered_map<RadioDTO::MessageKey, RadioDTO::Message, RadioDTO::MessageKeyHash> outboundLedger;
        RadioMacroOperations radioOps;
        int currentSequence;
        uint8_t TID;
        uint64_t UID;
};