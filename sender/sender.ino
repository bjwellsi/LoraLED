#include <RadioLib.h>
#include "ComDef.h"

// Heltec WiFi LoRa 32 V3 SX1262 pins:
// Module(NSS, DIO1, RESET, BUSY)
SX1262 radio = new Module(8, 14, 12, 13);

#define LORA_FREQ 915.0

const uint8_t MAX_RECEIVERS  = 32;
uint8_t receiverCount; 
uint8_t receivers[MAX_RECEIVERS];

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Sender booting");

  int state = radio.begin(LORA_FREQ);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Radio init failed, code: ");
    Serial.println(state);
    while (true);
  }

  Serial.println("Sender ready");
}

void loop() {
  String cli = checkCli();
  if (cli != "") {
    cli.trim();

    //TODO make this a new task. 
    //That way cli can always listen/ have universal interupts
    handleCliCommand(cli);

    Serial.print("Handled command: ");
    Serial.println(cli);
  }
}

String checkCli() {
  if (Serial.available() > 0){
    return Serial.readStringUntil('\n');
  }
  return "";
}

void handleCliCommand(String line){
  ComDef::CommandPacket packet{}; 

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

  sendCommand(packet);
}

void sendCommand(ComDef::CommandPacket packet) {
  int state = radio.transmit((uint8_t*)&packet, sizeof(packet));

  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("Sent packet");
  } else {
    Serial.print("Send failed, code: ");
    Serial.println(state);
  }
}

void sendHandshake(ComDef::Handshake handshake) {
  int state = radio.transmit((uint8_t*)handshake, sizeof(handshake));

  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("Sent handshake");
  } else {
    Serial.print("Send failed, code: ");
    Serial.println(state);
  }
}

void initializeReceivers(){
  //reset receivers
  receiverCount = 0;
  for(int i = 0; i < MAX_RECEIVERS; i++){
    receivers[i] = 0;
  }

  //send out an init command to all radios
  sendCommand({
      .targetId = (uint8_t)0,
      .command = (uint8_t)1
      });
  
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