#include "utils/logger.h"
#include "main.h"
#include "TCPGecko.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <coreinit/cache.h>
#include <gx2/event.h>

#include <vector>
#include <string>
#include <algorithm>
#include <stdint.h>
#include <thread>

#include <notifications/notifications.h>
#include <utils/CThread.h>

//addresses vector for the find command
std::vector<uint32_t> potentialAddresses;
NotificationModuleHandle currentText;

//Splits the string into a vector
std::vector<std::string> Split(const std::string& s, const std::string& delimiter) {
    std::string str = s;
    size_t pos = 0;
    
    //if the string terminates with a
    if((pos = str.find("\n")) != std::string::npos){
        str = str.substr(0, str.length() - 2);
    }
    std::vector<std::string> tokens;
    
    std::string token;
    while ((pos = str.find(delimiter)) != std::string::npos) {
        token = str.substr(0, pos);
        tokens.push_back(token);
        
        str.erase(0, pos + delimiter.length());
    }
    tokens.push_back(str);

    return tokens;
}

#pragma region Peek/Poke

uint32_t Peek(uint32_t addr) {
      //casting the address to a pointer and immediatly dereferences it
      uint32_t val = *((uint32_t *)(uintptr_t)addr);
      return val;
}

void Poke(uint32_t addr, uint32_t val){
      //dereference cast of address as pointer then apply value
      *((uint32_t *)(uintptr_t)addr) = val;
}

/*
All of the next peek/poke functions are pretty useless but like why not 
*/

uint16_t Peek16(uint32_t addr) {
      //Peek address
      uint32_t val = Peek(addr);
      //bitshift 
      uint16_t result = (val >> 16);
      return result;
}

void Poke16(uint32_t addr, uint16_t val){
      *((uint16_t *)(uintptr_t)addr) = val;
}

uint8_t Peek8(uint32_t addr) {
      //Peek address
      uint32_t val = Peek(addr);
      //bitshift 
      uint8_t result = (val >> 24);
      return result;
}

void Poke8(uint32_t addr, uint8_t val){
      *((uint8_t *)(uintptr_t)addr) = val;
}

float PeekF32(uint32_t addr) {
      //Peek address
      float val = *((float *)(uintptr_t)addr);
      return val;
}

void PokeF32(uint32_t addr, float val){
      *((float *)(uintptr_t)addr) = val;
}

#pragma endregion PeekPoke

void Call(uint32_t addr){
      ((void(*)(void))addr)();
}

#pragma region Find Functions
void StartFindValue32(uint32_t value){
      potentialAddresses.clear();
      for (uint32_t i = 0x10000000; i < 0x19000000; i++)
      {
            if(*((uint32_t *)(uintptr_t)i) == value){
                  potentialAddresses.push_back(i);
            }
      }
}
void ContinueFindValue32(uint32_t value){
      std::vector<uint32_t> newSet;
      for (uint32_t i = 0; i < potentialAddresses.size(); i++)
      {
            if(*((uint32_t *)(uintptr_t)potentialAddresses[i]) == value){
                  newSet.push_back(potentialAddresses[i]);
            }
      }
      potentialAddresses = newSet;
}
#pragma endregion Find Functions

