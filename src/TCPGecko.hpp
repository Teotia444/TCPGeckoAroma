#ifndef TCPGECKO_H_
#define TCPGECKO_H_

// Libutils (https://github.com/Maschell/libutils) implementation on tcp servers

#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils/logger.h"


#include <vector>
#include <string>

class TCPServer {
public:
    TCPServer(int port);
    virtual ~TCPServer();

    bool isConnected() {
        return connected;
    }
    
    int getClientFD() {
        return clientfd;
    }

    int getSocketFD() {
        return sockfd;
    }

protected:
    bool shouldExit() {
        return (exitThread == 1);
    }

    struct sockaddr_in getSockAddr() {
        return sock_addr;
    }
private:
    virtual void CloseSockets();

    static void DoTCPThread(std::stop_token stop_token, void *arg);
    virtual void DoTCPThreadInternal(std::stop_token stop_token);

    virtual void onConnectionClosed(){
        DEBUG_FUNCTION_LINE("Default onConnectionClosed \n");
    }

    struct sockaddr_in sock_addr;
    volatile int sockfd = -1;
    volatile int clientfd = -1;

    int port = 0;
    volatile bool connected = false;

    volatile int exitThread = 0;
    std::jthread pThread;
};

#endif