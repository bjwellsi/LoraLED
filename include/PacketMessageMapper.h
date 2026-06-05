#pragma once

#include "ComDef.h"
#include "RadioDTO.h"

namespace PacketMessageMapper {

    template <typename SenderIdT, typename ReceiverIdT>
    RadioDTO::Message retrieveMessageFromHeader(ComDef::PacketBytes packet){
        
        RadioDTO::Message ret;
        ComDef::PacketHeader<SenderIdT, ReceiverIdT>* header; 
        if (packet.length < sizeof(ComDef::PacketHeader<SenderIdT, ReceiverIdT>)) {
            return RadioDTO::Message{};
        }

        memcpy(&header, packet.data, sizeof(header));

        ret.expectsAck = header->expectsAck;
        ret.sequenceID = header->prefix.sequence;
        ret.senderId.idKind = header->prefix.senderIdKind;
        if(ret.senderId.idKind == ComDef::IDKind::TID){
            ret.senderId.TID = header->senderId;
        }
        else if(ret.senderId.idKind == ComDef::IDKind::UID){
            ret.senderId.UID = header->senderId;
        }
        ret.targetId.idKind = header->prefix.receiverIdKind;
        if(ret.targetId.idKind == ComDef::IDKind::TID){
            ret.targetId.TID = header->receiverId;
        }
        else if(ret.targetId.idKind == ComDef::IDKind::UID){
            ret.targetId.UID = header->receiverId;
        }

        switch(header -> packetType){
            case ComDef::PacketType::UID_REPORT:
                ret.messageType = RadioDTO::MessageType::UID_REPORT;
                ComDef::UIDReportPacket<SenderIdT, ReceiverIdT>* uidr;
                if (packet.length < sizeof(ComDef::UIDReportPacket<SenderIdT, ReceiverIdT>)) {
                    return RadioDTO::Message{};
                }

                memcpy(&uidr, packet.data, sizeof(uidr));
            break;
            case ComDef::PacketType::ACK:
                ret.messageType = RadioDTO::MessageType::ACK;
                ComDef::ACK<SenderIdT, ReceiverIdT>* ack;
                if (packet.length < sizeof(ComDef::ACK<SenderIdT, ReceiverIdT>)) {
                    return RadioDTO::Message{};
                }

                memcpy(&ack, packet.data, sizeof(ack));
                ret.ack.responseCode = ack.status;
                ret.ack.ackingForSequence = ack.ackingForSequence;
                if(ack.ackingForTID != 0){
                    ret.ack.ackingForID.idKind = ComDef::IDKind::TID;
                    ret.ack.ackingForID.TID = ack.ackingForTID;
                }
                else if(ack.ackingForUID != 0){
                    ret.ack.ackingForID.idKind = ComDef::IDKind::UID;
                    ret.ack.ackingForID.UID = ack.ackingForUID;
                }
            break;
            case ComDef::PacketType::TID_ASSIGN:
                ret.messageType = RadioDTO::MessageType::TID_ASSIGN;
                ComDef::TIDAssignmentPacket<SenderIdT, ReceiverIdT>* tidp;
                if (packet.length < sizeof(ComDef::TIDAssignmentPacket<SenderIdT, ReceiverIdT>)) {
                    return RadioDTO::Message{};
                }

                memcpy(&tidp, packet.data, sizeof(tidp));
                ret.tidAssign.newTID = tidp->newTID;
            break;
            case ComDef::PacketType::COMMAND:
                ComDef::CommandPacket<SenderIdT, ReceiverIdT>* cmd;
                if (packet.length < sizeof(ComDef::CommandPacket<SenderIdT, ReceiverIdT>)) {
                    return RadioDTO::Message{};
                }

                memcpy(&cmd, packet.data, sizeof(cmd));

                switch(cmd->command){
                    case ComDef::Command::STOP:
                        ret.messageType = RadioDTO::MessageType::STOP_COMMAND;
                    break;
                    case ComDef::Command::TRANSMIT_UID:
                        ret.messageType = RadioDTO::MessageType::UID_CHIRP_COMMAND;
                    break;
                    case ComDef::Command::FLASH_TID:
                        ret.messageType = RadioDTO::MessageType::TID_REPORT;
                    break;
                    case ComDef::Command::FLASH:
                        ret.messageType = RadioDTO::MessageType::FLASH_COLOR;
                        ret.flashColor.color = (cmd->p1, cmd->p2, cmd->p3);
                        ret.flashColor.count = cmd->p4;
                        ret.flashColor.offTime = ((uint16_t)cmd->p5 << 8) | cmd->p6;
                        ret.flashColor.onTime = ((uint16_t)cmd->p7 << 8) | cmd->p8;
                    break;
                    case ComDef::Command::SOLID_COLOR:
                        ret.messageType = RadioDTO::MessageType::SOLID_COLOR;
                        ret.solidColor.color = (cmd->p1, cmd->p2, cmd->p3);
                    break;

                }
            break;
        }

        return ret;
    }

    
    template <typename SenderIdT, typename ReceiverIdT>
    ComDef::PacketHeader<SenderIdT, ReceiverIdT> genHeader(RadioDTO::Message message){
        ComDef::PacketHeader<SenderIdT, ReceiverIdT> header;
        header.expectsAck = message.expectsAck;
        header.packetType = message.messageType;
        header.prefix.sequence = message.sequenceID;
        header.prefix.senderIdKind = message.senderId.idKind; 
        header.prefix.receiverIdKind = message.targetId.idKind; 
        if(message.senderId.idKind == ComDef::IDKind::TID){
            header.senderId = message.senderId.TID;
        }
        else if(message.targetId.idKind == ComDef::IDKind::UID){
            header.senderId = message.senderId.UID;
        }
        if(message.targetId.idKind == ComDef::IDKind::TID){
            header.receiverId = message.targetId.TID;
        }
        else if(message.targetId.idKind == ComDef::IDKind::UID){
            header.receiverId = message.targetId.UID;
        }
    }

