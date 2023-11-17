#include "tcplike.hpp"
#include "signal-handler.hpp"
#include <filesystem>
#include <iostream>
#include <cstring>


int main(int argc, char ** argv){
    setSignalHandler();
    
    uint8_t buffer[4096];
    TCPlikeServer server(7755);
    server.accept();
    std::size_t bytesLeft = sizeof(buffer) - 1;
    uint8_t * dataPtr = buffer;
    std::memset(buffer, 0, sizeof(buffer));
    for (const auto & entry : std::filesystem::directory_iterator(std::filesystem::current_path())){
        if(entry.is_regular_file()){
            std::string fileName = entry.path().filename().string() + '\n';
            std::size_t fileNameSize = fileName.size();
            if(fileNameSize <= bytesLeft){
                std::strcpy(reinterpret_cast<char *>(dataPtr), fileName.c_str());
                dataPtr += fileNameSize;
                bytesLeft -= fileNameSize;
            }

        }
    }
    server.send(buffer, sizeof(buffer));

    return 0;
}
