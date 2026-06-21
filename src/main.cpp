#include "main.h"
#include "TCPGecko.hpp"
#include "utils/logger.h"

#include <wups.h>
<<<<<<< Updated upstream
#include <utils/CThread.h>
=======
#include <wups/config_api.h>
#include <malloc.h>
#include <thread>
>>>>>>> Stashed changes

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("TCPGecko");
WUPS_PLUGIN_DESCRIPTION("Port of the TCPGecko features over to an aroma plugin.");
WUPS_PLUGIN_VERSION("v0.2");
WUPS_PLUGIN_AUTHOR("Teotia444");
WUPS_USE_STORAGE("tcpgecko"); // Unique id for the storage api

<<<<<<< Updated upstream
OSThread* mainThread = NULL;
TCPServer* socketThread = NULL;
=======
WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    //maybe add something to the config menu from aroma in the future?
    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();
}

OSThread* ct;
std::jthread* thread;

OSThread* GetMainThread(){
    return ct;
}

INITIALIZE_PLUGIN() {
    initLogging();
    DEBUG_FUNCTION_LINE_INFO("Initialized TCPGecko Plugin");
}
>>>>>>> Stashed changes

ON_APPLICATION_START() {
    initLogging();
    DEBUG_FUNCTION_LINE_INFO("TCPGecko Plugin Application start");
    
<<<<<<< Updated upstream
    mainThread = OSGetCurrentThread();
    socketThread = new TCPServer(7332);
}

ON_APPLICATION_REQUESTS_EXIT(){
    delete socketThread;
=======
    ct = OSGetCurrentThread();
    
    thread = new std::jthread(Start);
    OSSetThreadAffinity((OSThread *)thread->native_handle(), OS_THREAD_ATTRIB_AFFINITY_CPU2);
}

ON_APPLICATION_REQUESTS_EXIT(){
    thread->request_stop();
    thread->join();
    DEBUG_FUNCTION_LINE_INFO("sure ig bro");
}

DEINITIALIZE_PLUGIN() {
>>>>>>> Stashed changes
    deinitLogging();
}


