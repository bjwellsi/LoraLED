#pragma once

#define MAX_QUEUE_SIZE 32

#include "MessageTransport.h"

MessageTransport::MessageTransport(){
    inboundQueue;
    inboundLedger;
    outboundLedger;
    currentSequence = 0;
}

void MessageTransport::tick(){
    radioOps.tick();
    //handle ingress
    //what it does is check the radio for messages waiting
    if(radioOps.messageWaiting()){
        RadioDTO::Message nextMessage = radioOps.retrieveMessage();
        if(!nextMessage.targetId.idMatches(TID, UID)){
            //is the target actually us. if not, ignore it
            return;
        }
        else if(nextMessage.messageType == RadioDTO::MessageType::ACK){
            //if the message is an ack, check the outbound ledger. 
            //if the outbound ledger contains the sequence that's being acked, update its ack status to the status of the incoming ack. 
            RadioDTO::MessageKey key = nextMessage.ack.getAckingForKey();

            auto it = outboundLedger.find(key);

            if(it != outboundLedger.end()) {
                RadioDTO::Message& found = it->second;

                found.responseCode = nextMessage.ack.responseCode;
            }
            //store the ack in the inbound ledger for good measure, but don't queue it
            inboundLedger[nextMessage.getMessageKey()] = nextMessage;
        }
        else{
            //check if we have already seen this message
            RadioDTO::MessageKey key = nextMessage.getMessageKey();

            auto it = inboundLedger.find(key);

            if(it != inboundLedger.end()) {
                RadioDTO::Message& found = it->second;
                found.responseCode = nextMessage.ack.responseCode;
                //check the ack status on the existing message
                //if the ack status says the message is still in progress, just ignore the repeat request
                //if the ack status says the message is complete, whether error or otherwise, send another ack if the message expects an ack
                if(found.expectsAck && (found.responseCode == ComDef::AckResponseCode::SUCCESS || found.responseCode == ComDef::AckResponseCode::ERROR)){
                    //you should have already sent an ack. find it
                    if(!(found.sentAck == nullptr)){
                        radioOps.sendMessage(found.sentAck, 1, 500);
                    }
                    else{
                        //no ack present on a message that was marked done. should not happen. but it did. make a new ack
                        //technically this is method overwrites the responsecode but I'm overwriting it with itself so sue me
                        markDone(&found, found.responseCode);
                    }
                }
            }
            else{
                //new message, add it to the ledger and queue
                inboundLedger[key] = nextMessage;
                inboundQueue.push(&nextMessage);
            }
            
        }
    }
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
        ack.sequenceID = message->sequenceID;
        
        message->sentAck = &ack;
        sendMessage(ack, 1, 500);
    }
}


RadioDTO::Message* MessageTransport::sendMessage(RadioDTO::Message message, int resendCount, int timeout){
    //TODO need to check the outbound ledger for existing references by sequenceid so that this can handle resending messages

    //this needs to assign the sequenceid and return a reference to the packet in the outbound ledger
    //this returns a reference so that the calling operation can listen for acks if it wants to
    message.sequenceID = currentSequence++;
    outboundLedger[message.getMessageKey()] = message;
    RadioDTO::Message* outboundMessage = &message;
    radioOps.sendMessage(outboundMessage, resendCount, timeout);
    return outboundMessage;
}


void MessageTransport::assignUID(uint64_t uid){
    UID = uid;
}

void MessageTransport::assignTID(uint8_t tid){
    TID = tid;
}

