#pragma once

#include <FastLED.h>

namespace AsyncOps{
  bool chargeAnimationBuilder(CRGB color, int speedMs);
  
  bool flashAnimationBuilder(int count, int timeOff, int timeOn, CRGB color);
}