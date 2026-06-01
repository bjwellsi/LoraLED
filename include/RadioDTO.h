#pragma once

#include <Arduino.h>
#include <FastLED.h>

namespace RadioDTO {
    enum MessageType{
        INVALID,
        ACK,
        SOLID_COLOR,
        FLASH_COLOR,
        STOP_COMMAND,
        UID_REPORT
    };

    enum AckResponseCode{
        INVALID,
        NOT_STARTED,
        IN_PROGRESS,
        SUCCESS,
        ERROR
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
    };

    struct StopCommand{
        uint8_t target = 0;
    };

    struct Message{
        MessageType messageType = MessageType::INVALID;
        uint16_t sequenceID = 0;
        bool expectsAck = false;
        AckResponseCode responseCode = NOT_STARTED;
        union {
            SolidColor solidColor;
            FlashColor flashColor;
            StopCommand stop;
            UIDReport uidReport;
            TIDAssign tidAssign;
        };

        Message(){
            stop = {};
        };
    };
}