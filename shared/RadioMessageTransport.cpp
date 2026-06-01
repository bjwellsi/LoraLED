#pragma once

#define MAX_QUEUE_SIZE 32

#include "RadioMessageTransport.h"

RadioMessageTransport::RadioMessageTransport(){
    inboundQueueSize = 0;
    inboundLedgerSize = 0;
    outboundLedgerSize = 0;
}

void tick(){
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
    // if the message is an ack, check the outbound queue. if the outbound queue contains the sequence that's being acked, update its ack status to the status of the incoming ack. 

}

RadioDTO::Message* nextMessage(){
        //TODO
    //return a reference to the next queued message
    //remove it from the inbound queue and mark it as in progress
    return nullptr;
}

void markDone(RadioDTO::Message* message, RadioDTO::AckResponseCode responseCode){
        //TODO
    //this is going to update the message in the inbound ledger
    //and then send an ack if the message wanted one
}


RadioDTO::Message* sendMessage(RadioDTO::Message message, int resendCount, int timeout){
        //TODO
    //this needs to assign the sequenceid and return a reference to the packet in the outbout ledger
    //then just send the packet according to the params requested. the implmenetation of those params (resends etc) will be handled by a state machine owned by a different layer of abstraction
    //this returns a reference so that the calling operation can listen for acks if it wants to
    return nullptr;
}

