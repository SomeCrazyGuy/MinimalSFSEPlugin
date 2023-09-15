#include <Windows.h>
#include <stdint.h>

#pragma region SFSE_SETUP_CODE
#define MAKE_VERSION(major, minor, build) ((((major)&0xFF)<<24)|(((minor)&0xFF)<<16)|(((build)&0xFFF)<<4))

typedef uint32_t PluginHandle;

typedef struct SFSEPluginVersionData_t {
        uint32_t        dataVersion;
        uint32_t        pluginVersion;
        char            name[256];
        char            author[256];
        uint32_t        addressIndependence;
        uint32_t        structureIndependence;
        uint32_t        compatibleVersions[16];
        uint32_t        seVersionRequired;
        uint32_t        reservedNonBreaking;
        uint32_t        reservedBreaking;
} SFSEPluginVersionData;

typedef struct SFSEPluginInfo_t {
        uint32_t	infoVersion;
        const char*     name;
        uint32_t	version;
} SFSEPluginInfo;

typedef struct SFSEInterface_t {
        uint32_t	sfseVersion;
        uint32_t	runtimeVersion;
        uint32_t	interfaceVersion;
        void*           (*QueryInterface)(uint32_t id);
        PluginHandle    (*GetPluginHandle)(void);
        SFSEPluginInfo* (*GetPluginInfo)(const char* name);
} SFSEInterface;

typedef struct SFSEMessage_t {
        const char*     sender;
        uint32_t        type;
        uint32_t        dataLen;
        void*           data;
} SFSEMessage;

typedef void (*SFSEMessageEventCallback)(SFSEMessage* msg);

typedef struct SFSEMessagingInterface_t {
        uint32_t        interfaceVersion;
        bool            (*RegisterListener)(PluginHandle listener, const char* sender, SFSEMessageEventCallback handler);
        bool	        (*Dispatch)(PluginHandle sender, uint32_t messageType, void* data, uint32_t dataLen, const char* receiver);
}SFSEMessagingInterface;

static void OnPreload();
static void OnPostLoad();
static void OnDelayLoad();

static DWORD ThreadProc_OnDelayLoad(void* unused) {
        (void)unused;
        Sleep(8000); // a reasonable amount of time for the game to initialize
        OnDelayLoad();
        return 0;
}

static void MyMessageEventCallback(SFSEMessage* msg) {
        if (msg->type == 0 /* postload */) {
                OnPostLoad();
                CreateThread(NULL, 4096, ThreadProc_OnDelayLoad, NULL, 0, NULL);
        }
}

extern "C" __declspec(dllexport) void SFSEPlugin_Preload(const SFSEInterface * sfse) {
        PluginHandle my_handle = sfse->GetPluginHandle();
        SFSEMessagingInterface* msg = (SFSEMessagingInterface*)sfse->QueryInterface(1 /* messaging interface */);
        msg->RegisterListener(my_handle, "SFSE", MyMessageEventCallback);
        OnPreload();
}
#pragma endregion SFSE Setup Code

// Relocate an offset from imagebase
template<typename T> static inline T Relocate(uintptr_t offset);

// Write memory
template<typename T> static inline bool WriteMemory(void* address, T* buffer);


#pragma region HELPER_FUNCTIONS

static uintptr_t relocate_impl(uintptr_t offset) {
        static uintptr_t base = (uintptr_t)GetModuleHandleW(NULL);
        return base + offset;
}

template<typename T>
static inline T Relocate(uintptr_t offset) {
        return (T) relocate_impl(offset);
}

static bool write_process_memory_impl(void* address, void* buffer, size_t buffer_size) {
        static HANDLE process = GetCurrentProcess();
        size_t bytes_written = 0;
        auto ret = WriteProcessMemory(process, address, buffer, buffer_size, &bytes_written);
        return (ret && (bytes_written == buffer_size));
}

template<typename T>
static inline bool WriteMemory(void* address, T* buffer) {
        return write_process_memory_impl(address, buffer, sizeof(T));
}

#pragma endregion Functions to help patch memory

// ----------------------------------------------------------------------------
// ----                Put Your SFSE Plugin Code Below                     ----
// ----------------------------------------------------------------------------

extern "C" __declspec(dllexport) SFSEPluginVersionData SFSEPlugin_Version = {
        1, // SFSE api version
        1, // Plugin version
        "Plugin Name",
        "Plugin Author",
        1, // AddressIndependence::Signatures
        1, // StructureIndependence::NoStructs
        {MAKE_VERSION(1, 7, 29), 0}, //game version 1.7.29
        0, //does not rely on any sfse version
        0, 0 //reserved fields
};

// Called once during preload
// use this to patch the game exe before static initialization
static void OnPreload() {
        MessageBoxA(NULL, "OnPreload", "Debug", 0);
}

// Called once when SFSE sends the postload event after static initialization
static void OnPostLoad() {
        MessageBoxA(NULL, "OnPostLoad", "Debug", 0);
}

// This function is called once about 8 seconds after the dll is loaded
// should be enough time for the game to initialize any data you want to modify
static void OnDelayLoad() {
        MessageBoxA(NULL, "OnDelayLoad", "Debug", 0);
}