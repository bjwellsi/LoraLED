#include "TaskHandling.h"
#include "LedDriver.h"

extern CRGB* leds;
extern int totalLEDs;
extern volatile bool stopAnimation;

namespace AsyncOps{
  bool chargeAnimationBuilder(CRGB color, int speedMs){
    for(int i = 0; i < totalLEDs; i++){
      if(stopAnimation) return false;

      leds[i] = color;
      FastLED.show();
      if(TaskHandling::sleepInterruptible(speedMs)) return false;
    }
    return true;
  }

  bool flashAnimationBuilder(int count, int timeOff, int timeOn, CRGB color){
    for(int i = 0; i < count || count < 1; i++){
      //so the calling op can know to kill itself if exit was forced
      if(stopAnimation) return false;

      Serial.println("Flash off");
      LedDriver::setColor(CRGB::Black);
      if(TaskHandling::sleepInterruptible(timeOff)) return false;

      Serial.println("Flash on");
      LedDriver::setColor(color);
      if(TaskHandling::sleepInterruptible(timeOn)) return false;
    }
    Serial.println("Exited loop, flash off");
    LedDriver::setColor(CRGB::Black);
    //to tell the calling op that you weren't interrupted, you stopped on your own
    return true;
  }
}