#include <FastLED.h>
#include <RadioLib.h>

// SX1262 pins for Heltec V3
// Module(NSS, DIO1, RESET, BUSY)
SX1262 radio = new Module(8, 14, 12, 13);

#define DATA_PIN  5
#define NUM_LEDS  14
#define LED_TYPE  WS2811
#define COLOR_ORDER BRG
#define LORA_FREQ 915.0

CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Receiver booting");

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(150);
  FastLED.clear(true);

  int state = radio.begin(LORA_FREQ);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Radio init failed, code: ");
    Serial.println(state);
    errAnimation();
    while (true);
  }

  bootAnimation(); // startup OK indicator
  Serial.println("Receiver ready");
}

void loop() {
  byte payload[1];

  int state = radio.receive(payload, 1);

  if (state == RADIOLIB_ERR_NONE) {
    byte incoming = payload[0];

    Serial.print("Received: ");
    Serial.println(incoming);

    if (incoming == 1) setColor(CRGB::Red);
    else if (incoming == 2) setColor(CRGB::Green);
    else if (incoming == 3) setColor(CRGB::Blue);
    else if (incoming == 0) setColor(CRGB::Black);
  }
  // else ignore (no packet / timeout)
}

void errAnimation(){
  flashAnimation(-1, 200, 200, CRGB::Red);
}

void bootAnimation(){
  unsigned long lastTime = 0;

  chargeAnimation(CRGB::Green, 30);
  flashAnimation(1, 250, 400, CRGB::Green);
}

void chargeAnimation(CRGB color, int speedMs){
  unsigned long lastTime = 0;
  int delayMs = 35;
  int i = 0;
  int colorIndex = 0;
  while(i < NUM_LEDS){
    if(millis() - lastTime > speedMs){
      leds[i] = color;
      FastLED.show();

      i++;
      lastTime = millis();
    }
  }
}

void flashAnimation(int count, int timeOff, int timeOn, CRGB color){
  unsigned long lastTime = 0;
  int i = 0;
  setColor(CRGB::Black);
  lastTime = millis();
  bool colorOn = false;
  while(true){
    if(i >= count && count >= 0){
      //count < 0 means flash forever
      break;
    }
    if(colorOn && millis() - lastTime > timeOn){
      setColor(CRGB::Black);
      colorOn = false;
      i++;
      lastTime = millis();
    }
    else if(!colorOn && millis() - lastTime > timeOff) {
      setColor(color);
      colorOn = true;
      lastTime = millis();
    }
  }
}

void setColor(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
}