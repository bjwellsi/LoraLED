#pragma once

#include "ComDef.h"
#include "RadioDTO.h"

namespace PacketMessageMapper {
    RadioDTO::Message retrieveMessage(ComDef::PacketBytes packet);
    ComDef::PacketBytes generatePacket(RadioDTO::Message message);
}