#include "udppeer.hpp"
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <cstring>

UDPPeer::UDPPeer(uint16_t port){
    this->sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    this->port = port;
    this->socketAddr.sin_family = AF_INET;
}

UDPPeer::UDPPeer(){
    this->sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    int status;
    std::mt19937 gen(randomDevice());
    std::uniform_int_distribution<uint16_t> dis(49152, 65535); //ephemereal port range
    this->port = dis(gen);
    this->socketAddr.sin_family = AF_INET;
}

UDPPeer::~UDPPeer(){
    close(this->sock);
}

int UDPPeer::send(uint32_t ipAddr, uint16_t port, const void * data, std::size_t len){
    if(len > UDP_PAYLOAD_MAXLENGTH){
        return -1;
    }
    udphdr * udpHeader;
    udpHeader = reinterpret_cast<udphdr * >(dataBuffer);
    udpHeader->source = htons(this->port);
    udpHeader->dest = htons(port);
    udpHeader->len = htons(sizeof(udphdr) + len);
    udpHeader->check = 0;
    std::memcpy(dataBuffer + sizeof(udphdr), data, len);

    this->socketAddr.sin_addr.s_addr = htonl(ipAddr);

    int n = sendto(this->sock, dataBuffer, sizeof(udphdr) + len, 0, reinterpret_cast<sockaddr *>(&this->socketAddr), sizeof(socketAddr));
    return n;
}

int UDPPeer::receive(uint32_t & ipAddr, uint16_t & port, void * data, std::size_t len){
    if(len > UDP_PAYLOAD_MAXLENGTH){
        return -1;
    }
    udphdr * udpHeader;
    iphdr * ipHeader;
    ipHeader = reinterpret_cast<iphdr *>(dataBuffer);
    udpHeader = reinterpret_cast<udphdr *>(dataBuffer + sizeof(iphdr));
    socklen_t socketLen = sizeof(socketAddr);
    int n = recvfrom(this->sock, dataBuffer, sizeof(iphdr) + sizeof(udphdr) + len, 0, reinterpret_cast<sockaddr *>(&this->socketAddr), &socketLen);
    ipAddr = ntohl(this->socketAddr.sin_addr.s_addr);
    port = ntohs(udpHeader->source);
    if(ntohs(udpHeader->dest) == this->port){
        std::memcpy(data, dataBuffer + sizeof(iphdr) + sizeof(udphdr), len); 
        return n;
    }
    return -2;
}
