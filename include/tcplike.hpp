#ifndef TCPLIKE_H
#define TCPLIKE_H

#include "udppeer.hpp"
#include <cstdint>
#include <chrono>

enum TCPlikeType {
    SND = 0,
    SYN = 1,
    SYN_ACK = 2,
    ACK = 3,
    FIN = 4,
    RST = 5
};

struct TCPlikeHeader {
    uint32_t sequenceNumber = 0;
    uint16_t checksum = 0;
    uint16_t type;
};
const unsigned short TCPLIKE_SEGMENT_MAXLENGTH = 2048;
const unsigned short TCPLIKE_PAYLOAD_MAXLENGTH = TCPLIKE_SEGMENT_MAXLENGTH - sizeof(TCPlikeHeader);
const unsigned char TCPLIKE_TIMEOUT = 4;
const std::chrono::milliseconds TCPLIKE_TIMEOUT_DURATION = std::chrono::milliseconds(4000);
const unsigned char TCPLIKE_RETRANSMIT_ATTEMPTS = 3;

class TCPlikePeer {
public:
    TCPlikePeer() = default;
    TCPlikePeer(uint16_t port) : transportLayer(port){};
    int send(const void * data, std::size_t len);
    int receive(void * data, std::size_t len);
protected:
    struct {
        uint32_t ipAddr;
        uint16_t port;
    } connectionData;
    uint16_t calculateChecksum(TCPlikeHeader * segmentPtr, std::size_t len, uint16_t & previousChecksum);
    uint16_t calculateChecksum(TCPlikeHeader * segmentPtr, std::size_t len);
    int transmitDataSegment(uint32_t ipAddr, uint16_t port, uint32_t sequenceNumber, std::size_t len = TCPLIKE_SEGMENT_MAXLENGTH);
    int receiveDataSegment(uint32_t ipAddr, uint16_t port, uint32_t sequenceNumber, std::size_t len = TCPLIKE_SEGMENT_MAXLENGTH);

    int transmitSyn(uint32_t ipAddr, uint16_t port);
    int transmitSynAck(uint32_t ipAddr, uint16_t port);
    int transmitAck(uint32_t ipAddr, uint16_t port);
    int transmitRst(uint32_t ipAddr, uint16_t port);
    int transmitFin(uint32_t ipAddr, uint16_t port);
    int transmitSnd(uint32_t ipAddr, uint16_t port);
    int receiveSyn(uint32_t & ipAddr, uint16_t & port);
    int receiveSynAck(uint32_t ipAddr, uint16_t port);
    int receiveAck(uint32_t ipAddr, uint16_t port);
    int receiveSnd(uint32_t ipAddr, uint16_t port);

    bool isConnectionEstablished = false;

    UDPPeer transportLayer;
    uint8_t sendBuffer[TCPLIKE_SEGMENT_MAXLENGTH];
    uint8_t receiveBuffer[TCPLIKE_SEGMENT_MAXLENGTH];
    TCPlikeHeader * const sendHeaderPtr = reinterpret_cast<TCPlikeHeader *>(sendBuffer);
    void * const sendPayloadPtr = reinterpret_cast<void * const>(sendBuffer + sizeof(TCPlikeHeader));
    TCPlikeHeader * const receiveHeaderPtr = reinterpret_cast<TCPlikeHeader *>(receiveBuffer);
    void * const receivePayloadPtr = reinterpret_cast<void * const>(receiveBuffer + sizeof(TCPlikeHeader));

private:
    int transmitControlSegment(TCPlikeType type, uint32_t ipAddr, uint16_t port);
    int receiveControlSegment(TCPlikeType type, uint32_t ipAddr, uint16_t port);
};

class TCPlikeServer : public TCPlikePeer {
public:
    using TCPlikePeer::TCPlikePeer;
    int accept();
};

class TCPlikeClient : public TCPlikePeer {
public:
    using TCPlikePeer::TCPlikePeer;
    int connect(uint32_t ipAddr, uint16_t port);
};

#endif
