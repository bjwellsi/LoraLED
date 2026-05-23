#pragma once

#include <FastLED.h>

namespace AnimationTasks{
  struct FlashConfig{
    int count;
    int timeOn;
    int timeOff;
    CRGB color;
  };
  
  void errAnimationTask(void* parameter);
  
  void bootAnimationTask(void* parameter);
  
  void idAssignmentAnimationTask(void* parameter);
  
  void flashAnimationTask(void* parameter);
}