#include "TaskHandling.h"
#include <FastLED.h>

extern TaskHandle_t animationTaskHandle;
extern volatile bool stopAnimation;
extern volatile bool animationExited;

namespace TaskHandling {

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
}