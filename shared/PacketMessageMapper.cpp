#include "PacketMessageMapper.h"

namespace PacketMessageMapper {
    RadioDTO::Message retrieveMessage(ComDef::PacketBytes packet){
        //take the packet bytes, figure out what kind of packet you're looking at
        //use that to generate a message         
        RadioDTO::Message ret;
        ComDef::PacketHeader* header = (ComDef::PacketHeader*)packet.data;
        ret.expectsAck = header->expectsAck;
        ret.sequenceID = header->sequence;
        ret.responseCode = ComDef::AckResponseCode::NOT_STARTED;
        switch (header -> packetType){
            case ComDef::PacketType::UID_REPORT:
                ComDef::UIDReportPacket* uid = (ComDef::UIDReportPacket*)packet.data;
                ret.messageType = RadioDTO::MessageType::UID_REPORT;

                ret.uidReport.UID = uid->UID;
                break;
            case ComDef::PacketType::ACK:
                ComDef::Ack* ack = (ComDef::Ack*)packet.data;
                ret.messageType = RadioDTO::MessageType::ACK;

                ret.ack.responseCode = (ComDef::AckResponseCode)ack->status;
                ret.ack.sequenceID = ack->header.sequence;
                break;
            case ComDef::PacketType::COMMAND:
                ComDef::CommandPacket* cmd = (ComDef::CommandPacket*)packet.data;

                switch (cmd -> command){
                    case ComDef::Command::STOP:
                        ret.messageType = RadioDTO::MessageType::STOP_COMMAND;
                        ret.stop.target = cmd->targetId;
                        break;
                    case ComDef::Command::TRANSMIT_UID:
                        ret.messageType = RadioDTO::MessageType::UID_CHIRP_COMMAND;
                        ret.uidChirpCommand.target = cmd->targetId;
                        break;
                    case ComDef::Command::SOLID_COLOR:
                        ret.messageType = RadioDTO::MessageType::SOLID_COLOR;
                        ret.solidColor.target = cmd->targetId;
                        ret.solidColor.color = CRGB(cmd->p1, cmd->p2, cmd->p3);
                        break;
                    case ComDef::Command::FLASH:
                        ret.messageType = RadioDTO::MessageType::FLASH_COLOR;
                        ret.flashColor.target = cmd->targetId;
                        ret.flashColor.color = CRGB(cmd->p1, cmd->p2, cmd->p3);
                        ret.flashColor.count = cmd->p4;
                        ret.flashColor.offTime = ((uint16_t)cmd->p5 << 8) | cmd->p6;
                        ret.flashColor.onTime = ((uint16_t)cmd->p7 << 8) | cmd->p8;
                        break;
                }
                
                break;
            case ComDef::PacketType::TID_ASSIGN:
                ComDef::TIDAssignmentPacket* tid = (ComDef::TIDAssignmentPacket*)packet.data;
                ret.messageType = RadioDTO::MessageType::TID_ASSIGN;

                ret.tidAssign.TID = tid->TransientID;
                ret.tidAssign.UID = tid->UID;
                break;
        }

        return ret;
    }   

    ComDef::PacketBytes generatePacket(RadioDTO::Message message){
        //take the message, generate a packet based on it
        //turn that packet into raw bytes
        ComDef::PacketBytes ret;
        switch (message.messageType){
            case RadioDTO::MessageType::ACK:
                ComDef::Ack ack;
                ack.header.expectsAck = message.expectsAck;
                ack.header.packetType = ComDef::PacketType::ACK;
                ack.header.sequence = message.sequenceID;

                ack.status = message.ack.responseCode;

                ret.data = reinterpret_cast<uint8_t*>(&ack);
                size_t len = sizeof(ack);
                break;
            case RadioDTO::MessageType::FLASH_COLOR:
                ComDef::CommandPacket cmd;
                cmd.command = ComDef::Command::FLASH;
                cmd.header.expectsAck = message.expectsAck;
                cmd.header.packetType = ComDef::PacketType::COMMAND;
                cmd.header.sequence = message.sequenceID;

                cmd.targetId = message.flashColor.target;
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
                cmd.command = ComDef::Command::SOLID_COLOR;
                cmd.header.expectsAck = message.expectsAck;
                cmd.header.packetType = ComDef::PacketType::COMMAND;
                cmd.header.sequence = message.sequenceID;

                cmd.targetId = message.solidColor.target;
                //color is in p1,2,3
                cmd.p1 = message.solidColor.color.r;
                cmd.p2 = message.solidColor.color.g;
                cmd.p3 = message.solidColor.color.b;
                
                ret.data = reinterpret_cast<uint8_t*>(&cmd);
                size_t len = sizeof(cmd);
                break;
            case RadioDTO::MessageType::STOP_COMMAND:
                ComDef::CommandPacket cmd;
                cmd.command = ComDef::Command::STOP;
                cmd.header.expectsAck = message.expectsAck;
                cmd.header.packetType = ComDef::PacketType::COMMAND;
                cmd.header.sequence = message.sequenceID;

                cmd.targetId = message.stop.target;

                ret.data = reinterpret_cast<uint8_t*>(&cmd);
                size_t len = sizeof(cmd);
                break;
            case RadioDTO::MessageType::UID_REPORT:
                ComDef::UIDReportPacket uid;
                uid.header.expectsAck = message.expectsAck;
                uid.header.packetType = ComDef::PacketType::UID_REPORT;
                uid.header.sequence = message.sequenceID;
                
                uid.UID = message.uidReport.UID;
                
                ret.data = reinterpret_cast<uint8_t*>(&uid);
                size_t len = sizeof(uid);
                break;
            case RadioDTO::MessageType::UID_CHIRP_COMMAND:
                ComDef::CommandPacket cmd;
                cmd.command = ComDef::Command::TRANSMIT_UID;
                cmd.header.expectsAck = message.expectsAck;
                cmd.header.packetType = ComDef::PacketType::COMMAND;
                cmd.header.sequence = message.sequenceID;
                
                cmd.targetId = message.uidChirpCommand.target;
                
                ret.data = reinterpret_cast<uint8_t*>(&cmd);
                size_t len = sizeof(cmd);
                break;
            case RadioDTO::MessageType::TID_ASSIGN:
                ComDef::TIDAssignmentPacket tid;
                tid.header.expectsAck = message.expectsAck;
                tid.header.packetType = ComDef::PacketType::TID_ASSIGN;
                tid.header.sequence = message.sequenceID;

                tid.UID = message.tidAssign.UID;
                tid.TransientID = message.tidAssign.TID;
        
                ret.data = reinterpret_cast<uint8_t*>(&tid);
                size_t len = sizeof(tid);
                break;
        }
    return ret;
    }
}