    template <typename SenderIdT, typename ReceiverIdT>
    ComDef::PacketBytes generatePacketTemplate(RadioDTO::Message message){
        //take the message, generate a packet based on it
        //turn that packet into raw bytes
        ComDef::PacketBytes ret;
        ComDef::PacketHeader<SenderIdT, ReceiverIdT> header = genHeader(message);
        switch (message.messageType){
            case RadioDTO::MessageType::ACK:
                ComDef::Ack ack;
                ack.header = header;

                ack.status = message.ack.responseCode;
                ack.ackingForSequence = message.ack.ackingForSequence;
                
                if(message.ack.ackingForID.idKind == ComDef::IDKind::TID){
                    ack.ackingForTID = message.ack.ackingForID.TID;
                }
                else if(message.ack.ackingForID.idKind == ComDef::IDKind::UID){
                    ack.ackingForUID = message.ack.ackingForID.UID;
                }

                ret.data = reinterpret_cast<uint8_t*>(&ack);
                size_t len = sizeof(ack);
                break;
            case RadioDTO::MessageType::FLASH_COLOR:
                ComDef::CommandPacket cmd;
                cmd.header = header;

                cmd.command = ComDef::Command::FLASH;
                
                //color is in p1,2,3
                cmd.p1 = message.solidColor.color.r;
                cmd.p2 = message.solidColor.color.g;
                cmd.p3 = message.solidColor.color.b;
                //count is 4
                cmd.p4 = message.flashColor.count;
                //on/off time are 5/6, 7/8
                cmd.p5 = (uint8_t)(((uint16_t)message.flashColor.offTime >> 8) & 0xFF);
                cmd.p6 = (uint8_t)(((uint16_t)message.flashColor.offTime) & 0xFF);
                cmd.p7 = (uint8_t)(((uint16_t)message.flashColor.onTime >> 8) & 0xFF);
                cmd.p8 = (uint8_t)(((uint16_t)message.flashColor.onTime) & 0xFF);

                ret.data = reinterpret_cast<uint8_t*>(&cmd);
                size_t len = sizeof(cmd);
                break;
            case RadioDTO::MessageType::SOLID_COLOR:
                ComDef::CommandPacket cmd;
                cmd.header = header;
                
                cmd.command = ComDef::Command::SOLID_COLOR;
                //color is in p1,2,3
                cmd.p1 = message.solidColor.color.r;
                cmd.p2 = message.solidColor.color.g;
                cmd.p3 = message.solidColor.color.b;
                
                ret.data = reinterpret_cast<uint8_t*>(&cmd);
                size_t len = sizeof(cmd);
                break;
            case RadioDTO::MessageType::STOP_COMMAND:
                ComDef::CommandPacket cmd;
                cmd.header = header;

                cmd.command = ComDef::Command::STOP;
                
                ret.data = reinterpret_cast<uint8_t*>(&cmd);
                size_t len = sizeof(cmd);
                break;
            case RadioDTO::MessageType::UID_REPORT:
                ComDef::UIDReportPacket uid;
                uid.header = header;
                
                ret.data = reinterpret_cast<uint8_t*>(&uid);
                size_t len = sizeof(uid);
                break;
            case RadioDTO::MessageType::UID_CHIRP_COMMAND:
                ComDef::CommandPacket cmd;
                cmd.header = header;
                
                cmd.command = ComDef::Command::TRANSMIT_UID;
                                                
                ret.data = reinterpret_cast<uint8_t*>(&cmd);
                size_t len = sizeof(cmd);
                break;
            case RadioDTO::MessageType::TID_ASSIGN:
                ComDef::TIDAssignmentPacket tid;
                tid.header = header;

                tid.newTID = message.tidAssign.TID;
        
                ret.data = reinterpret_cast<uint8_t*>(&tid);
                size_t len = sizeof(tid);
                break;
        }
    return ret;
    }
    
    ComDef::PacketBytes generatePacket(RadioDTO::Message message);
    RadioDTO::Message retrieveMessage(ComDef::PacketBytes packet);
}