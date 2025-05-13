#include "main.h"
#include "TCPGecko.h"

#include "utils/logger.h"
#include <coreinit/filesystem.h>
#include <wups.h>
#include <wups/config_api.h>
#include <malloc.h>

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

OSThread* GetMainThread(){
    return ct;
}

static void thread_deallocator(OSThread *thread, void *stack)
{
    free(stack);
    free(thread);
}

ON_APPLICATION_START() {
    ct = OSGetCurrentThread();
    stopSocket(false);

    //init memory space for our socket thread
    OSThread* socket = (OSThread*)calloc(1, sizeof(OSThread));
    int stack_size = 3 * 1024 * 1024;
    void* stack_addr = (uint8_t*) memalign(8, stack_size) + stack_size;

    //asks wiiu to open thread on another core
    if(!OSCreateThread(socket, Start, 0, NULL, stack_addr, stack_size, 0x10, OS_THREAD_ATTRIB_AFFINITY_ANY)) return;
    OSSetThreadDeallocator(socket, thread_deallocator);
    OSDetachThread(socket);
    OSResumeThread(socket);
    
}

ON_APPLICATION_REQUESTS_EXIT(){
    stopSocket(true);
}

