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
OSThread* st;

OSThread* GetMainThread(){
    return ct;
}

static void thread_deallocator(OSThread *thread, void *stack)
{
    free(stack);
    free(thread);
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
    stopSocket(false);

    //init memory space for our socket thread
    st = (OSThread*)calloc(1, sizeof(OSThread));
    int stack_size = 6 * 1024;
    void* stack_addr = (uint8_t*) memalign(8, stack_size) + stack_size;

    //asks wiiu to open thread on another core
    if(!OSCreateThread(st, Start, 0, NULL, stack_addr, stack_size, 0x10, OS_THREAD_ATTRIB_AFFINITY_CPU1)) return;
    OSSetThreadDeallocator(st, thread_deallocator);
    OSDetachThread(st);
    OSResumeThread(st);
    
}

ON_APPLICATION_REQUESTS_EXIT(){
    stopSocket(true);
    OSJoinThread(st, NULL);
    deinitLogging();
}

DEINITIALIZE_PLUGIN() {
>>>>>>> Stashed changes
    deinitLogging();
}


