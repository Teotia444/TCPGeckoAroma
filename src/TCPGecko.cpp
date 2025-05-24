#include <main.h>
#include <coreinit/filesystem.h>
#include <vector>
#include <string>
#include <algorithm>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <string.h>
#include <thread>
#include <map>
#include <notifications/notifications.h>
#include <gx2/draw.h>

//addresses vector for the find command
std::vector<uint32_t> potentialAddresses;
bool stop;
NotificationModuleHandle currentText;

struct clientDetails{
      int32_t clientfd;  // client file descriptor
      int32_t serverfd;  // server file descriptor
      std::vector<int> clientList; // for storing all the client fd
      clientDetails(void){ // initializing the variable
            this->clientfd=-1;
            this->serverfd=-1;
      }
};

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

void stopSocket(bool v){
      stop=v;
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
void* Commands(int client, std::string command){
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
                  write(client, message, strlen(message));
                  return 0;
            }
            if(type == ""){
                  const char *message = "Instruction is missing type (-t)\n";
                  write(client, message, strlen(message));
                  return 0;
            }
            if(strtol(address.c_str(), NULL, 0) == 0){
                  const char *message = "Invalid address (-a)\n";
                  write(client, message, strlen(message));
                  return 0;
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
                  write(client, message, strlen(message));
                  return 0;
            }

            //finally, send back our stuff to the client
            write(client, val.c_str(), strlen(val.c_str()));
            return 0;
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
                  write(client, message, strlen(message));
                  return 0;
            }
            if(type == ""){
                  const char *message = "Instruction is missing type (-t)\n";
                  write(client, message, strlen(message));
                  return 0;
            }
            if(type == ""){
                  const char *message = "Instruction is missing value (-v)\n";
                  write(client, message, strlen(message));
                  return 0;
            }
            if(strtol(address.c_str(), NULL, 0) == 0){
                  const char *message = "Invalid address (-a)\n";
                  write(client, message, strlen(message));
                  return 0;
            }

            int val = 0;
            float valf = 0;  

            //parse value as int or float (this is kinda ugly ill change it someday)
            if(type!="f32"){
                  val = std::strtol(value.c_str(), NULL, 0);
            }else{
                  uint32_t num;
                  sscanf(value.c_str(), "%x", &num);
                  valf = *((float*)&num);
            }

            //this can all be simplified to a bitshift depending on the type but we have those functions ready anyway 
            if(type=="u8"){
                  Poke8((uint32_t)strtol(address.c_str(), NULL, 0), (uint8_t)val);
            }
            else if(type=="u16"){
                  Poke16((uint32_t)strtol(address.c_str(), NULL, 0), (uint16_t)val);
            }
            else if(type=="u32"){
                  Poke((uint32_t)strtol(address.c_str(), NULL, 0), (uint32_t)val);
            }
            else if(type=="f32"){
                  PokeF32((uint32_t)strtol(address.c_str(), NULL, 0), valf);
            }
            else {
                  const char *message = "Invalid type (-u)\n";
                  write(client, message, strlen(message));
                  return 0;
            }

            //writes a message to the socket so that the client knows the request has been handled
            char const *message = "ok\n";
            write(client, message, strlen(message));
            return 0;
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
                  write(client, message, strlen(message));
                  return 0;
            }
            if(type == ""){
                  const char *message = "Instruction is missing type (-t)\n";
                  write(client, message, strlen(message));
                  return 0;
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
                        write(client, message, strlen(message));
                        return 0;
                  }
                  message += val + "|";
            }
            //finally, send back our stuff to the client
            write(client, message.c_str(), strlen(message.c_str()));
            return 0;
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
                  write(client, message, strlen(message));
                  return 0;
            }
            if(type == ""){
                  const char *message = "Instruction is missing type (-t)\n";
                  write(client, message, strlen(message));
                  return 0;
            }
            if(value.size() == 0){
                  const char *message = "Instruction is missing value (-v)\n";
                  write(client, message, strlen(message));
                  return 0;
            }

            for (size_t i = 0; i < address.size(); i++)
            {
                  int val = 0;
                  float valf = 0;  

                  //parse value as int or float (this is kinda ugly ill change it someday)
                  if(type!="f32"){
                        val = std::strtol(value[i].c_str(), NULL, 0);
                  }else{
                        uint32_t num;
                        sscanf(value[i].c_str(), "%x", &num);
                        valf = *((float*)&num);
                  }

                  //this can all be simplified to a bitshift depending on the type but we have those functions ready anyway 
                  if(type=="u8"){
                        Poke8((uint32_t)strtol(address[i].c_str(), NULL, 0), (uint8_t)val);
                  }
                  else if(type=="u16"){
                        Poke16((uint32_t)strtol(address[i].c_str(), NULL, 0), (uint16_t)val);
                  }
                  else if(type=="u32"){
                        Poke((uint32_t)strtol(address[i].c_str(), NULL, 0), (uint32_t)val);
                  }
                  else if(type=="f32"){
                        PokeF32((uint32_t)strtol(address[i].c_str(), NULL, 0), valf);
                  }
                  else {
                        const char *message = "Invalid type (-u)\n";
                        write(client, message, strlen(message));
                        return 0;
                  }
            }
            //writes a message to the socket so that the client knows the request has been handled
            char const *message = "ok\n";
            write(client, message, strlen(message));
            return 0;
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
                        write(client, message, strlen(message));
                  }
                  return 0;
            }
            //writes a message to the socket so that the client knows the request has been handled
            char const *message = "ok\n";
            write(client, message, strlen(message));
            return 0;
      }
      
      else if(instruction == "pause"){
            //writes a message to the socket so that the client knows the request has been handled
            const char *message = "ok\n";
            write(client, message, strlen(message));

            //suspend the main thread
            OSThread* maint = GetMainThread();
            OSSuspendThread(maint);
            return 0;
      }

      else if(instruction == "advance"){
            const char *message = "ok\n";
            write(client, message, strlen(message));

            OSThread* maint = GetMainThread();
            //for some reasons the OSSleepTicks work weirdly, this is very hacky and it just so happens that resuming
            //thread twice advances by a frame? idk
            OSResumeThread(maint);
            OSSleepTicks(OSTime(1000));
            OSSuspendThread(maint);

            OSResumeThread(maint);
            OSSleepTicks(OSTime(1000));
            OSSuspendThread(maint);

            return 0;
      }
      
      else if(instruction == "resume"){
            const char *message = "ok\n";
            write(client, message, strlen(message));

            //resumes the main thread
            OSThread* maint = GetMainThread();
            OSResumeThread(maint);
            return 0;
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
            write(client, message, strlen(message));
            return 0;
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
            const char *message = "ok\n";
            write(client, message, strlen(message));
            return 0;
      }

      else{
            const char *message = "Invalid Instruction. You need to use either of the following instructions : \npeek -t (type:u8, u16, u32, f32) -a (address:0xFFFFFFFF) \npoke -t (type:u8, u16, u32, f32) -a (address:0xFFFFFFFF) -v (value:0xFF)\nfind -s (step:first, next, list) -v (value:0xFF)\npause (pauses the main wiiu thread execution)\nresume (resumes the main wiiu thread)\nadvance (advances the wiiu thread by 1 frame)\n";
            write(client, message, strlen(message));
            return 0;
      }
}

