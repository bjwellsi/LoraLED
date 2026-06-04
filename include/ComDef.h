#pragma once

#include <stdint.h>

#define MAX_PACKET_SIZE 64

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

  enum AckResponseCode : uint8_t {
    NO_RESPONSE = 0,
    INVALID = 1, 
    NOT_STARTED = 2, 
    IN_PROGRESS = 3,
    SUCCESS = 4,
    ERROR = 5
  };

  enum IDKind : uint8_t {
    TID = 0,
    UID = 0
  };

  template <typename SenderIdT, typename ReceiverIdT>
  struct __attribute__((packed)) PacketHeader {
    uint8_t packetType = INVALID;
    uint16_t sequence = 0;
    uint8_t expectsAck = 0;
    uint8_t idKind = 0;
    SenderIdT senderId;
    ReceiverIdT receiverId;

    void reset(){
      *this = PacketHeader{};
    }
  };
  
  template <typename SenderIdT, typename ReceiverIdT>
  struct __attribute__((packed)) Ack {
    PacketHeader<SenderIdT, ReceiverIdT> header;
    uint8_t status = 0;

    void reset(){
      *this = Ack{};
    }
  };

  template <typename SenderIdT, typename ReceiverIdT>
  struct __attribute__((packed)) UIDReportPacket {
    PacketHeader<SenderIdT, ReceiverIdT> header;
    uint8_t target = 0;
    uint64_t UID = 0;

    void reset(){
      *this = UIDReportPacket{};
    }
  };

  template <typename SenderIdT, typename ReceiverIdT>
  struct __attribute__((packed)) TIDAssignmentPacket {
    PacketHeader<SenderIdT, ReceiverIdT> header;
    uint64_t UID = 0;
    uint8_t TransientID = 0;

    void reset(){
      *this = TIDAssignmentPacket{};
    }
  };

  template <typename SenderIdT, typename ReceiverIdT>
  struct __attribute__((packed)) CommandPacket {
    PacketHeader<SenderIdT, ReceiverIdT> header;
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


  struct PacketBytes{
    uint8_t* data; 
    uint8_t length = 0;
  };
}
