#pragma once

#include <Arduino.h>

namespace TaskHandling {
  void startAnimation(void (*animFn)(void*), void* config);

  void stopCurrentAnimation();

  bool sleepInterruptible(uint32_t totalMs);

  void cleanupAnimationTask();
}