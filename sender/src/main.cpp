#include <Arduino.h>
#include <RadioLib.h>
#include "ComDef.h"
#include "SenderStateManagement.h"
#include "CLIListener.h"
#include "RadioDTO.h"
#include "MessageTransport.h"

const uint8_t MAX_RECEIVERS = 32;
uint8_t receiverCount; 
SenderStateManagement::Receiver receivers[MAX_RECEIVERS];
SenderStateManagement::AssignMissingTIDsContext assignMissingTIDsContext;
SenderStateManagement::ActiveOp activeOp;
CLIListener cli;
MessageTransport messageTransport;

uint64_t getUID();
void handleCliCommand(String line);
void handleMessage(RadioDTO::Message *incoming);
void printTIDs();
void assignMissingTIDs();
void assignMissingTIDsStateMachine();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Sender booting");

  activeOp = SenderStateManagement::IDLE;

  messageTransport.assignTID(1);
  messageTransport.assignUID(getUID());

  Serial.println("Sender ready");
}

void loop() {
  cli.tick();
  messageTransport.tick();
  if(cli.messageWaiting()){
    String cliMessage = cli.retrieveMessage();

    Serial.print("Handling command: ");
    Serial.println(cliMessage);

    handleCliCommand(cliMessage);
  }
  if(messageTransport.messageWaiting()){
    RadioDTO::Message *message = messageTransport.nextMessage();

    Serial.print("Handling message: ");
    Serial.print(message->sequenceID);
    Serial.print(" from ");
    Serial.println(message->senderId.idKind == ComDef::IDKind::TID ? message->senderId.TID : message->senderId.UID);
    handleMessage(message);
  }
  //run active state machine
  switch (activeOp){
    case SenderStateManagement::ASSIGNING_TIDS: 
      assignMissingTIDsStateMachine();
      break;
  }
}

