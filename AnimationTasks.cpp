#include "AnimationTasks.h"
#include "AsyncOps.h"
#include "TaskHandling.h"

namespace AnimationTasks{
  void errAnimationTask(void* parameter){
    AsyncOps::flashAnimationBuilder(0, 200, 200, CRGB::Red);
    TaskHandling::cleanupAnimationTask();
  }

  void bootAnimationTask(void* parameter){
    if(!AsyncOps::chargeAnimationBuilder(CRGB::Green, 30)){
      TaskHandling::cleanupAnimationTask();
    }
    AsyncOps::flashAnimationBuilder(1, 250, 400, CRGB::Green);
    TaskHandling::cleanupAnimationTask();
  }

  void idAssignmentAnimationTask(void* parameter){
    AsyncOps::flashAnimationBuilder(0, 100, 1000, CRGB::Blue);
    TaskHandling::cleanupAnimationTask();
  }

  void flashAnimationTask(void* parameter){

    FlashConfig* config = (FlashConfig*)parameter;

    int count = config->count;
    int timeOn = config->timeOn;
    int timeOff = config->timeOff;
    CRGB color = config->color;
    delete config;

    Serial.println("Config loaded");
    AsyncOps::flashAnimationBuilder(count, timeOff, timeOn, color);

    Serial.println("Exited loop");
    TaskHandling::cleanupAnimationTask();
  }
}