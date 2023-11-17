#include "tcplike.hpp"
#include <chrono>
#include <cstdint>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

int TCPlikePeer::send(const void * data, std::size_t len){
    if(!isConnectionEstablished){
        return -1;
    }

    uint32_t sequenceNumber = 0;

    const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(data);
    std::size_t payloadLen = TCPLIKE_PAYLOAD_MAXLENGTH;
    std::size_t bytesLeft = len;
    std::memcpy(sendPayloadPtr, dataPtr, payloadLen);

    int status;
    bool isFinished = false; 
    bool ackReceived = false;
    
    while(bytesLeft){
        for(int i = 0; i < TCPLIKE_RETRANSMIT_ATTEMPTS; ++i){
            const std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
            std::chrono::time_point<std::chrono::steady_clock> now;
            std::chrono::milliseconds duration;
            do {
                transmitDataSegment(connectionData.ipAddr, connectionData.port, sequenceNumber, sizeof(TCPlikeHeader) + payloadLen);
                status = receiveAck(connectionData.ipAddr, connectionData.port);
                if(status == 0){
                    isFinished = true;
                    i = TCPLIKE_RETRANSMIT_ATTEMPTS;
                    ackReceived = true;
                    break;
                }
                now = std::chrono::steady_clock::now();
                duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
            } while(duration <= TCPLIKE_TIMEOUT_DURATION && !isFinished);
            isFinished = false;
        }
        if(ackReceived){
            if(bytesLeft >= payloadLen){
                bytesLeft -= payloadLen;
                dataPtr += payloadLen;
                //payloadLen = (bytesLeft < TCPLIKE_PAYLOAD_MAXLENGTH) ? bytesLeft : TCPLIKE_PAYLOAD_MAXLENGTH;
                payloadLen = TCPLIKE_PAYLOAD_MAXLENGTH;
                sequenceNumber++;
                std::memset(sendPayloadPtr, 0, TCPLIKE_PAYLOAD_MAXLENGTH);
                std::memcpy(sendPayloadPtr, dataPtr, payloadLen);
                ackReceived = false;
            }
            else {
                break;
            }
        }
        else {
            transmitRst(connectionData.ipAddr, connectionData.port);
            return -2;
        }
    }
    


    return 0;
}
int TCPlikePeer::receive(void * data, std::size_t len){
    if(!isConnectionEstablished){
        return -1;
    }

    uint32_t sequenceNumber = 0;

    uint8_t * dataPtr = reinterpret_cast<uint8_t *>(data);
    std::size_t payloadLen = TCPLIKE_PAYLOAD_MAXLENGTH;
    std::size_t bytesLeft = len;

    int status;

    while(bytesLeft){
        status = receiveDataSegment(connectionData.ipAddr, connectionData.port, sequenceNumber, sizeof(TCPlikeHeader) + payloadLen);
        if(status == 0) {
            transmitAck(connectionData.ipAddr, connectionData.port);
            std::memcpy(dataPtr, receivePayloadPtr, payloadLen);
            if(bytesLeft >= payloadLen){
                bytesLeft -= payloadLen;
                dataPtr += payloadLen;
                payloadLen = TCPLIKE_PAYLOAD_MAXLENGTH;
                sequenceNumber++;
            }
        }
        else {
            transmitRst(connectionData.ipAddr, connectionData.port);
            return -2;
        }
    }

    return 0;
}

int TCPlikeClient::connect(uint32_t ipAddr, uint16_t port){
    if(isConnectionEstablished){
        return -1;
    }

    int status;
    bool isFinished = false; 
    bool synAckReceived = false;
    for(int i = 0; i < TCPLIKE_RETRANSMIT_ATTEMPTS; ++i){
        transmitSyn(ipAddr, port);
        //wait(isWaitFinished);
        const std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
        std::chrono::time_point<std::chrono::steady_clock> now;
        std::chrono::milliseconds duration;
        do {
            status = receiveSynAck(ipAddr, port);
            if(status == 0){
                isFinished = true;
                i = TCPLIKE_RETRANSMIT_ATTEMPTS;
                synAckReceived = true;
                break;
            }
            now = std::chrono::steady_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        } while(duration <= TCPLIKE_TIMEOUT_DURATION && !isFinished);
        isFinished = false;
        //} while(!isWaitFinished);
    }
    
    if(synAckReceived) {
        transmitAck(ipAddr, port);

        isConnectionEstablished = true;
        connectionData.ipAddr = ipAddr;
        connectionData.port = port;

        return 0;
    };

    return -2;
}

