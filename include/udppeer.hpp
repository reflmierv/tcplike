#ifndef UDPLAYER_H
#define UDPLAYER_H

#include <asm-generic/socket.h>
#include <cstddef>
#include <arpa/inet.h>
#include <cstdint>
#include <random>
#include <netinet/ip.h>
#include <netinet/udp.h>

const unsigned short RAW_PACKET_MAXLENGTH = 32767;
const unsigned short UDP_PAYLOAD_MAXLENGTH = RAW_PACKET_MAXLENGTH - sizeof(udphdr);

class UDPPeer {
    typedef uint32_t Socket;
public:
    UDPPeer();
    UDPPeer(uint16_t port);
    ~UDPPeer();
    int send(uint32_t ipAddr, uint16_t port, const void * data, std::size_t len);
    int receive(uint32_t & ipAddr, uint16_t & port, void * data, std::size_t len);
private:
    Socket sock;
    uint16_t port;
    sockaddr_in socketAddr;
    std::random_device randomDevice;
    uint8_t dataBuffer[UDP_PAYLOAD_MAXLENGTH];
};

#endif
