#include "main.h"
#include "TCPGecko.h"
#include "utils/logger.h"

#include <coreinit/filesystem.h>
#include <wups.h>
#include <wups/config_api.h>
#include <malloc.h>
#include <thread>
#include <coreinit/thread.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("TCPGecko");
WUPS_PLUGIN_DESCRIPTION("Port of the TCPGecko features over to an aroma plugin.");
WUPS_PLUGIN_VERSION("v0.1");
WUPS_PLUGIN_AUTHOR("Teotia444");
WUPS_USE_WUT_DEVOPTAB();                // Use the wut devoptabs
WUPS_USE_STORAGE("tcpgecko"); // Unique id for the storage api

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

ON_APPLICATION_START() {
    initLogging();
    DEBUG_FUNCTION_LINE_INFO("TCPGecko Plugin Application start");
    
    ct = OSGetCurrentThread();
    
    thread = new std::jthread(Start);
    OSSetThreadAffinity((OSThread *)thread->native_handle(), OS_THREAD_ATTRIB_AFFINITY_CPU2);
}

ON_APPLICATION_REQUESTS_EXIT(){
    thread->request_stop();
    thread->join();
}

DEINITIALIZE_PLUGIN() {
    deinitLogging();
}