int Start(int argc, const char **argv){
      NotificationModule_InitLibrary();
      if (NotificationModule_AddDynamicNotificationEx("", &currentText, {255, 255, 255, 255}, {255, 255, 255, 0}, nullptr, nullptr, false) != NOTIFICATION_MODULE_RESULT_SUCCESS) {
            currentText = 0;
      }


      
      auto client = new clientDetails();

      //init server socket
      client->serverfd = socket(AF_INET, SOCK_STREAM, 0);
      if(client->serverfd <= 0){
            delete client;
            return 1;
      }


      int opt=1;
      if(setsockopt(client->serverfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof opt) <0){
            delete client;
            return 1;
      }

      struct sockaddr_in serverAddr;
      serverAddr.sin_family = AF_INET;
      serverAddr.sin_port = htons(7332);
      serverAddr.sin_addr.s_addr = INADDR_ANY;

      if(bind(client->serverfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))<0){
            delete client;
            return 1;
      }
      if(listen(client->serverfd, 10)<0){
            delete client;
            return 1;
      }
      
      fd_set readfds;
      size_t  valread;
      int maxfd;
      int sd=0;
      int activity;

      while (!stop){
            FD_ZERO(&readfds);
            FD_SET(client->serverfd, &readfds);
            maxfd=client->serverfd;

            //copying the client list to readfds
            //so that we can listen to all the client
            for(auto sd:client->clientList){
                  FD_SET(sd, &readfds);
                  if (sd>maxfd){
                        maxfd=sd;
                  }
            }
            if (sd>maxfd){
                  maxfd=sd;
            }

            activity=select(maxfd+1, &readfds, NULL, NULL, NULL);
            if (activity<0){
                  continue;
            }

            /*
              if something happen on client->serverfd then it means its
              new connection request
             */
            if (FD_ISSET(client->serverfd, &readfds)) {
                  client->clientfd = accept(client->serverfd, (struct sockaddr *) NULL, NULL);
                  if (client->clientfd < 0) {
                        continue;
                  }
                  
                  //adding client to list
                  client->clientList.push_back(client->clientfd);
                  //NotificationModule_UpdateDynamicNotificationText(*currentText, "text.c_str()");
                  
                  
            }

            //for storing the recive message
            char message[1024];
            for(uint i=0;i<client->clientList.size();++i){
                  sd=client->clientList[i];
                  if (FD_ISSET(sd, &readfds)){
                        valread=read(sd, message, 1024);
                        //check if client disconnected
                        if (valread==0){
                              close(sd);
                              //remove the client from the list 
                              client->clientList.erase(client->clientList.begin()+i);
                              //NotificationModule_AddInfoNotification("client disconnected");
                        }else{
                              std::thread t1(Commands, client->clientList[i], (std::string)message);
                              t1.detach();
                        }
                        memset(message, 0, 1024);
                        
                  }
            }
      }
      
      close(client->serverfd);
      delete client;
      stop=false;
      return 0;
      
}