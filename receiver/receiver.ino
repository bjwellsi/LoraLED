#include <FastLED.h>
#include <RadioLib.h>
#include <Preferences.h>
#include <Arduino.h>

// SX1262 pins for Heltec V3
// Module(NSS, DIO1, RESET, BUSY)
SX1262 radio = new Module(8, 14, 12, 13);

Preferences prefs;

#define DATA_PIN  5
#define LEDS_PER_TUBE 14
#define LED_TYPE  WS2811
#define COLOR_ORDER BRG
#define LORA_FREQ 915.0

CRGB* leds = nullptr;
uint8_t tubeCount;
uint8_t TransientID;
int totalLEDs;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Receiver booting");

  tubeCount = loadTubeCount();
  tubeCount = 4;
  totalLEDs = tubeCount * LEDS_PER_TUBE;
  
  Serial.print("Loaded ");
  Serial.print(totalLEDs);
  Serial.print(" LEDS across ");
  Serial.print(tubeCount);
  Serial.println(" tubes");
  

  leds = new CRGB[totalLEDs];
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, totalLEDs);
  FastLED.setBrightness(100);
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


void initializeReceiver(){
//so now the init flow is this
//boot. if you have a uid/tube count, load them.
//if you don't load defaults. 
//when you get a command to init, start blinking and transmitting your uid. 
idAssignmentAnimation();
//between transmissions, listen for a response containing your uid and a transid
uint8_t uid = 0;
saveTransientID(uid);

//now you can just listen like normal. 
//the next step will likely be a tube count transmission signal, but this will be completely initialized via the sender.
//what it'll do is assign you a blinking color, once the end user ids you by that color, it'll ask for tube count, they input, then it transmits.
//but basically all that is purely sender side logic.
}
uint64_t getUID(){
  return ESP.getEfuseMac();
}

void clearMemory(){
  saveTubeCount(0);
  saveTransientID(0);
}

void saveTransientID(uint8_t id){
  prefs.begin("config", false);
  prefs.putUChar("id", id);
  prefs.end();
}

uint8_t loadTransientID(){
  prefs.begin("config", true);
  uint8_t id = prefs.getUChar("id", 0);
  prefs.end();
  return id;
}

void saveTubeCount(uint8_t count){
  prefs.begin("config", false);
  prefs.putUChar("tubes", count);
  prefs.end();
}

uint8_t loadTubeCount(){
  prefs.begin("config", true);
  uint8_t count = prefs.getUChar("tubes", 1);
  prefs.end();
  return count;
}

void errAnimation(){
  flashAnimation(-1, 200, 200, CRGB::Red);
}

void bootAnimation(){
  unsigned long lastTime = 0;

  chargeAnimation(CRGB::Green, 30);
  flashAnimation(1, 250, 400, CRGB::Green);
}

void idAssignmentAnimation(){
  flashAnimation(-1, 100, 1000, CRGB::Blue);
}

void chargeAnimation(CRGB color, int speedMs){
  unsigned long lastTime = 0;
  int delayMs = 35;
  int i = 0;
  int colorIndex = 0;
  while(i < totalLEDs){
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
  fill_solid(leds, totalLEDs, color);
  FastLED.show();
}