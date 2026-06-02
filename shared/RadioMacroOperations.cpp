#include "RadioMacroOperations.h"
#include "RadioOperations.h"
#include "PacketMessageMapper.h"

void RadioMacroOperations::tick(){
    //crawl the op list trying to send each message in it
    for(int i = 0; i < messages.size(); i++){
        RadioOpContext messageContext = messages[i];
        if(messageContext.message -> responseCode == ComDef::AckResponseCode::SUCCESS 
            || messageContext.message -> responseCode == ComDef::AckResponseCode::ERROR
            || messageContext.timeoutEndtime > millis() 
            || messageContext.maxRetryCount > messageContext.currentRetryCount)
        {
            //remove this context so we stop trying to send this message
            messages.erase(messages.begin() + i);
            i--;
        }
        else {
            //resend the packet
            radioOperations.sendPacketBytes(messageContext.packet.data, messageContext.packet.length);
        }
    }
}

void RadioMacroOperations::cancelMessage(RadioDTO::Message* message){
    for(int i = 0; i < messages.size(); i++){
        if(messages[i].message == message){
            messages.erase(messages.begin() + i);
        }
    }
}

void RadioMacroOperations::sendMessage(RadioDTO::Message* message, int retryCount, int timeout){
    //add the message to the list of messages currently being sent by building the op context for it
    RadioOpContext messageContext;
    messageContext.message = message;
    messageContext.packet = PacketMessageMapper::generatePacket(*message);
    messageContext.maxRetryCount = retryCount;
    messageContext.timeoutEndtime = millis() + timeout;

    messages.push_back(messageContext);
}

bool RadioMacroOperations::messageWaiting(){
    return radioOperations.messageWaiting();
}

RadioDTO::Message RadioMacroOperations::retrieveMessage(){
    if(radioOperations.messageWaiting()){
        ComDef::PacketBytes incoming = radioOperations.checkForMessages();
        return PacketMessageMapper::retrieveMessage(incoming);
    }
    else return RadioDTO::Message{};
}