#pragma once

#include <RadioLib.h>
#include "ComDef.h"

#define LORA_FREQ 915.0

class RadioOperations{
    public:
        RadioOperations();

        bool sendPacketBytes(const uint8_t* bytes, size_t size);

        void initRadio();

        ComDef::PacketBytes checkForMessages();

        void setRadioReceivedFlag();

        static void receivedPacket();

        bool messageWaiting();

    private:
        static volatile bool radioReceivedFlag;

        SX1262 radio = new Module(8, 14, 12, 13);
};