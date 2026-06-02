#pragma once

#define MAX_QUEUE_SIZE 32

#include "MessageTransport.h"

MessageTransport::MessageTransport(){
    inboundLedgerIndex = 0;
    outboundLedgerIndex = 0;
    inboundQueue;
    currentSequence = 0;
}

void MessageTransport::tick(){
    radioOps.tick();
    //TODO
    //lots going to happen here.
    //handles ingress primarily
    //what it does is check the radio for messages waiting
    //if there's one, check 2 things. 
    //  1. is the target actually us. if not, ignore it
    //      how to determine this is a bit of a todo. this layer needs some insight into target id ultimately unless you want to propogate all messages up to the parent layer. which would mean a much more bloated inbound ledger/higher likelihood of messages falling off
    //  2. have we already seen this message. ie check the inbound ledger for matching sequence id
    //      if we have not, simply add the message to the inbound queue and the inbound ledger. 
    //      if we have, check the ack status on that message
    //          if the ack status says the message is still in progress, just ignore the repeat request
    //          if the ack status says the message is complete, whether error or otherwise, send another ack if the message expects an ack
    // actually third thing to check. 
    // if the message is an ack, check the outbound ledger. if the outbound ledger contains the sequence that's being acked, update its ack status to the status of the incoming ack. 

}

RadioDTO::Message* MessageTransport::nextMessage(){
    //return a reference to the next queued message
    //remove it from the inbound queue and mark it as in progress
    if(inboundQueue.empty()){
       return nullptr; 
    }
    RadioDTO::Message* message = inboundQueue.front();
    inboundQueue.pop();
    message -> responseCode = ComDef::AckResponseCode::IN_PROGRESS;
    return message;
}

void MessageTransport::markDone(RadioDTO::Message* message, ComDef::AckResponseCode responseCode){
    //mark the message in the inbound queue and send an ack if necessary

    message -> responseCode = responseCode;
    if(message -> expectsAck){
        RadioDTO::Message ack;
        ack.messageType = RadioDTO::MessageType::ACK;
        ack.ack.responseCode = responseCode;
        ack.ack.sequenceID = message->sequenceID;

        sendMessage(ack, 1, 500);
    }
}


RadioDTO::Message* MessageTransport::sendMessage(RadioDTO::Message message, int resendCount, int timeout){
    //TODO need to check the outbound ledger for existing references by sequenceid so that this can handle resending messages

    //this needs to assign the sequenceid and return a reference to the packet in the outbound ledger
    //this returns a reference so that the calling operation can listen for acks if it wants to
    message.sequenceID = currentSequence++;
    outboundLedger[outboundLedgerIndex] = message;
    RadioDTO::Message* outboundMessage = &message;
    radioOps.sendMessage(outboundMessage, resendCount, timeout);
    return outboundMessage;
}

