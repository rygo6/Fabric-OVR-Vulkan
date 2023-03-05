#ifndef FABRIC_IPC_H
#define FABRIC_IPC_H

#include "fbr_app.h"
#include "fbr_macros.h"

#include <stdint.h>

#ifdef WIN32
#include <windows.h>
#endif

#define FBR_IPC_BUFFER_COUNT 256
#define FBR_IPC_BUFFER_SIZE FBR_IPC_BUFFER_COUNT * sizeof(uint8_t)
#define FBR_IPC_HEADER_SIZE 1

#define FBR_IPC_TARGET_COUNT 4

typedef struct FbrIPCBuffer {
    uint8_t head;
    uint8_t tail;
    uint8_t pRingBuffer[FBR_IPC_BUFFER_COUNT];
} FbrIPCBuffer;

typedef struct FbrIPC {
#ifdef WIN32
    HANDLE hMapFile;
#endif
    FbrIPCBuffer *pIPCBuffer;
    void (*pTargetFuncs[FBR_IPC_TARGET_COUNT])( /*const*/ FbrApp*, void*);
} FbrIPC;

//bool fbrIPCDequeAvailable(const FbrIPCBuffer *pIPCBuffer);

int fbrIPCPollDeque(FbrApp *pApp, FbrIPC *pIPC);

void fbrIPCEnque(FbrIPC *pIPC, FbrIPCTargetType target, void *param);

int fbrCreateProducerIPC(FbrIPC **ppAllocIPC);

int fbrCreateReceiverIPC(FbrIPC **ppAllocIPC);

void fbrDestroyIPC(FbrIPC *pIPC);

#endif //FABRIC_IPC_H
