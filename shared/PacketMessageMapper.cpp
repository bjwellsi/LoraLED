#include "PacketMessageMapper.h"

namespace PacketMessageMapper {
    RadioDTO::Message retrieveMessage(ComDef::PacketBytes packet){
        ComDef::PacketHeaderPrefix* headerPrefix{};
        if (packet.length < sizeof(ComDef::PacketHeaderPrefix)) {
            return RadioDTO::Message{};
        }

        memcpy(&headerPrefix, packet.data, sizeof(headerPrefix));
        
        ComDef::IDKind senderIDKind = static_cast<ComDef::IDKind>(headerPrefix->senderIdKind);
        ComDef::IDKind receiverIDKind = static_cast<ComDef::IDKind>(headerPrefix->receiverIdKind);

        if(senderIDKind == ComDef::IDKind::TID && receiverIDKind == ComDef::IDKind::TID){
            return retrieveMessageFromHeader<uint8_t, uint8_t>(packet); 
        }
        else if(senderIDKind == ComDef::IDKind::UID && receiverIDKind == ComDef::IDKind::TID){
            return retrieveMessageFromHeader<uint64_t, uint8_t>(packet); 
        }
        else if(senderIDKind == ComDef::IDKind::TID && receiverIDKind == ComDef::IDKind::UID){
            return retrieveMessageFromHeader<uint8_t, uint64_t>(packet); 
        }
        else if(senderIDKind == ComDef::IDKind::UID && receiverIDKind == ComDef::IDKind::UID){
            return retrieveMessageFromHeader<uint64_t, uint64_t>(packet); 
        }

        return RadioDTO::Message{};
    }   

    
    ComDef::PacketBytes generatePacket(RadioDTO::Message message){
        if(message.senderId.idKind == ComDef::IDKind::TID && message.targetId.idKind == ComDef::IDKind::TID){
            return generatePacketTemplate<uint8_t, uint8_t>(message); 
        }
        else if(message.senderId.idKind == ComDef::IDKind::UID && message.targetId.idKind == ComDef::IDKind::TID){
            return generatePacketTemplate<uint64_t, uint8_t>(message); 
        }
        else if(message.senderId.idKind == ComDef::IDKind::TID && message.targetId.idKind == ComDef::IDKind::UID){
            return generatePacketTemplate<uint8_t, uint64_t>(message); 
        }
        else if(message.senderId.idKind == ComDef::IDKind::UID && message.targetId.idKind == ComDef::IDKind::UID){
            return generatePacketTemplate<uint64_t, uint64_t>(message); 
        }

        return ComDef::PacketBytes{};
    }
}