#include <Arduino.h>
#include <RadioLib.h>
#include "ComDef.h"
#include "RadioHandler.h"
#include "SenderStateManagement.h"
#include "CLIListener.h"
#include "RadioDTO.h"
#include "MessageTransport.h"

volatile bool radioReceivedFlag = true;
SX1262 radio = nullptr;
RadioHandler::RadioCallbacks radioCallbacks;
RadioHandler::RadioOpContext radioOpContext;
RadioHandler::RadioState radioState;

const uint8_t MAX_RECEIVERS = 32;
uint8_t receiverCount; 
SenderStateManagement::Receiver receivers[MAX_RECEIVERS];
SenderStateManagement::AssignMissingTIDsContext assignMissingTIDsContext;
SenderStateManagement::ActiveOp activeOp;
CLIListener cli;
MessageTransport messageTransport;

uint16_t currentSequence;

void handleCliCommand(String cli);
void assignMissingTIDs();
void assignMissingTIDsStateMachine();
void assignMissingTIDsCallback(RadioHandler::ResponseCode responseCode);
String checkCli();
void processUIDReportPacket(ComDef::UIDReportPacket uidPacket);
void printTIDs();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Sender booting");
  radioCallbacks.onUIDReport = processUIDReportPacket;

  RadioHandler::initRadio();
  activeOp = SenderStateManagement::IDLE;
  currentSequence = 0;

  Serial.println("Sender ready");
}

void loop() {
  cli.tick();
  if(cli.messageWaiting()){
    String cliMessage = cli.retrieveMessage();

    Serial.print("Handling command: ");
    Serial.println(cliMessage);

    handleCliCommand(cliMessage);
  }
  //check radio
  RadioHandler::tickRadio();
  switch (activeOp){
    case SenderStateManagement::ASSIGNING_TIDS: 
      assignMissingTIDsStateMachine();
      break;
  }
}

void handleCliCommand(String line){
  if (line.startsWith("off ")){
    RadioDTO::Message message;
    message.stop = {};

    int targetId;

    int parsed = sscanf(line.c_str(), "off %d", &targetId);
    
    if (parsed != 1) {

      Serial.println("Bad off command");
      return;
    }


    message.stop.target = targetId;

    messageTransport.sendMessage(message, -1, 1000);
  }
  else if (line.startsWith("solid ")){
    ComDef::CommandPacket packet;

    packet.header.packetType = ComDef::COMMAND;
    packet.header.sequence = RadioHandler::nextSequence(); 

    int targetId, r, g, b;

    int parsed = sscanf(line.c_str(), "solid %d %d %d %d", &targetId, &r, &g, &b);
    
    if (parsed != 4) {
      Serial.println("Bad color command");
      return;
    }

    packet.targetId = (uint8_t)targetId;
    packet.command = ComDef::SOLID_COLOR;
    packet.p1 = (uint8_t)r;
    packet.p2 = (uint8_t)g;
    packet.p3 = (uint8_t)b;

    RadioHandler::sendPacket(packet);
  }
  else if (line.startsWith("flash ")){
    ComDef::CommandPacket packet;

    packet.header.packetType = ComDef::COMMAND;
    packet.header.sequence = RadioHandler::nextSequence(); 

    int targetId, r, g, b, count, timeOff, timeOn;

    int parsed = sscanf(line.c_str(), "flash %d %d %d %d %d %d %d", &targetId, &r, &g, &b, &count, &timeOff, &timeOn);

    if (parsed != 7) {
      Serial.println("Bad flash command");
      return;
    }

    packet.targetId = (uint8_t)targetId;
    packet.command = ComDef::FLASH;
    packet.p1 = (uint8_t)r;
    packet.p2 = (uint8_t)g;
    packet.p3 = (uint8_t)b;
    packet.p4 = (uint8_t)count;
    packet.p5 = (uint8_t)(((uint16_t)timeOff >> 8) & 0xFF);
    packet.p6 = (uint8_t)(((uint16_t)timeOff) & 0xFF);
    packet.p7 = (uint8_t)(((uint16_t)timeOn >> 8) & 0xFF);
    packet.p8 = (uint8_t)(((uint16_t)timeOn) & 0xFF);

    RadioHandler::sendPacket(packet);
  }
  else if (line.startsWith("init")){
    ComDef::CommandPacket packet;

    packet.header.packetType = ComDef::COMMAND;
    packet.header.sequence = RadioHandler::nextSequence(); 

    packet.targetId = (uint8_t)0;
    packet.command = ComDef::TRANSMIT_UID;

    RadioHandler::sendPacket(packet);
  }
  else if (line.startsWith("tidAssign")){
    assignMissingTIDs();
  }
  else if (line.startsWith("tidPrint")){
    printTIDs();
  }
  else {
    Serial.println("Unknown command");
    return;
  }
}

void assignMissingTIDs(){
  activeOp = SenderStateManagement::ASSIGNING_TIDS; 
  assignMissingTIDsContext.currentReceiverIndex = 0;
}

void printTIDs(){
  Serial.println("List of TIDs");
  for(int i = 0; i < receiverCount; i++){
    Serial.println(receivers[i].TransientID);
  }
  Serial.println("Transmitting TID flash command");
  ComDef::CommandPacket packet;

  packet.header.packetType = ComDef::COMMAND;
  packet.header.sequence = RadioHandler::nextSequence(); 

  packet.targetId = (uint8_t)0;
  packet.command = ComDef::FLASH_TID;

  RadioHandler::sendPacket(packet);
}

void assignMissingTIDsCallback(RadioHandler::ResponseCode responseCode){
  assignMissingTIDsContext.awaitingAck = false;
  if(responseCode == RadioHandler::ACK_RECEIVED){
    receivers[assignMissingTIDsContext.currentReceiverIndex].initialized = true;
  }
}

void assignMissingTIDsStateMachine(){
  if(assignMissingTIDsContext.awaitingAck == false){
    for(int i = assignMissingTIDsContext.currentReceiverIndex; i < receiverCount; i++){
      SenderStateManagement::Receiver& receiver = receivers[receiverCount];
      if(receiver.UID > 0 && receiver.initialized == false){
        assignMissingTIDsContext.currentReceiverIndex = i;
        assignMissingTIDsContext.awaitingAck = true;
        
        ComDef::TIDAssignmentPacket assignmentPacket;
        assignmentPacket.header.packetType = ComDef::TID_ASSIGN;
        assignmentPacket.header.sequence = RadioHandler::nextSequence(); 
        assignmentPacket.UID = receiver.UID;
        assignmentPacket.TransientID = (uint8_t)(i + 1);
        
        RadioHandler::sendTillAck(assignmentPacket, -1, 10000, assignMissingTIDsCallback);
      }
    return;
    }
    //if you make it out of the for loop, all recs are initialized
    assignMissingTIDsContext.reset();
    activeOp = SenderStateManagement::IDLE;
  }
}

void processUIDReportPacket(ComDef::UIDReportPacket uidPacket){
  uint16_t sequence = uidPacket.header.sequence;
  int indexOfRec = -1;
  for(int i = 0; i < receiverCount; i++){
    if(receivers[i].UID == uidPacket.UID){
      indexOfRec = i;
      break;
    }
  }
  if(indexOfRec < 0){
    receiverCount++;
    SenderStateManagement::Receiver newRec;
    newRec.TransientID = receiverCount;
    newRec.UID = uidPacket.UID;
    newRec.initialized = false;
   
    receivers[receiverCount - 1] = newRec;
  }
  RadioHandler::sendAck(ComDef::SUCCESS, sequence);
}
