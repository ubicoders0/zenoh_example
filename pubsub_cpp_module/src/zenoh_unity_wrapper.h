
// zenoh_unity.h
#pragma once

#ifdef _WIN32
  #ifdef ZENOH_UNITY_BUILD
    #define ZU_API __declspec(dllexport)
  #else
    #define ZU_API __declspec(dllimport)
  #endif
  #define ZU_CALL __cdecl
#else
  #define ZU_API __attribute__((visibility("default")))
  #define ZU_CALL
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to a Node instance.
// 0 means failure / invalid handle.
typedef uint64_t ZU_NodeHandle;

// Subscription callback invoked from a background thread.
// NOTE: Do NOT touch UnityEngine APIs directly in this callback.
// Forward the data to Unity's main thread (see C# wrapper below).
typedef void (ZU_CALL *ZU_MessageCallback)(
    const char* key,
    const uint8_t* data,
    int32_t len,
    void* user_data);

// ---- Lifecycle --------------------------------------------------------------
ZU_API ZU_NodeHandle ZU_CreateNode(const char* name /* nullable */);
ZU_API void          ZU_DestroyNode(ZU_NodeHandle node);
ZU_API void          ZU_ShutdownNode(ZU_NodeHandle node);

// ---- Publisher API ----------------------------------------------------------
ZU_API int32_t ZU_HasPublisher(ZU_NodeHandle node, const char* key);
ZU_API int32_t ZU_CreatePublisher(ZU_NodeHandle node, const char* key);
ZU_API int32_t ZU_RemovePublisher(ZU_NodeHandle node, const char* key);
ZU_API int32_t ZU_Publish(ZU_NodeHandle node, const char* key,
                          const uint8_t* data, int32_t len);

// ---- Subscriber API ---------------------------------------------------------
ZU_API int32_t ZU_HasSubscriber(ZU_NodeHandle node, const char* key);
ZU_API int32_t ZU_CreateSubscriber(ZU_NodeHandle node, const char* key,
                                   ZU_MessageCallback cb, void* user_data);
ZU_API int32_t ZU_RemoveSubscriber(ZU_NodeHandle node, const char* key);

#ifdef __cplusplus
} // extern "C"
#endif
