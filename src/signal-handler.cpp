#include <csignal>
#include <iostream>

void signalHandler(int sigNum);

void setSignalHandler(){
    signal(SIGINT, signalHandler);
    signal(SIGKILL, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
}

void signalHandler(int sigNum){
    switch(sigNum){
        case SIGINT: {
            std::cerr << "The process has been interrupted by the user" << std::endl; 
            break;
        }
        case SIGKILL: {
            std::cerr << "The process has been killed" << std::endl; 
            break;
        }
        case SIGTERM: {
            std::cerr << "The process has received a termination request" << std::endl; 
            break;
        }
        case SIGQUIT: {
            std::cerr << "The process has quit and produced a core dump" << std::endl; 
            break;
        }
    }
    exit(0);
}
