#ifndef FABRIC_IPC_H
#define FABRIC_IPC_H

#include "fbr_app.h"
#include "fbr_macros.h"

#include <stdint.h>

#ifdef WIN32
#include <windows.h>
#endif

#define FBR_IPC_RING_BUFFER_COUNT 256
#define FBR_IPC_RING_BUFFER_SIZE FBR_IPC_RING_BUFFER_COUNT * sizeof(uint8_t)
#define FBR_IPC_RING_HEADER_SIZE 1

#define FBR_IPC_TARGET_COUNT 4

typedef struct FbrRingBuffer {
    uint8_t head;
    uint8_t tail;
    uint8_t pRingBuffer[FBR_IPC_RING_BUFFER_COUNT];
} FbrRingBuffer;

typedef struct FbrIPCRingBuffer {
#ifdef WIN32
    HANDLE hMapFile;
#endif
    FbrRingBuffer *pRingBuffer;
    void (*pTargetFuncs[FBR_IPC_TARGET_COUNT])( /*const*/ FbrApp*, void*);
} FbrIPCRingBuffer;

typedef struct FbrIPCBuffer {
#ifdef WIN32
    HANDLE hMapFile;
#endif
    void *pBuffer;
} FbrIPCBuffer;

//bool fbrIPCDequeAvailable(const FbrRingBuffer *pRingBuffer);

int fbrIPCPollDeque(FbrApp *pApp, FbrIPCRingBuffer *pIPC);

void fbrIPCEnque(FbrIPCRingBuffer *pIPC, FbrIPCTargetType target, void *param);

int fbrCreateProducerIPCRingBuffer(FbrIPCRingBuffer **ppAllocIPC);

int fbrCreateReceiverIPCRingBuffer(FbrIPCRingBuffer **ppAllocIPC);

int fbrCreateIPCBuffer(FbrIPCBuffer **ppAllocIPC, int bufferSize);

int fbrImportIPCBuffer(FbrIPCBuffer **ppAllocIPC, int bufferSize);

void fbrDestroyIPCBuffer(FbrIPCBuffer *pIPC);

void fbrDestroyIPCRingBuffer(FbrIPCRingBuffer *pIPC);

#endif //FABRIC_IPC_H
