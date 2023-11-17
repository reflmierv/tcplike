#include "tcplike.hpp"
#include "signal-handler.hpp"
#include <iostream>

int main(int argc, char ** argv){
    setSignalHandler();

    TCPlikeClient client;

    uint8_t receiveBuffer[8000];
    if(client.connect(0x7f000001, 7755) == 0){
        client.receive(receiveBuffer, sizeof(receiveBuffer));
        std::cout << receiveBuffer << std::endl;
    }
    return 0;
}
