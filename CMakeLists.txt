cmake_minimum_required(VERSION 3.20.0)
project(tcplike CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

set(sources src/udppeer.cpp src/tcplike.cpp src/signal-handler.cpp)

add_executable(filesend-client src/filesend-client.cpp ${sources})
add_executable(filesend-server src/filesend-server.cpp ${sources})
