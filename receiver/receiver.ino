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
TaskHandle_t animationTaskHandle = nullptr;
volatile bool stopAnimation = false;
volatile bool animationExited = false;

struct FlashConfig{
  int count;
  int timeOn;
  int timeOff;
  CRGB color;
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Receiver booting");

  tubeCount = loadTubeCount();
  //TODO Actually init this, hardcoded for now
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
    startAnimation(&errAnimationTask, nullptr);
    while (true);
  }

  startAnimation(&bootAnimationTask, nullptr);

  // //await the boot animation for now
  // while(!animationExited){
  //   vTaskDelay(pdMS_TO_TICKS(500));
  //   Serial.println("Awaiting boot sequence");
  // }
  Serial.println("Receiver ready");
}

void loop() {
  byte payload[1];

  int state = radio.receive(payload, 1);
  if (state == RADIOLIB_ERR_NONE) {
    byte incoming = payload[0];

    Serial.print("Received: ");
    Serial.println(incoming);
    processCommand(incoming);
  }
  // else ignore (no packet / timeout)

}

void processCommand(byte command){

  //so here's the idea. we have 1 task which handles listening for commands. 
  //It loops through repeatedly, simply waiting to hear any new command. 
  //If it gets a command, it stops any current animations
  //then it starts the new one

  //process the command. Right now this is really simple, since we only have a few animations
  if (command == 0) setColorAnimationAtomic(CRGB::Black);
  else if (command == 8) {
    startAnimation(&errAnimationTask, nullptr);
  }
  else if (command == 1) setColorAnimationAtomic(CRGB::Red);
  else if (command == 2) setColorAnimationAtomic(CRGB::Green);
  else if (command == 3) setColorAnimationAtomic(CRGB::Blue);
  else if (command == 4) {
    FlashConfig* flashConf = new FlashConfig{
      .count = -1,
      .timeOn = 500,
      .timeOff = 500,
      .color = CRGB::Red
    };
    Serial.println("Starting flash task");
    startAnimation(&flashAnimationTask, flashConf);
  }
  else if (command == 5) {
    FlashConfig* flashConf = new FlashConfig{
      .count = -1,
      .timeOn = 500,
      .timeOff = 500,
      .color = CRGB::Green
    };
    Serial.println("Starting flash task");
    startAnimation(&flashAnimationTask, flashConf);
  }
  else if (command == 6) {
    FlashConfig* flashConf = new FlashConfig{
      .count = -1,
      .timeOn = 500,
      .timeOff = 500,
      .color = CRGB::Blue
    };
    Serial.println("Starting flash task");
    startAnimation(&flashAnimationTask, flashConf);
  }
}

void startAnimation(void (*animFn)(void*), void* config){
  //make sure no animations are currently running
  stopCurrentAnimation();

  //reset these so your new animation won't exit on its own
  stopAnimation = false;
  animationExited = false;

  Serial.println("Anim configured, previous anim stopped, flags reset. Starting new anim task.");
  xTaskCreatePinnedToCore(
    animFn,
    "Animation",
    4096,
    config,
    1,
    &animationTaskHandle,
    1
  );
}

void stopCurrentAnimation(){
  if (animationTaskHandle != nullptr){
    
    Serial.println("Attempting graceful stop.");
    //stop whatever animation is currently running
    stopAnimation = true;

    //give it a little time to stop
    vTaskDelay(pdMS_TO_TICKS(20));

    if(!animationExited){
      Serial.println("Task failed to stop. Forcing stop.");
      //if it doesn't stop, force kill. 
      vTaskDelete(animationTaskHandle);
    }

    animationTaskHandle = nullptr;
    animationExited = false;
    FastLED.clear(true);
  }
}

bool sleepInterruptible(uint32_t totalMs){
  //loop until you get force interrupted by someone else altering stopAnimation
  uint32_t start = millis();

  while (millis() - start < totalMs){
    if (stopAnimation) return true;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  return false;
}

void cleanupAnimationTask(){
  animationExited = true;
  vTaskDelete(nullptr);
}


void initializeReceiver(){
  //so now the init flow is this
  //boot. if you have a uid/tube count, load them.
  //if you don't load defaults. 
  //when you get a command to init, start blinking and transmitting your uid. 
  startAnimation(&idAssignmentAnimationTask, nullptr);
  //between transmissions, listen for a response containing your uid and a transid
  uint8_t uid = 0;
  saveTransientID(uid);
  setColorAnimationAtomic(CRGB::Black);

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

void errAnimationTask(void* parameter){
  flashAnimationBuilder(-1, 200, 200, CRGB::Red);
  cleanupAnimationTask();
}

void bootAnimationTask(void* parameter){
  if(!chargeAnimationBuilder(CRGB::Green, 30)){
    cleanupAnimationTask();
  }
  flashAnimationBuilder(1, 250, 400, CRGB::Green);
  cleanupAnimationTask();
}

void idAssignmentAnimationTask(void* parameter){
  flashAnimationBuilder(-1, 100, 1000, CRGB::Blue);
  cleanupAnimationTask();
}


void flashAnimationTask(void* parameter){

  FlashConfig* config = (FlashConfig*)parameter;

  int count = config->count;
  int timeOn = config->timeOn;
  int timeOff = config->timeOff;
  CRGB color = config->color;
  delete config;

  Serial.println("Config loaded");
  flashAnimationBuilder(count, timeOff, timeOn, color);

  Serial.println("Exited loop");
  cleanupAnimationTask();
}

bool chargeAnimationBuilder(CRGB color, int speedMs){

  for(int i = 0; i < totalLEDs; i++){
    if(stopAnimation) return false;

    leds[i] = color;
    FastLED.show();
    if(sleepInterruptible(speedMs)) return false;
  }
  return true;
}

bool flashAnimationBuilder(int count, int timeOff, int timeOn, CRGB color){
  for(int i = 0; i < count || count < 0; i++){
    //so the calling op can know to kill itself if exit was forced
    if(stopAnimation) return false;

    Serial.println("Flash off");
    setColor(CRGB::Black);
    if(sleepInterruptible(timeOff)) return false;

    Serial.println("Flash on");
    setColor(color);
    if(sleepInterruptible(timeOn)) return false;
  }
  Serial.println("Exited loop, flash off");
  setColor(CRGB::Black);
  //to tell the calling op that you weren't interrupted, you stopped on your own
  return true;
}

void setColorAnimationAtomic(CRGB color) {
  stopCurrentAnimation();
  setColor(color);
}

void setColor(CRGB color) {
  fill_solid(leds, totalLEDs, color);
  FastLED.show();
}