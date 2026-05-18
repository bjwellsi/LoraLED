#include "AtomicOps.h"
#include "LedDriver.h"
#include "TaskHandling.h"

namespace AtomicOps {
  void setColorAnimation(CRGB color) {
    TaskHandling::stopCurrentAnimation();
    LedDriver::setColor(color);
  }
}