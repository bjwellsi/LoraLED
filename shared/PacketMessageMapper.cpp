#include "PacketMessageMapper.h"

namespace PacketMessageMapper {
    RadioDTO::Message retrieveMessage(ComDef::PacketBytes){
        //TODO 
        //take the packet bytes, figure out what kind of packet you're looking at
        //use that to generate a message         
    }

    ComDef::PacketBytes generatePacket(RadioDTO::Message){
        //TODO
        //take the message, generate a packet based on it
        //turn that packet into raw bytes
    }
}