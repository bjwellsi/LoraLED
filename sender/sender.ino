#include <RadioLib.h>
#include "../ComDef.h"
#include "../RadioHandler.h"
#include "SenderStateManagement.h"

volatile bool radioReceivedFlag = true;
SX1262 radio;
const RadioHandler::RadioCallBacks radioCallbacks = {.onHandshake = processHandshake, .onCommand = nullptr, .onAck = processAck};

const uint8_t MAX_RECEIVERS = 32;
uint8_t receiverCount; 
uint8_t receivers[MAX_RECEIVERS];

uint16_t currentSequence;

SenderStateManagement::ActiveOp activeOp;
SenderStateManagement::InitContext initContext;

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
  switch (activeOp){
    case INIT: 
      initailizeReceivers();
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
  ComDef::CommandPacket packet = {.header = {.packetType = COMMAND, .sequence = currentSequence}}; 
  currentSequence++;

  if (line.startsWith("off ")){
    int targetId;

    int parsed = sscanf(line.c_str(), "off %d", &targetId);
    
    if (parsed != 1) {
      Serial.println("Bad off command");
      return;
    }

    packet.targetId = (uint8_t)targetId;
    packet.command = (uint8_t)0;
  }
  else if (line.startsWith("solid ")){
    int targetId, r, g, b;

    int parsed = sscanf(line.c_str(), "solid %d %d %d %d", &targetId, &r, &g, &b);
    
    Serial.println(parsed);
    if (parsed != 4) {
      Serial.println("Bad color command");
      return;
    }

    packet.targetId = (uint8_t)targetId;
    packet.command = (uint8_t)20;
    packet.p1 = (uint8_t)r;
    packet.p2 = (uint8_t)g;
    packet.p3 = (uint8_t)b;
  }
  else if (line.startsWith("flash ")){
    int targetId, r, g, b, count, timeOff, timeOn;

    int parsed = sscanf(line.c_str(), "flash %d %d %d %d %d %d %d", &targetId, &r, &g, &b, &count, &timeOff, &timeOn);

    if (parsed != 7) {
      Serial.println("Bad flash command");
      return;
    }

    packet.targetId = (uint8_t)targetId;
    packet.command = (uint8_t)21;
    packet.p1 = (uint8_t)r;
    packet.p2 = (uint8_t)g;
    packet.p3 = (uint8_t)b;
    packet.p4 = (uint8_t)count;
    packet.p5 = (uint8_t)(((uint16_t)timeOff >> 8) & 0xFF);
    packet.p6 = (uint8_t)(((uint16_t)timeOff) & 0xFF);
    packet.p7 = (uint8_t)(((uint16_t)timeOn >> 8) & 0xFF);
    packet.p8 = (uint8_t)(((uint16_t)timeOn) & 0xFF);
  }
  else if (line.starts.wih("init")){
    initailizeReceivers();
  }
  else {
    Serial.println("Unknown command");
    return;
  }

  RadioHandler::sendPacket(packet);
}

void initializeReceivers(){
  //TODO check the timeout. 
  //every loop needs to make this comparison so that no matter where we are in the process we can interrupt
  if(initContext.initState == STOP){
    //you're done, you've either hit your timeout or been commanded to stop
    //TODO
    //if there's a current rec being saved, remove it
    //send a stop command to all radios. Again, chirp it three times
    initContet.initState = INITIALIZED;
    activeOp = IDLE;
  }
  else if(initContext.initState == UNINITIALIZED){
    //begin fresh

    //reset receivers
    receiverCount = 0;
    for(int i = 0; i < MAX_RECEIVERS; i++){
      receivers[i] = 0;
    }
  
    initContext.initState = SENDING_INIT_COMMAND;
    initContet.increment = 0;
    initContext.commandPacket = {.header = {.packetType = COMMAND, .sequence = currentSequence}
      .targetId = (uint8_t)0, .command = 1
      }; 
    initContext.currentSequence++;
    initContext.waitStart = millis();
  }
  else if(initContext.initState == SENDING_INIT_COMMAND){
    //send out an init command to all radios
    //could retry this over and over, but that's needlessly complex
    //instead just gonna chirp it 3 times and call that good
    if(initContext.increment < 3){
      if (millis() - initContext.waitStart > random(20, 200)) {
        RadioHandler::sendPacket(packet);
        initContext.increment++;
        initContext.waitStart = millis();
      }
    }
    else {
      initContext.increment = 0;
      initContext.initState = LISTENING_FOR_RECEIVERS;
    }
  }
  else if(initContext.initState == LISTENING_FOR_RECEIVERS && initContext.handshakeQueued){
    //TODO
    //a handshake has been initiated by a rec
    //get the next rec transid, build a packet with it
    //save the rec
    initContext.initState = SENDING_TID;
  }
  else if(initContext.initState = SENDING_TID){
    //TODO
    //periodically send tid until you get an ack or you timeout
    //if no ack comes in and you hit your timeout remove the rec
    //restart the loop to listen for other 
    initContext.initState = LISTENING_FOR_RECEIVERS;
  } 
}



void initializeReceivers(){
  
  //start listening for responses
  //once you recieve a command to stop from the user or you hit max entries, break
  uint32_t startTime = millis();
  uint32_t timeout = 1000000;
  bool stop = false;
  while(millis() - startTime < timeout && stop == false){
    if(checkCli() == "stop"){
      //cli command comes in to stop
      break;
    }
    else {
      ComDef::Handshake handshakeResp;
      int state = radio.receive((uint8_t*)&handshakeResp, sizeof(handshakeResp));
      if(state == RADIOLIB__ERR_NONE && handshakeResp.Sender = 0){
        //handshake initiated by reciever
        uint64_t uidResponse = handshakeResp.TransientId;
        //on each response increment r count, add a new entry to r
        receiverCount++;
        receivers[receiverCount - 1] = recieverCount;
        uint32_t handshakeStart = millis();
        uint32_t handshakeTimeout = 10000;
        bool receiverInitialized = false;
        ComDef::Handshake handshakeAssign = {.Sender = 1, .UID = uidResponse, .TransientId = receiverCount}
        while(millis() - handshakeStart < handshakeTimeout && stop == false){
          //tell the receiver what its new id is 
          //response will need to include uidresponse
          //on a random interval, send the response handshake, listen for ok from the receiver
          if(checkCli() == "stop"){
            stop = true;
            break;
          }
          else {
            ComDef::Handshake handshakeConfirm;
            int state = radio.receive((uint8_t*)&handshakeConfirm, sizeof(handshakeConfirm));
            if(state == RADIOLIB__ERR_NONE && handshakeConfirm.Sender = 0){
              //response received
              //make sure the values match
              if(handshakeComfirm.UID = handshakeAssign.UID && handshakeConfirm.TransientId = handshakeAssign.TransientId){
                receiverInitialized = true;
                break;
              }
              //otherwise just restart the loop and try again
            }
            else{
              //send or resend the uid
              sendhandshake(handshakeAssign);
              //wait random amount of time 
              delay(random(50, 300));
            }
          }
        }
        if(!receiverInitialized){
          //handshake timed out, remove the receiver
          receivers[receiverCount - 1] = 0;
          receiverCount --;
        }
      }
    }
    //otherwise restart the loop
  }
  //init done, tell the receivers. Whether they got an id or not
  sendCommand({.command = 0, .targetId = 0})
  
}
