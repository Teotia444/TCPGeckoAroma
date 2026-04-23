#ifndef TCPGECKO_H_
#define TCPGECKO_H_

#include "utils/CThread.h"
#include <vector>
#include <string>

void Start(CThread* thread, void* arg);

struct clientDetails{
      int32_t clientfd;  // client file descriptor
      int32_t serverfd;  // server file descriptor
      std::vector<int> clientList; // for storing all the client fd
      clientDetails(void){ // initializing the variable
            this->clientfd=-1;
            this->serverfd=-1;
      }
};
#endif