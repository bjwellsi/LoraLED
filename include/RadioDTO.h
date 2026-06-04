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

    struct EmptyPayload{};

    struct TIDAssign{
        uint8_t newTID = 0;
    };

    struct SolidColor{
        CRGB color = 0x000000;
    };

    struct FlashColor{
        CRGB color = 0x000000;
        uint8_t count;
        uint16_t onTime;
        uint16_t offTime;
    };

    struct Ack{
        Identifier ackingForID;
        uint16_t ackingForSequence;
        ComDef::AckResponseCode responseCode = ComDef::AckResponseCode::INVALID;
        
        MessageKey getAckingForKey() const {
            MessageKey ret;
            ret.sequenceID = this->ackingForSequence;
            ret.senderId = this->ackingForID;
            return ret;
        };
    };

    struct Identifier{
        ComDef::IDKind idKind;
        union {
            uint8_t TID;
            uint64_t UID;
        };

        Identifier(){
            idKind = ComDef::IDKind::TID;
            TID = 0;
        };
        
        bool operator==(const Identifier& other) const {
            return (other.idKind == this->idKind && (idKind == ComDef::IDKind::TID && TID == other.TID) || (idKind == ComDef::IDKind::UID && UID == other.UID));
        };

        bool idMatches(uint8_t tid, uint64_t uid){
            return (idKind == ComDef::IDKind::TID && (tid == TID || TID == 0)) || (idKind == ComDef::IDKind::UID && (uid == UID || UID == 0));
        };
    };

    struct MessageKey{
        Identifier senderId;
        uint16_t sequenceID = 0;

        bool operator==(const MessageKey& other) const {
            return this->senderId == other.senderId && this->sequenceID == other.sequenceID;
        };
    };

    struct MessageKeyHash{
        size_t operator()(const MessageKey& key) const {
            uint32_t kind = static_cast<uint8_t>(key.senderId.idKind);
            uint32_t seq = key.sequenceID;

            if(key.senderId.idKind == ComDef::IDKind::TID){
                //easy case
                return (
                    (kind << 24) | 
                    ((uint32_t(key.senderId.TID)) << 16) |
                    seq
                );
            }
            else {
                //need to take 64 bit key + 16 bit seq + 8 bit id kind and turn into 32 bit hash
                uint64_t uid = key.senderId.UID;
                uint32_t uidFolded = uint32_t(uid >> 32) ^ uint32_t(uid);

                return (
                    uidFolded 
                    ^ (kind << 24)
                    ^ seq
                );
            }
        };
    };

    struct Message{
        MessageType messageType = MessageType::INVALID;
        uint16_t sequenceID = 0;
        bool expectsAck = false;
        ComDef::AckResponseCode responseCode = ComDef::AckResponseCode::NOT_STARTED;
        Identifier senderId;
        Identifier targetId;
        Message* sentAck;
        union {
            EmptyPayload emptyPayload;
            SolidColor solidColor;
            FlashColor flashColor;
            TIDAssign tidAssign;
            Ack ack;
        };

        Message(){
            emptyPayload = {};
            sentAck = nullptr;
        };

        MessageKey getMessageKey() const{
            MessageKey ret;
            ret.sequenceID = this->sequenceID;
            ret.senderId = this->senderId;
            return ret;
        };
    };
}