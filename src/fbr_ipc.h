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
#define FBR_IPC_TARGET_COUNT 2

typedef enum FbrIPCTargetType {
    FBR_IPC_TARGET_EXTERNAL_TEXTURE = 0,
    FBR_IPC_TARGET_EXTERNAL_CAMERA_UBO = 1,
} FbrIPCTargetType;

typedef struct FbrIPCExternalTextureParam {
    HANDLE handle;
    uint16_t width;
    uint16_t height;
} FbrIPCExternalTextureParam;

typedef struct FbrIPCExternalCameraUBO {
    HANDLE handle;
} FbrIPCExternalCameraUBO;


//#define FBR_IPC_TARGET_SIGNATURE(N) EXPAND_CONCAT(FBR_IPC_TARGET_SIGNATURE_, N)
//#define FBR_IPC_TARGET_SIGNATURE_0 FbrIPCExternalTextureParam
//#define FBR_IPC_TARGET_SIGNATURE_1 FbrIPCExternalCameraUBO

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

bool fbrIPCDequeAvailable(const FbrIPCBuffer *pIPCBuffer);

int fbrIPCPollDeque(FbrApp *pApp, FbrIPC *pIPC);

void fbrIPCEnque(FbrIPCBuffer *pIPCBuffer, FbrIPCTargetType target, void *param);

int fbrCreateProducerIPC(FbrIPC **ppAllocIPC);

int fbrCreateReceiverIPC(FbrIPC **ppAllocIPC);

int fbrDestroyIPC(FbrIPC *pIPC);

#endif //FABRIC_IPC_H