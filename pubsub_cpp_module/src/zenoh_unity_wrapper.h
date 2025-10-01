
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

// ---- Query Server (Queryable) ----------------------------------------------
// Callback invoked on a background thread when a query arrives.
// DO NOT touch Unity APIs here—queue to main thread and finish via ZU_CompleteRequest / ZU_FailRequest.
typedef void (ZU_CALL *ZU_QueryCallback)(
    uint64_t request_id,
    const char* key_expr,
    const uint8_t* payload, int32_t payload_len,
    const char* parameters,
    void* user_data);

// Declare a server (queryable) for `key_expr`. The native side waits up to `timeout_ms`
// for ZU_CompleteRequest / ZU_FailRequest. On timeout, the client gets an error.
ZU_API int32_t ZU_CreateServer(
    ZU_NodeHandle node,
    const char* key_expr,
    ZU_QueryCallback cb,
    void* user_data,
    int32_t timeout_ms /* e.g., 3000 */);

ZU_API int32_t ZU_RemoveServer(ZU_NodeHandle node, const char* key_expr);

// Finish a pending request successfully (send reply bytes).
ZU_API int32_t ZU_CompleteRequest(
    ZU_NodeHandle node,
    uint64_t request_id,
    const uint8_t* bytes,
    int32_t len);

// Fail a pending request.
ZU_API int32_t ZU_FailRequest(
    ZU_NodeHandle node,
    uint64_t request_id,
    const char* message);


#ifdef __cplusplus
} // extern "C"
#endif
