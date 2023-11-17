#include "tcplike.hpp"
#include "signal-handler.hpp"
#include <iostream>

int main(int argc, char ** argv){
    if(argc != 3){
        std::cout << "Usage: " << argv[0] << " IP-ADDRESS PORT" << std::endl;
        return -1;
    }
    setSignalHandler();
    uint32_t ipAddr = ntohl(inet_addr(argv[1]));
    uint16_t port = std::stoi(argv[2]);

    TCPlikeClient client;

    uint8_t receiveBuffer[8000];
    if(client.connect(ipAddr, port) == 0){
        client.receive(receiveBuffer, sizeof(receiveBuffer));
        std::cout << receiveBuffer << std::endl;
    }
    return 0;
}