void handleCliCommand(String line){
  if (line.startsWith("off ")){

    int targetId;

    int parsed = sscanf(line.c_str(), "off %d", &targetId);
    
    if (parsed != 1) {

      Serial.println("Bad off command");
      return;
    }

    RadioDTO::Message message;
    message.messageType = RadioDTO::MessageType::STOP_COMMAND;
    message.targetId.idKind = ComDef::IDKind::TID;
    message.targetId.TID = targetId;

    messageTransport.sendMessage(message, 1, 1000);
  }
  else if (line.startsWith("solid ")){
    int targetId, r, g, b;

    int parsed = sscanf(line.c_str(), "solid %d %d %d %d", &targetId, &r, &g, &b);
    
    if (parsed != 4) {
      Serial.println("Bad color command");
      return;
    }

    RadioDTO::Message message;
    message.messageType = RadioDTO::MessageType::SOLID_COLOR;
    message.targetId.idKind = ComDef::IDKind::TID;
    message.targetId.TID = targetId;

    message.solidColor.color = ((uint8_t)r, (uint8_t)g, (uint8_t)b);

    messageTransport.sendMessage(message, 1, 1000);
  }
  else if (line.startsWith("flash ")){
    int targetId, r, g, b, count, timeOff, timeOn;

    int parsed = sscanf(line.c_str(), "flash %d %d %d %d %d %d %d", &targetId, &r, &g, &b, &count, &timeOff, &timeOn);

    if (parsed != 7) {
      Serial.println("Bad flash command");
      return;
    }

    RadioDTO::Message message;
    message.messageType = RadioDTO::MessageType::FLASH_COLOR;
    message.targetId.idKind = ComDef::IDKind::TID;
    message.targetId.TID = targetId;

    message.flashColor.color = ((uint8_t)r, (uint8_t)g, (uint8_t)b);
    message.flashColor.color = (uint8_t)count;
    message.flashColor.offTime = timeOff;
    message.flashColor.onTime = timeOn;

    messageTransport.sendMessage(message, 1, 1000);
  }
  else if (line.startsWith("init")){
    RadioDTO::Message message;
    message.messageType = RadioDTO::MessageType::UID_CHIRP_COMMAND;
    message.targetId.idKind = ComDef::IDKind::TID;
    message.targetId.TID = 0;

    messageTransport.sendMessage(message, 1, 1000);
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

void handleMessage(RadioDTO::Message *incoming){
  //sender side
  if(incoming->messageType == RadioDTO::MessageType::UID_REPORT){
    //check if we already knew about this uid, save it if not
    if(incoming->senderId.idKind != ComDef::IDKind::UID){
      //bad, no uid in report
      messageTransport.markDone(incoming, ComDef::AckResponseCode::ERROR);
      return;
    }

    uint64_t incomingUID = incoming->senderId.UID; 
    for(int i = 0; i < receiverCount; i++){
      if(receivers[i].UID == incomingUID){
        //already exists
        messageTransport.markDone(incoming, ComDef::AckResponseCode::SUCCESS);
        return;
      }
    }
    //if you got here, rec doesn't exist yet. check that we're not full
    if(receiverCount < MAX_RECEIVERS){
      //insert
      SenderStateManagement::Receiver newRec;
      newRec.initialized = false;
      //for now the tid is just the index + 1 (room for sender to be 1)
      newRec.TransientID = receiverCount + 2;
      newRec.UID = incomingUID;
      receivers[receiverCount] = newRec;
      receiverCount++;
      messageTransport.markDone(incoming, ComDef::AckResponseCode::SUCCESS);
    }
    else {
      //hit max rec cnt
      messageTransport.markDone(incoming, ComDef::AckResponseCode::ERROR);
    }
  }
  else{
    //if you get in a command of any sort, discard it
    //really all we care about are uid reports right now. just ack the rest if they want an ack
    messageTransport.markDone(incoming, ComDef::AckResponseCode::SUCCESS);
  }
}

void printTIDs(){
  Serial.println("List of TIDs");
  for(int i = 0; i < receiverCount; i++){
    Serial.println(receivers[i].TransientID);
  }
  Serial.println("Transmitting TID flash command");
    RadioDTO::Message message;
    message.messageType = RadioDTO::MessageType::TID_REPORT;
    message.targetId.idKind = ComDef::IDKind::TID;
    message.targetId.TID = 0;

    messageTransport.sendMessage(message, 1, 1000);
}

void assignMissingTIDs(){
  activeOp = SenderStateManagement::ASSIGNING_TIDS; 
  assignMissingTIDsContext.currentReceiverIndex = 0;
}

void assignMissingTIDsStateMachine(){
  if(assignMissingTIDsContext.currentReceiverTimeoutEndTime > millis()){
    assignMissingTIDsContext.awaitingAck == nullptr;
    assignMissingTIDsContext.currentReceiverIndex++;
    return;
  }
  else if(assignMissingTIDsContext.awaitingAck == nullptr){
    for(int i = assignMissingTIDsContext.currentReceiverIndex; i < receiverCount; i++){
      //get the next rec
      if(!receivers[i].initialized){
        SenderStateManagement::Receiver nextRec = receivers[i];
        RadioDTO::Message assignment;
        assignment.tidAssign.newTID = nextRec.TransientID;
        assignment.targetId.idKind = ComDef::IDKind::UID;
        assignment.targetId.UID = nextRec.UID;

        assignment.expectsAck = true;

        RadioDTO::Message *sent = messageTransport.sendMessage(assignment, -1, assignMissingTIDsContext.maxRecWait);

        assignMissingTIDsContext.awaitingAck = sent;
        assignMissingTIDsContext.currentReceiverTimeoutEndTime = millis() + assignMissingTIDsContext.maxRecWait;
        
        return;
      }
    }
    //if you make it out of the for loop, all recs are initialized
    assignMissingTIDsContext.reset();
    activeOp = SenderStateManagement::IDLE;
  }
  else{
    //check if the message has acked
    RadioDTO::Message *current = assignMissingTIDsContext.awaitingAck;
    if(current->responseCode == ComDef::AckResponseCode::SUCCESS){
      receivers[assignMissingTIDsContext.currentReceiverIndex].initialized = true;
      assignMissingTIDsContext.awaitingAck == nullptr;
      assignMissingTIDsContext.currentReceiverIndex++;
    }
    else if(current->responseCode == ComDef::AckResponseCode::ERROR
      || current->responseCode == ComDef::AckResponseCode::CANCELED
      || current->responseCode == ComDef::AckResponseCode::TIMEOUT)
    {
      assignMissingTIDsContext.awaitingAck == nullptr;
      assignMissingTIDsContext.currentReceiverIndex++;
    }
    //otherwise keep waiting on this ack
  }
}

uint64_t getUID(){
  return ESP.getEfuseMac();
}