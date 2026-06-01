#pragma once

#include <Arduino.h>

class CLIListener {
    public:
        CLIListener();
        void tick();
        bool messageWaiting();
        String retrieveMessage();
    private:
        String buffer;
        String message;
};