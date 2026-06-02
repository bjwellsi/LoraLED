#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "ComDef.h"

namespace RadioDTO {
    enum MessageType{
        INVALID,
        ACK,
        SOLID_COLOR,
        FLASH_COLOR,
        STOP_COMMAND,
        TID_ASSIGN,
        UID_REPORT,
        UID_CHIRP_COMMAND
    };


    struct UIDReport{
        uint64_t UID = 0;
    };

    struct TIDAssign{
        uint64_t UID = 0;
        uint8_t TID = 0;
    };

    struct SolidColor{
        uint8_t target = 0;
        CRGB color = 0x000000;
    };

    struct FlashColor{
        uint8_t target = 0;
        CRGB color = 0x000000;
        uint8_t count;
        uint16_t onTime;
        uint16_t offTime;
    };

    struct StopCommand{
        uint8_t target = 0;
    };

    struct UIDChirpCommand{
        uint8_t target = 0;
    };

    struct Ack{
        uint16_t sequenceID = 0;
        ComDef::AckResponseCode responseCode = ComDef::AckResponseCode::INVALID;
    };

    struct Message{
        MessageType messageType = MessageType::INVALID;
        uint16_t sequenceID = 0;
        bool expectsAck = false;
        ComDef::AckResponseCode responseCode = ComDef::AckResponseCode::NOT_STARTED;
        union {
            SolidColor solidColor;
            FlashColor flashColor;
            StopCommand stop;
            UIDReport uidReport;
            TIDAssign tidAssign;
            Ack ack;
            UIDChirpCommand uidChirpCommand;
        };

        Message(){
            stop = {};
        };
    };
}