int TCPlikeServer::accept() {
    if(isConnectionEstablished){
        return -1;
    }
    int status;

    do {
        status = receiveSyn(connectionData.ipAddr, connectionData.port);
    } while(status < 0);

    bool ackReceived = false;
    for(int i = 0; i < TCPLIKE_RETRANSMIT_ATTEMPTS; ++i){
        transmitSynAck(connectionData.ipAddr, connectionData.port);
        status = receiveAck(connectionData.ipAddr, connectionData.port);
        if(status == 0){
            isConnectionEstablished = true;
            return 0;
            break;
        }
    }

    return -2;
}

int TCPlikePeer::transmitDataSegment(uint32_t ipAddr, uint16_t port, uint32_t sequenceNumber, std::size_t len){
    if(len <= sizeof(TCPlikeHeader) || len > TCPLIKE_SEGMENT_MAXLENGTH){
        return -1;
    }

    int status;

    sendHeaderPtr->type = htons(TCPlikeType::SND);
    sendHeaderPtr->sequenceNumber = htons(sequenceNumber);
    sendHeaderPtr->checksum = htons(calculateChecksum(sendHeaderPtr,len));
    do {
        status = transportLayer.send(ipAddr, port, sendHeaderPtr, len);
    } while(status < 0);

    return 0;
}
int TCPlikePeer::receiveDataSegment(uint32_t ipAddr, uint16_t port, uint32_t sequenceNumber, std::size_t len){
    if(len <= sizeof(TCPlikeHeader) || len > TCPLIKE_SEGMENT_MAXLENGTH){
        return -1;
    }

    uint32_t receiveIpAddr;
    uint16_t receivePort;

    uint16_t origChecksum;
    uint16_t calculatedChecksum;

    int status;

    bool isFinished = false;
    bool isValid = false;

    const std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    std::chrono::time_point<std::chrono::steady_clock> now;
    std::chrono::milliseconds duration;
    do{
        status = transportLayer.receive(receiveIpAddr, receivePort, receiveHeaderPtr, len);
        if(status > 0 && receiveHeaderPtr->sequenceNumber == htons(sequenceNumber) && ipAddr == receiveIpAddr && port && receivePort && receiveHeaderPtr->type == htons(TCPlikeType::SND)){
            isValid = true;
            isFinished = true;
        }
        now = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    } while(duration <= TCPLIKE_TIMEOUT_DURATION && !isFinished);
    isFinished = false;

    if(!isValid){
        return -2;
    }
    
    if(receiveHeaderPtr->sequenceNumber != htons(sequenceNumber)){
        return -4;
    }

    calculatedChecksum = calculateChecksum(receiveHeaderPtr, len, origChecksum);

    if(origChecksum != calculatedChecksum){
        return -3;
    }
    
    return 0;
}
int TCPlikePeer::transmitControlSegment(TCPlikeType type, uint32_t ipAddr, uint16_t port){
    if(type == TCPlikeType::SND || type > TCPlikeType::RST){
        return -1;
    }
    int status;

    sendHeaderPtr->type = htons(type);
    sendHeaderPtr->sequenceNumber = 0;
    sendHeaderPtr->checksum = htons(calculateChecksum(sendHeaderPtr, sizeof(TCPlikeHeader)));
    do {
        status = transportLayer.send(ipAddr, port, sendHeaderPtr, sizeof(TCPlikeHeader));
    } while(status < 0);

    return 0;
}

inline int TCPlikePeer::transmitSyn(uint32_t ipAddr, uint16_t port){
    return transmitControlSegment(TCPlikeType::SYN, ipAddr, port);
}
inline int TCPlikePeer::transmitSynAck(uint32_t ipAddr, uint16_t port){
    return transmitControlSegment(TCPlikeType::SYN_ACK, ipAddr, port);
}
inline int TCPlikePeer::transmitAck(uint32_t ipAddr, uint16_t port){
    return transmitControlSegment(TCPlikeType::ACK, ipAddr, port);
}
inline int TCPlikePeer::transmitRst(uint32_t ipAddr, uint16_t port){
    return transmitControlSegment(TCPlikeType::RST, ipAddr, port);
}
inline int TCPlikePeer::transmitFin(uint32_t ipAddr, uint16_t port){
    return transmitControlSegment(TCPlikeType::FIN, ipAddr, port);
}


