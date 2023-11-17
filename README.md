# tcplike

Imlementation of a TCP-like protocol on top of UDP.

## Project requirements

- **CMake 3.20**
- **C++17**

## Filesent

An example of using TCP-like protocol implementation. Client sends connects to the server and receives the list of files in server's current directory


## How to Set Up and Run the Project

Clone the repository:
```
git clone https://github.com/reflmierv/tcplike
```

Navigate into the directory `2048`. Create the directory `build`:
```
cd tcplike 
mkdir build
```

Generate the build files in `build` directory:
```
cmake -S . -B build
```

Build the project:
```
cmake --build build
```

Run the server:
```
sudo build/filesent-server PORT
```

Run the client:
```
sudo build/filesent-client IP-ADDRESS PORT
```

