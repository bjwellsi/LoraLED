#include <RadioLib.h>

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
    char c = Serial.read();

    if (c == 'r') sendCommand(1); // red
    else if (c == 'g') sendCommand(2); // green
    else if (c == 'b') sendCommand(3); // blue
    else if (c == 'o') sendCommand(0); // off
    else {
      Serial.print("Unknown command: ");
      Serial.println(c);
      return;
    }

    Serial.print("Handled command: ");
    Serial.println(c);
  }
}

void sendCommand(byte cmd) {
  byte payload[1] = { cmd };

  int state = radio.transmit(payload, 1);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("Sent: ");
    Serial.println(cmd);
  } else {
    Serial.print("Send failed, code: ");
    Serial.println(state);
  }
}