int TCPlikePeer::receiveControlSegment(TCPlikeType type, uint32_t ipAddr, uint16_t port){
    if(type == TCPlikeType::SND || type == TCPlikeType::SYN || type > TCPlikeType::RST){
        return -1;
    }
    
    uint32_t receiveIpAddr;
    uint16_t receivePort;

    uint16_t origChecksum;
    uint16_t calculatedChecksum;

    int status;
    bool isFinished = false;
    bool isValid = false;
    //wait(isWaitFinished);
    const std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    std::chrono::time_point<std::chrono::steady_clock> now;
    std::chrono::milliseconds duration;
    do{
        status = transportLayer.receive(receiveIpAddr, receivePort, receiveHeaderPtr, sizeof(TCPlikeHeader));
        if(status > 0 && ipAddr == receiveIpAddr && port == receivePort && receiveHeaderPtr->type == htons(type)){
            isValid = true;
            isFinished = true;
            break;
        }
        now = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    } while(duration <= TCPLIKE_TIMEOUT_DURATION && !isFinished);

    if(!isValid){
        return -2;
    }

    calculatedChecksum = calculateChecksum(receiveHeaderPtr, sizeof(TCPlikeHeader), origChecksum);
    

    if(origChecksum != calculatedChecksum){
        return -3;
    }
    
    return 0;
}


int TCPlikePeer::receiveSyn(uint32_t & ipAddr, uint16_t & port){
    uint16_t origChecksum;
    uint16_t calculatedChecksum;

    int status;
    bool isFinished = false;
    bool isValid = false;
    const std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    std::chrono::time_point<std::chrono::steady_clock> now;
    std::chrono::milliseconds duration;
    do{
        status = transportLayer.receive(ipAddr, port, receiveHeaderPtr, sizeof(TCPlikeHeader));
        if(status > 0 && receiveHeaderPtr->type == htons(TCPlikeType::SYN)){
            isValid = true;
            isFinished = true;
        }
        now = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    } while(duration <= TCPLIKE_TIMEOUT_DURATION && !isFinished);
    isFinished = false;

    if(!isValid){
        return -2;
    }

    calculatedChecksum = calculateChecksum(receiveHeaderPtr, sizeof(TCPlikeHeader), origChecksum);

    if(origChecksum != calculatedChecksum){
        ipAddr = 0;
        port = 0;
        return -3;
    }
    
    return 0;
}
inline int TCPlikePeer::receiveSynAck(uint32_t ipAddr, uint16_t port){
    return receiveControlSegment(TCPlikeType::SYN_ACK, ipAddr, port);
}
inline int TCPlikePeer::receiveAck(uint32_t ipAddr, uint16_t port){
    return receiveControlSegment(TCPlikeType::ACK, ipAddr, port);
}
uint16_t TCPlikePeer::calculateChecksum(TCPlikeHeader * segmentPtr, std::size_t len, uint16_t & origChecksum){
    origChecksum = ntohs(segmentPtr->checksum);
    segmentPtr->checksum = 0;

    uint32_t sum = 0;
    std::size_t bytesLeft = len; 
    const uint16_t * ptr = reinterpret_cast<const uint16_t * >(segmentPtr);

    //sum all 16-bit values in data
    for(; bytesLeft > 1; ++ptr, bytesLeft -= 2){
        sum += *ptr; 
    }
    
    //add odd byte to the sum, if left
    if(bytesLeft == 1){
        sum += *reinterpret_cast<const uint8_t *>(ptr);
    }

    //while the sum is bigger than 16 bit, add 16 most significant bits and 16 least significant bits of the sum
    while(sum >> 16){
        sum = (sum & 0xffff) + (sum >> 16);
    }

    //perform one's complement of the sum
    uint16_t answer = ~(static_cast<uint16_t>(sum));

    segmentPtr->checksum = htons(origChecksum);
    return answer;
}
inline uint16_t TCPlikePeer::calculateChecksum(TCPlikeHeader * segmentPtr, std::size_t len){
    static uint16_t optVal;
    return calculateChecksum(segmentPtr, len, optVal);
};