//Handles a raw request
int Commands(TCPServer* socket, std::stop_token stop_token){
      char buffer[1024];
      while(!stop_token.stop_requested()){
            if(socket->getClientFD() == -1) return 0;

            if(read(socket->getClientFD(), buffer, 1024) <= 0){
                  GX2WaitForVsync();
                  return 0;
            }

            std::string command = (std::string)buffer;
            DEBUG_FUNCTION_LINE_INFO("Processing command from client %d: %s", socket->getClientFD(), command.c_str());
            std::vector<std::string> args = Split(command, " ");
            std::string instruction = args[0];

            transform(instruction.begin(), instruction.end(), instruction.begin(),::tolower);

            if(instruction == "peek"){
                  std::string address = "";
                  std::string type = "";

                  //gets important informations from the request
                  for (uint i = 0; i < args.size(); i++)
                  {
                        if(args[i] == "-t"){
                              type = args[i+1];
                        }
                        if(args[i] == "-a"){
                              address = args[i+1];
                        }
                  }

                  //checks for the different arguments
                  if(address == ""){
                        const char *message = "Instruction is missing address (-a)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }
                  if(type == ""){
                        const char *message = "Instruction is missing type (-t)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }
                  if(strtol(address.c_str(), NULL, 0) == 0){
                        const char *message = "Invalid address (-a)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }

                  //this can all be simplified to a bitshift depending on the type but we have those functions ready anyway 
                  std::string val = "";
                  if(type=="u8"){
                        val = std::to_string(Peek8((uint32_t)strtol(address.c_str(), NULL, 0)));
                  }
                  else if(type=="u16"){
                        val = std::to_string(Peek16((uint32_t)strtol(address.c_str(), NULL, 0)));
                  }
                  else if(type=="u32"){
                        val = std::to_string(Peek((uint32_t)strtol(address.c_str(), NULL, 0)));
                  }
                  else if(type=="f32"){
                        val = std::to_string(PeekF32((uint32_t)strtol(address.c_str(), NULL, 0)));
                  }
                  else {
                        const char *message = "Invalid type (-u)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }

                  //finally, send back our stuff to the client
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), val.c_str());
                  write(socket->getClientFD(), val.c_str(), strlen(val.c_str()));
                  
            }

            else if (instruction == "poke"){
                  std::string address = "";
                  std::string type = "";
                  std::string value = "";

                  //gets important informations from the request
                  for (uint i = 0; i < args.size(); i++)
                  {
                        if(args[i] == "-t"){
                              type = args[i+1];
                        }
                        if(args[i] == "-a"){
                              address = args[i+1];
                        }
                        if(args[i] == "-v"){
                              value = args[i+1];
                        }
                  }

                  //checks for the different arguments
                  if(address == ""){
                        const char *message = "Instruction is missing address (-a)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }
                  if(type == ""){
                        const char *message = "Instruction is missing type (-t)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }
                  if(type == ""){
                        const char *message = "Instruction is missing value (-v)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }
                  if(strtol(address.c_str(), NULL, 0) == 0){
                        const char *message = "Invalid address (-a)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }

                  uint val = 0;
                  float valf = 0;  

                  //parse value as int or float (this is kinda ugly ill change it someday)
                  if(type!="f32"){
                        val = std::strtoul(value.c_str(), NULL, 0);
                  }else{
                        uint32_t num;
                        sscanf(value.c_str(), "%x", &num);
                        valf = *((float*)&num);
                  }

                  //this can all be simplified to a bitshift depending on the type but we have those functions ready anyway 
                  if(type=="u8"){
                        Poke8((uint32_t)strtoul(address.c_str(), NULL, 0), (uint8_t)val);
                  }
                  else if(type=="u16"){
                        Poke16((uint32_t)strtoul(address.c_str(), NULL, 0), (uint16_t)val);
                  }
                  else if(type=="u32"){
                        Poke((uint32_t)strtoul(address.c_str(), NULL, 0), (uint32_t)val);
                  }
                  else if(type=="f32"){
                        PokeF32((uint32_t)strtoul(address.c_str(), NULL, 0), valf);
                  }
                  else {
                        const char *message = "Invalid type (-u)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }

                  //writes a message to the socket so that the client knows the request has been handled
                  char const *message = "ok\n";
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                  write(socket->getClientFD(), message, strlen(message));
                  
            }

            else if(instruction == "peekmultiple"){
                  std::vector<std::string> address;
                  std::string type = "";

                  //gets important informations from the request
                  for (uint i = 0; i < args.size(); i++)
                  {
                        if(args[i] == "-t"){
                              type = args[i+1];
                        }
                        if(args[i] == "-a"){
                              address.emplace_back(args[i+1]);
                        }
                  }

                  //checks for the different arguments
                  if(address.size() == 0){
                        const char *message = "Instruction is missing address (-a)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }
                  if(type == ""){
                        const char *message = "Instruction is missing type (-t)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }

                  std::string message = "";

                  //this can all be simplified to a bitshift depending on the type but we have those functions ready anyway 
                  for (size_t i = 0; i < address.size(); i++)
                  {
                        std::string val = "";
                        if(type=="u8"){
                              val = std::to_string(Peek8((uint32_t)strtol(address[i].c_str(), NULL, 0)));
                        }
                        else if(type=="u16"){
                              val = std::to_string(Peek16((uint32_t)strtol(address[i].c_str(), NULL, 0)));
                        }
                        else if(type=="u32"){
                              val = std::to_string(Peek((uint32_t)strtol(address[i].c_str(), NULL, 0)));
                        }
                        else if(type=="f32"){
                              val = std::to_string(PeekF32((uint32_t)strtol(address[i].c_str(), NULL, 0)));
                        }
                        else {
                              const char *message = "Invalid type (-u)\n";
                              DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                              write(socket->getClientFD(), message, strlen(message));
                              
                        }
                        message += val + "|";
                  }
                  //finally, send back our stuff to the client
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                  write(socket->getClientFD(), message.c_str(), strlen(message.c_str()));
                  
            }

            else if (instruction == "pokemultiple"){
                  std::vector<std::string> address;
                  std::string type = "";
                  std::vector<std::string> value;

                  //gets important informations from the request
                  for (uint i = 0; i < args.size(); i++)
                  {
                        if(args[i] == "-t"){
                              type = args[i+1];
                        }
                        if(args[i] == "-a"){
                              address.emplace_back(args[i+1]);
                        }
                        if(args[i] == "-v"){
                              value.emplace_back(args[i+1]);
                        }
                  }

                  //checks for the different arguments
                  if(address.size() == 0){
                        const char *message = "Instruction is missing address (-a)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }
                  if(type == ""){
                        const char *message = "Instruction is missing type (-t)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }
                  if(value.size() == 0){
                        const char *message = "Instruction is missing value (-v)\n";
                        DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                        write(socket->getClientFD(), message, strlen(message));
                        
                  }

                  for (size_t i = 0; i < address.size(); i++)
                  {
                        uint val = 0;
                        float valf = 0;  

                        //parse value as int or float (this is kinda ugly ill change it someday)
                        if(type!="f32"){
                              val = std::strtoul(value[i].c_str(), NULL, 0);
                        }else{
                              uint32_t num;
                              sscanf(value[i].c_str(), "%x", &num);
                              valf = *((float*)&num);

                        }

                        //this can all be simplified to a bitshift depending on the type but we have those functions ready anyway 
                        if(type=="u8"){
                              Poke8((uint32_t)strtoul(address[i].c_str(), NULL, 0), (uint8_t)val);
                        }
                        else if(type=="u16"){
                              Poke16((uint32_t)strtoul(address[i].c_str(), NULL, 0), (uint16_t)val);
                        }
                        else if(type=="u32"){
                              Poke((uint32_t)strtoul(address[i].c_str(), NULL, 0), (uint32_t)val);
                        }
                        else if(type=="f32"){
                              PokeF32((uint32_t)strtoul(address[i].c_str(), NULL, 0), valf);
                        }
                        else {
                              const char *message = "Invalid type (-u)\n";
                              DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                              write(socket->getClientFD(), message, strlen(message));
                              
                        }
                  }
                  //writes a message to the socket so that the client knows the request has been handled
                  char const *message = "ok\n";
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                  write(socket->getClientFD(), message, strlen(message));
                  
            }

            else if(instruction == "find"){
                  std::string value = "";
                  std::string step = "";

                  //gets important informations from the request
                  for (uint i = 0; i < args.size(); i++)
                  {
                        if(args[i] == "-s"){
                              step = args[i+1];
                        }
                        if(args[i] == "-v"){
                              value = args[i+1];
                        }
                  }


                  if(step == "first"){
                        uint32_t val;
                        val = std::strtol(value.c_str(), NULL, 0);
                        StartFindValue32(val);
                  }
                  else if (step == "next"){
                        uint32_t val;
                        val = std::strtol(value.c_str(), NULL, 0);
                        ContinueFindValue32(val);
                  }
                  //if we dont recognize the step we just print out the array
                  else{
                        for (uint32_t i = 0; i < potentialAddresses.size(); i++)
                        {
                              char const *message = (std::to_string(potentialAddresses[i]) + "\n").c_str();
                              DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                              write(socket->getClientFD(), message, strlen(message));
                        }
                        
                  }
                  //writes a message to the socket so that the client knows the request has been handled
                  char const *message = "ok\n";
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                  write(socket->getClientFD(), message, strlen(message));
                  
            }

            else if(instruction == "pause"){
                  //writes a message to the socket so that the client knows the request has been handled
                  const char *message = "ok\n";
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                  write(socket->getClientFD(), message, strlen(message));

                  OSSuspendThread(mainThread);
                  
            }

            else if(instruction == "advance"){
                  const char *message = "ok\n";
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                  write(socket->getClientFD(), message, strlen(message));

                  //for some reasons the OSSleepTicks work weirdly, this is very hacky and it just so happens that resuming
                  //thread twice advances by a frame? idk
                  OSResumeThread(mainThread);
                  OSSleepTicks(OSTime(1000));
                  OSSuspendThread(mainThread);

                  OSResumeThread(mainThread);
                  OSSleepTicks(OSTime(1000));
                  OSSuspendThread(mainThread);

                  
            }

            else if(instruction == "resume"){
                  const char *message = "ok\n";
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                  write(socket->getClientFD(), message, strlen(message));

                  //resumes the main thread
                  OSResumeThread(mainThread);
            }

            else if(instruction == "call"){
                  std::string addr = "";

                  //gets important informations from the request
                  for (uint i = 0; i < args.size(); i++)
                  {
                        if(args[i] == "-a"){
                              addr = args[i+1];
                        }
                  }

                  uint32_t val;
                  val = std::strtol(addr.c_str(), NULL, 0);
                  Call(val);
                  const char *message = "ok\n";
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                  write(socket->getClientFD(), message, strlen(message));
            }

            else if(instruction == "drawtext"){
                  std::string text = "";
                  uint8_t r = 0;
                  uint8_t g = 0;
                  uint8_t b = 0;
                  uint8_t a = 0;

                  //gets important informations from the request
                  for (uint i = 0; i < args.size(); i++)
                  {
                        if(args[i] == "-text"){
                              int j = i + 1;
                              text += args[j];
                              text.erase(0, 1);

                              if(args[j].back() != ')'){
                                    j++;
                                    while(args[j].back() != ')' && j < args.size()){
                                          text += " ";
                                          text += args[j];
                                          j++;
                                    }
                                    text += " ";
                                    text += args[j];
                              }

                              text.pop_back();
                        }

                        if(args[i] == "-r"){
                              r = std::stoi(args[i+1]);
                        }
                        if(args[i] == "-g"){
                              g = std::stoi(args[i+1]);
                        }
                        if(args[i] == "-b"){
                              b = std::stoi(args[i+1]);
                        }
                        if(args[i] == "-a"){
                              a = std::stoi(args[i+1]);
                        }
                  }

                  NotificationModule_UpdateDynamicNotificationText(currentText, text.c_str());
                  NotificationModule_UpdateDynamicNotificationTextColor(currentText, _NMColor{.r = r, .g = g, .b = b, .a = a});

                  if(a == 0 || text == "" || (a == 2 && r == 0 && g == 0 && b == 255)){
                        NotificationModule_UpdateDynamicNotificationText(currentText, ".");
                        NotificationModule_UpdateDynamicNotificationTextColor(currentText, _NMColor{255, 0, 0, 2});
                  }

                  const char *message = "ok\n";
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                  write(socket->getClientFD(), message, strlen(message));
            }

            else{
                  const char *message = "Invalid Instruction. You need to use either of the following instructions : \npeek -t (type:u8, u16, u32, f32) -a (address:0xFFFFFFFF) \npoke -t (type:u8, u16, u32, f32) -a (address:0xFFFFFFFF) -v (value:0xFF)\nfind -s (step:first, next, list) -v (value:0xFF)\npause (pauses the main wiiu thread execution)\nresume (resumes the main wiiu thread)\nadvance (advances the wiiu thread by 1 frame)\n";
                  DEBUG_FUNCTION_LINE_INFO("Writing to client %d: %s", socket->getClientFD(), message);
                  write(socket->getClientFD(), message, strlen(message));
            }
      }
      return 0;
}

TCPServer::TCPServer(int port) {
    this->port = port;
    this->sockfd = -1;
    this->clientfd = -1;
    memset(&(this->sock_addr),0,sizeof(this->sock_addr));

    pThread = std::jthread(DoTCPThread, this);
    pThread.detach();
    
    //pThread = CThread::create(TCPServer::DoTCPThread, (void*)this, OS_THREAD_ATTRIB_AFFINITY_CPU1 | OS_THREAD_ATTRIB_DETACHED, 0xc);
    //pThread->resumeThread();
}

TCPServer::~TCPServer() {
    exitThread = 1;
    CloseSockets();
    DEBUG_FUNCTION_LINE("Thread will be closed\n");
    
    ICInvalidateRange((void*)&exitThread, 4);
    DCFlushRange((void*)&exitThread, 4);

    DEBUG_FUNCTION_LINE("Deleting it!\n");
    pThread.request_stop();
    
    DEBUG_FUNCTION_LINE("Thread done\n");
}

void TCPServer::CloseSockets() {
    if (this->sockfd != -1) {
      shutdown(sockfd, SHUT_RD);
      close(sockfd);  
    }
    if (this->clientfd != -1) {
      close(this->clientfd);
    }
    this->sockfd = -1;
    this->clientfd = -1;
}

void TCPServer::DoTCPThreadInternal(std::stop_token stop_token) {
    int ret;
    socklen_t len;
    NotificationModule_InitLibrary();
    if (NotificationModule_AddDynamicNotificationEx(".", &currentText, {0, 0, 255, 2}, {255, 255, 255, 0}, nullptr, nullptr, false) != NOTIFICATION_MODULE_RESULT_SUCCESS) {
      currentText = NULL;
    }
    connected = false;
    while (!stop_token.stop_requested()) {
        if(exitThread) {
            break;
        }
        memset(&(this->sock_addr),0,sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = this->port;
        sock_addr.sin_addr.s_addr = 0;

        this->sockfd = ret = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(ret == -1) {
            CloseSockets();
            continue;
        }
        int enable = 1;

        setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

        ret = bind(this->sockfd, (sockaddr *)&sock_addr, 16);
        if(ret < 0) {
            CloseSockets();
            continue;
        }
        ret = listen(this->sockfd, 1);
        if(ret < 0) {
            CloseSockets();
            continue;
        }

        do {
            DEBUG_FUNCTION_LINE("Waiting for a connection\n");
            if(exitThread) {
                break;
            }
            len = 16;
            clientfd = ret = accept(sockfd, (sockaddr *)&(sock_addr), &len);

            if(ret == -1) {
                CloseSockets();
                break;
            }

            NotificationModule_UpdateDynamicNotificationTextColor(currentText, {255, 255, 255, 2});
            connected = true;
            DEBUG_FUNCTION_LINE("Connection accepted\n");
            //commands
            Commands(this, stop_token);

            NotificationModule_UpdateDynamicNotificationTextColor(currentText, {0, 0, 255, 2});
            DEBUG_FUNCTION_LINE("Client disconnected\n");

            if(clientfd != -1) {
                close(clientfd);
            }
            clientfd = -1;
        } while(!stop_token.stop_requested());
        DEBUG_FUNCTION_LINE("Closing TCPServer\n");
        connected = false;
        onConnectionClosed();
        CloseSockets();
        continue;
    }
    NotificationModule_DeInitLibrary();
    DEBUG_FUNCTION_LINE("Ending DoTCPThreadInternal\n");
}

void TCPServer::DoTCPThread(std::stop_token stop_token, void *arg) {
    TCPServer * args = (TCPServer * )arg;
    return args->DoTCPThreadInternal(stop_token);
}