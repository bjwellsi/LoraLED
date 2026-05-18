#include "LedDriver.h"

extern CRGB* leds;
extern int totalLEDs;

namespace LedDriver {
  void setColor(CRGB color){
    fill_solid(leds, totalLEDs, color);
    FastLED.show();
  }
}