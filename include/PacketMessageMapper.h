#pragma once

#include "ComDef.h"
#include "RadioDTO.h"

namespace PacketMessageMapper {
    RadioDTO::Message retrieveMessage(ComDef::PacketBytes);
    ComDef::PacketBytes generatePacket(RadioDTO::Message);
}