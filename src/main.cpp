#include "main.h"
#include "TCPGecko.hpp"
#include "utils/logger.h"

#include <wups.h>
#include <utils/CThread.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("TCPGecko");
WUPS_PLUGIN_DESCRIPTION("Port of the TCPGecko features over to an aroma plugin.");
WUPS_PLUGIN_VERSION("v0.2");
WUPS_PLUGIN_AUTHOR("Teotia444");
WUPS_USE_STORAGE("tcpgecko"); // Unique id for the storage api

OSThread* mainThread = NULL;
TCPServer* socketThread = NULL;

ON_APPLICATION_START() {
    initLogging();
    DEBUG_FUNCTION_LINE_INFO("TCPGecko Plugin Application start");
    
    mainThread = OSGetCurrentThread();
    socketThread = new TCPServer(7332);
}

ON_APPLICATION_REQUESTS_EXIT(){
    delete socketThread;
    deinitLogging();
}


