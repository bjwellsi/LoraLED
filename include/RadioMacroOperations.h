#pragma once

#include "RadioOperations.h"
#include "RadioDTO.h"
#include <vector>

class RadioMacroOperations{
    public:
        void tick();
        void sendMessage(RadioDTO::Message* message, int retryCount, int timeout);
        RadioDTO::Message retrieveMessage();
        bool RadioMacroOperations::messageWaiting();
        void cancelMessage(RadioDTO::Message* message);

    private:
        struct RadioOpContext {
            RadioDTO::Message* message;
            int maxRetryCount = 0;
            int currentRetryCount = 0;
            int timeoutEndtime = 0;
            ComDef::PacketBytes packet;
        };
        std::vector<RadioOpContext> messages;
        RadioOperations radioOperations;
};