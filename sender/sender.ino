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
  //check radio
  RadioHandler::checkRadioBuffer(radioCallbacks);
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
    packet.command = STOP;
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
    packet.command = SOLID_COLOR;
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
    packet.command = FLASH;
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

void processAck(ComDef::Ack packet){
  //depends on current flow
  switch(activeOp){
    case INIT:
      initContext.ack = packet;
      initContext.ackQueued = true;
      break;
  }
}

void initializeReceivers(){
  //TODO
  //k so new flow. 
  //first we send out a report uid command. save and ack every rec we see
  //make sure to scan the list on each new rec so that recs that missed our ack don't get saved twice
  //then, once we've hit lets say 3 s with no new coms in the air, consider all recs reported
  //atp, loop the list assiging tids till ack 
  //
  //
  //
  //actually maybe even better idea.
  //what if the initial "report pls" command is a single op. 
  //just fire the command and forget. 
  //but then we process each uidreport packet by simply generating the tid on the spot if it doesn't exist, or saving if it does, and acking right away
  //that way, recs can just come online in whatever time frame they want
  //
  //only issue that introduces is concurrency problems with the radio handler. 
  //rn the radio handler is setup to only remember 1 state machine at a time. 
  //if you interrupt an active state machine to send your tid assignment bc a rec comes back online in the middle, you're gonna lose the whole op. 
  //this is solvable with a stack, but that is going to be annoying lol
}

void processUIDReportPacket(ComDef::UIDReportPacket){
  //TODO
  //this should only ever happen during the init flow
}

void initializeReceivers(){
  if(initContext.initState != STOP && initContext.initState != SENDING_STOP && initContext.initState != INITIALIZED && millis() + initContext.initTimeout < initContext.initBegin){
    //every loop needs to make this comparison so that no matter where we are in the process we can interrupt
    initContext.initState = STOP;
  }
  if(initContext.initState == STOP){
    //you're done, you've either hit your timeout or been commanded to stop
    //send a stop command to all radios. Again, chirp it three times
    currentSequence++;
    initContext.commandPacket = {.header = {.packetType = COMMAND, .sequence = currentSequence}
      .targetId = (uint8_t)0, .command = STOP
      }; 
    initContext.increment = 0;
    initContext.initState = SENDING_STOP;
  }
  else if(initContext.initState == SENDING_STOP){
    if(initContext.increment < 3){
      if (millis() - initContext.waitStart > random(20, 200)) {
        RadioHandler::sendPacket(initContext.commandPacket);
        initContext.increment++;
        initContext.waitStart = millis();
      }
    }
    else {
      initContext.increment = 0;
      initContext.initState = INITIALIZED;
      activeOp = IDLE;
    }
  }
  else if(initContext.initState == UNINITIALIZED){
    //begin fresh
    initContext.initTimeout = 10000;
    initContext.initBegin = millis();

    //reset receivers
    receiverCount = 0;
    for(int i = 0; i < MAX_RECEIVERS; i++){
      receivers[i] = 0;
    }
  
    initContext.initState = SENDING_INIT_COMMAND;
    initContext.increment = 0;
    currentSequence++;
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
        RadioHandler::sendPacket(initContext.commandPacket);
        initContext.increment++;
        initContext.waitStart = millis();
      }
    }
    else {
      initContext.increment = 0;
      initContext.initState = LISTENING_FOR_RECEIVERS;
    }
  }
  else if(initContext.initState == LISTENING_FOR_RECEIVERS && handshakeQueued){
    handshakeQueued = false;
    if(handshakePacket.sender == 0){
      return;
    }
    //a handshake has been initiated by a rec
    //get the next rec transid, build a packet with it
    //save the rec
    //send the packet
    uint64_t targetUID = initContext.handshakePacket.UID;
    initContext.currentReceiverTarget = targetUID;

    receiverCount++;
    receivers[receiverCount - 1] = {.TransientID = receiverCount, .UID = targetUID, .initialized = false};
    currentSequence++;
    ComDef::HandshakePacket tidSend = {.header = {.packetType = HANDSHAKE, .sequence = currentSequence}, .TransientId = receiverCount, .UID = targetUID, .sender = 1};
    initContext.handshakePacket = tidSend;
    initContext.initState = SENDING_TID;
    initContext.waitStart = 0;
    initContext.ackQueued = false;
  }
  else if(initContext.initState = SENDING_TID){
    if(initContext.ackQueued){
      initContext.ackQueued = false;
      //validate the id
      if(initContext.ack.header.sequence == initContext.handshakePacket.header.sequence && initContext.ack.status == SUCCESS){
        //this receiver is done
        //restart the loop to listen for other receivers. 
        receivers[receiverCount - 1].initialized = true;
        initContext.initState = LISTENING_FOR_RECEIVERS;
      }
    }
    else {
      if (millis() - initContext.waitStart > random(20, 200)) {
      //periodically send tid until you get an ack or you timeout
      RadioHandler::sendPacket(initContext.handshakePacket);
      initContext.waitStart = millis();
    }
  } 
}
