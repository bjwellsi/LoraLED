#include <RadioLib.h>
#include "ComDef.h"

// Heltec WiFi LoRa 32 V3 SX1262 pins:
// Module(NSS, DIO1, RESET, BUSY)
SX1262 radio = new Module(8, 14, 12, 13);

#define LORA_FREQ 915.0

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
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    handleCliCommand(line);

    Serial.print("Handled command: ");
    Serial.println(line);
  }
}

void handleCliCommand(String line){
  ComDef::Packet packet{}; 

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

    int parsed = sscanf(line.c_str(), "solid %d, %d, %d, %d", &targetId, &r, &g, &b);
    
    if (parsed != 4) {
      Serial.println("Bad color command");
      return;
    }

    packet.targetId = (uint8_t)targetId;
    packet.command = (uint8_t)1;
    packet.p1 = (uint8_t)r;
    packet.p2 = (uint8_t)g;
    packet.p3 = (uint8_t)b;
  }
  else if (line.startsWith("flash ")){
    int targetId, r, g, b, count, timeOff, timeOn;

    int parsed = sscanf(line.c_str(), "flash %d, %d, %d, %d, %d, %d, %d", &targetId, &r, &g, &b, &count, &timeOff, &timeOn);

    if (parsed != 7) {
      Serial.println("Bad flash command");
      return;
    }

    packet.targetId = (uint8_t)targetId;
    packet.command = (uint8_t)2;
    packet.p1 = (uint8_t)r;
    packet.p2 = (uint8_t)g;
    packet.p3 = (uint8_t)b;
    packet.p4 = (uint8_t)count;
    packet.p5 = (uint8_t)(((uint16_t)timeOff >> 8) & 0xFF);
    packet.p6 = (uint8_t)(((uint16_t)timeOff) & 0xFF);
    packet.p7 = (uint8_t)(((uint16_t)timeOn >> 8) & 0xFF);
    packet.p8 = (uint8_t)(((uint16_t)timeOn) & 0xFF);
  }
  else {
    Serial.println("Unknown command");
    return;
  }

  sendCommand(packet);
}

void sendCommand(ComDef::Packet packet) {
  int state = radio.transmit((uint8_t*)&packet, sizeof(packet));

  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("Sent packet");
  } else {
    Serial.print("Send failed, code: ");
    Serial.println(state);
  }
}