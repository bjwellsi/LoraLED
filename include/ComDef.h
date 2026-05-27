#pragma once

#include <stdint.h>

namespace ComDef{
  enum Command : uint8_t {
    STOP = 0,
    TRANSMIT_UID = 1, 
    SOLID_COLOR = 20,
    FLASH = 21,
    FLASH_TID = 22
  };

  enum PacketType : uint8_t {
    INVALID = 0,
    UID_REPORT = 1,
    COMMAND = 2,
    ACK = 3,
    TID_ASSIGN = 4
  };

  enum AckStatus : uint8_t {
    NO_RESPONSE = 0,
    SUCCESS = 1,
    ERROR = 2
  };

  struct __attribute__((packed)) PacketHeader {
    uint8_t packetType = INVALID;
    uint16_t sequence = 0;

    void reset(){
      *this = PacketHeader{};
    }
  };
  static_assert(sizeof(PacketHeader) == 3);
  
  struct __attribute__((packed)) Ack {
    PacketHeader header;
    uint8_t status = 0;

    void reset(){
      *this = Ack{};
    }
  };
  static_assert(sizeof(Ack) == 4);

  struct __attribute__((packed)) UIDReportPacket {
    PacketHeader header;
    uint64_t UID = 0;

    void reset(){
      *this = UIDReportPacket{};
    }
  };
  static_assert(sizeof(UIDReportPacket) == 11);

  struct __attribute__((packed)) TIDAssignmentPacket {
    PacketHeader header;
    uint64_t UID = 0;
    uint8_t TransientID = 0;

    void reset(){
      *this = TIDAssignmentPacket{};
    }
  };

  static_assert(sizeof(TIDAssignmentPacket) == 12);

  struct __attribute__((packed)) CommandPacket {
    PacketHeader header;
    uint8_t targetId = 0;
    uint8_t command = 0;
    uint8_t p1 = 0;
    uint8_t p2 = 0;
    uint8_t p3 = 0;
    uint8_t p4 = 0;
    uint8_t p5 = 0;
    uint8_t p6 = 0;
    uint8_t p7 = 0;
    uint8_t p8 = 0;
    uint8_t p9 = 0;
    uint8_t p10 = 0;
    uint8_t p11 = 0;
    uint8_t p12 = 0;
    uint8_t p13 = 0;
    uint8_t p14 = 0;

    void reset(){
      *this = CommandPacket{};
    }
  };

  static_assert(sizeof(CommandPacket) == 19);
}
