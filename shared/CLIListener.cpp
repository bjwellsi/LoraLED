#include "CLIListener.h"

CLIListener::CLIListener(){
    buffer = "";
    message = "";
}

void CLIListener::tick(){
    //current behavior is most recent message gets sent. 
    //so if I type 
    //  start \n stop \n green \n, then only "green" gets processed
    while(Serial.available() > 0){
        char c = Serial.read();

        if(c == '\r'){
            continue;
        }
        else if(c == '\n'){
            String line = buffer;
            line.trim();
            buffer = "";
        }
        else {
            buffer += c;
        }
    }
}

bool CLIListener::messageWaiting(){
    return !message.isEmpty();
}

String CLIListener::retrieveMessage(){
    String ret = message;
    message = "";
    return ret;
}