#pragma once

#include <stdint.h>

namespace ComDef{
  enum PacketType : uint8_t {
    HANDSHAKE = 1,
    COMMAND = 2,
    ACK = 3
  }

  struct __attribute__((packed)) PacketHeader {
    uint8_t packetType;
    uint16_t sequence;
  }
  static_assert(sizeof(PacketHeader) == 3);
  
  struct __attribute__((packed)) Handshake {
    PacketHeader header;
    uint8_t TransientId;
    uint64_t UID;
    uint8_t Sender;
  };

  static_assert(sizeof(Handshake) == 13);

  struct __attribute__((packed)) CommandPacket {
    PacketHeader header;
    uint8_t targetId;
    uint8_t command;
    uint8_t p1;
    uint8_t p2;
    uint8_t p3;
    uint8_t p4;
    uint8_t p5;
    uint8_t p6;
    uint8_t p7;
    uint8_t p8;
    uint8_t p9;
    uint8_t p10;
    uint8_t p11;
    uint8_t p12;
    uint8_t p13;
    uint8_t p14;
  };

  static_assert(sizeof(CommandPacket) == 19);
}
