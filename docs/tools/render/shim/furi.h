#pragma once
// Host shim: just enough of the Flipper SDK for the digivice app code to compile with plain cc.
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UNUSED(x)   (void)(x)
#define COUNT_OF(a) (sizeof(a) / sizeof((a)[0]))
#define furi_check(x) \
    do {              \
        if(!(x)) abort(); \
    } while(0)
#define FURI_LOG_I(tag, ...) ((void)(tag))
#define FURI_LOG_E(tag, ...) ((void)(tag))

#define FuriWaitForever 0xFFFFFFFFU
typedef enum {
    FuriStatusOk = 0,
    FuriStatusError = -1,
} FuriStatus;
typedef enum {
    FuriMutexTypeNormal,
} FuriMutexType;
typedef struct FuriMutex FuriMutex;
typedef struct FuriMessageQueue FuriMessageQueue;
typedef struct FuriEventLoop FuriEventLoop;
typedef struct FuriEventLoopTimer FuriEventLoopTimer;
typedef struct FuriEventFlag FuriEventFlag;
typedef struct FuriPubSubSubscription FuriPubSubSubscription;

FuriMutex* furi_mutex_alloc(FuriMutexType type);
FuriStatus furi_mutex_acquire(FuriMutex* mutex, uint32_t timeout);
FuriStatus furi_mutex_release(FuriMutex* mutex);
void furi_mutex_free(FuriMutex* mutex);
FuriMessageQueue* furi_message_queue_alloc(uint32_t count, uint32_t size);
FuriStatus furi_message_queue_put(FuriMessageQueue* q, const void* msg, uint32_t timeout);
FuriStatus furi_message_queue_get(FuriMessageQueue* q, void* msg, uint32_t timeout);
void furi_message_queue_free(FuriMessageQueue* q);
void* furi_record_open(const char* name);
void furi_record_close(const char* name);
bool furi_record_exists(const char* name);
uint32_t furi_get_tick(void);
uint32_t furi_kernel_get_tick_frequency(void);
uint32_t furi_ms_to_ticks(uint32_t ms);
