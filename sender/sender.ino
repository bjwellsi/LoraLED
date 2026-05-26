#include <RadioLib.h>
#include "../ComDef.h"
#include "../RadioHandler.h"
#include "SenderStateManagement.h"

volatile bool radioReceivedFlag = true;
SX1262 radio;
const RadioHandler::RadioCallBacks radioCallbacks = {.TIDAssignmentPacket = nullptr, .onCommand = nullptr, .onUIDReport = processUIDReportPacket};

const uint8_t MAX_RECEIVERS = 32;
uint8_t receiverCount; 
SenderStateManagement::Receiver receivers[MAX_RECEIVERS];

uint16_t currentSequence;

SenderStateManagement::ActiveOp activeOp;
SenderStateManagement::AssignMissingTIDsContexgt assignMissingTIDsContext;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Sender booting");

  initContext.initState = UNINITIALIZED;
  initContext.handshakeQueued = false;

  RadioHandler::initRadio();
  ActiveOp = IDLE;
  currentSequence = 0;

  Serial.println("Sender ready");
}

void loop() {
  String cli = checkCli();
  if (cli != "") {
    cli.trim();

    handleCliCommand(cli);

    Serial.print("Handled command: ");
    Serial.println(cli);
  }
  //check radio
  RadioHandler::checkRadioBuffer(radioCallbacks);
  switch (activeOp){
    case ASSIGNING_TIDS: 
      assignMissingTIDsStateMachine();
      break;
  }
}

String checkCli() {
  if (Serial.available() > 0){
    return Serial.readStringUntil('\n');
  }
  return "";
}

void handleCliCommand(String line){
  if (line.startsWith("off ")){
    ComDef::CommandPacket packet = {.header = {.packetType = COMMAND, .sequence = currentSequence}}; 
    currentSequence++;

    int targetId;

    int parsed = sscanf(line.c_str(), "off %d", &targetId);
    
    if (parsed != 1) {

      Serial.println("Bad off command");
      return;
    }

    packet.targetId = (uint8_t)targetId;
    packet.command = STOP;

    RadioHandler::sendPacket(packet);
  }
  else if (line.startsWith("solid ")){
    ComDef::CommandPacket packet = {.header = {.packetType = COMMAND, .sequence = currentSequence}}; 
    currentSequence++;

    int targetId, r, g, b;

    int parsed = sscanf(line.c_str(), "solid %d %d %d %d", &targetId, &r, &g, &b);
    
    Serial.println(parsed);
    if (parsed != 4) {
      Serial.println("Bad color command");
      return;
    }

    packet.targetId = (uint8_t)targetId;
    packet.command = SOLID_COLOR;
    packet.p1 = (uint8_t)r;
    packet.p2 = (uint8_t)g;
    packet.p3 = (uint8_t)b;

    RadioHandler::sendPacket(packet);
  }
  else if (line.startsWith("flash ")){
    ComDef::CommandPacket packet = {.header = {.packetType = COMMAND, .sequence = currentSequence}}; 
    currentSequence++;

    int targetId, r, g, b, count, timeOff, timeOn;

    int parsed = sscanf(line.c_str(), "flash %d %d %d %d %d %d %d", &targetId, &r, &g, &b, &count, &timeOff, &timeOn);

    if (parsed != 7) {
      Serial.println("Bad flash command");
      return;
    }

    packet.targetId = (uint8_t)targetId;
    packet.command = FLASH;
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
    ComDef::CommandPacket packet = {.header = {.packetType = COMMAND, .sequence = currentSequence}}; 
    currentSequence++;

    packet.targetId = (uint8_t)0;
    packet.command = TRANSMIT_UID;

    RadioHandler::sendPacket(packet);
  }
  else if (line.startsWith("tidAssign")){
    assignMissingTIDs();
  }
  else {
    Serial.println("Unknown command");
    return;
  }
}

void assignMissingTIDs(){
  activeOp = ASSIGNING_TIDS; 
  assignMissingTIDsContext.currentReceiverIndex = 0;
}

void assignMissingTIDsStateMachine(){
  if(receiverCount assignMissingTIDsContext.awaitingAck == false){
    for(int i = assignMissingTIDsContext.currentReceiverIndex; i < receiverCount; i++){
      SenderStateManagement::Receiver receiver = receivers[receiverCount];
      if(receiver != nullptr && receiver.initialized == false){
        assignMissingTIDsContext.currentReceiverIndex = i;
        assignMissingTIDsContext.awaitingAck = true;
        ComDef::TIDAssignmentPacket assignmentPacket = {
          .header = {
            .sequence = ++currentSequence,
            .packetType = TID_ASSIGN,
          },
          .UID = receiver.UID,
          .TransientID = TransientID
        }
        RadioHandler::sendTillAck(assignmentPacket, -1, 10000, assignMissingTIDsCallback);
      }
    return;
    }
    //if you make it out of the for loop, all recs are initialized
    assignMissingTIDsContext.reset();
    activeOp = IDLE;
  }
}

void assignMissingTIDsCallback(RadioHandler::ResponseCode responseCode){
  assignMissingTIDsContext.awaitingAck = false;
  if(responseCode == ACK_RECEIVED){
    receivers[assignMissingTIDsContext.currentReceiverIndex].initialized = true;
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
    receivers[receiverCount - 1] = {.TransientID = receiverCount, .UID = targetUID, .initialized = false};
  }
  RadioHandler::SendAck(SUCCESS, sequence);
}
