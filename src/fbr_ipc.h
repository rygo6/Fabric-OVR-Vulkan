#ifndef FABRIC_IPC_H
#define FABRIC_IPC_H

#include <windows.h>
#include <stdint.h>

#define FBR_IPC_BUFFER_COUNT 256
#define FBR_IPC_BUFFER_SIZE FBR_IPC_BUFFER_COUNT * sizeof(FbrIPCBufferElement)

typedef enum FbrIPCType {
    FBR_IPC_TYPE_NONE = 0,
    FBR_IPC_TYPE_INT = 1,
    FBR_IPC_TYPE_FLOAT = 2,
    FBR_IPC_TYPE_CHAR = 3,
    FBR_IPC_TYPE_PTR = 4,
    FBR_IPC_TYPE_LONG = 5,
} FbrIPCType;

#define FBR_IPC_TYPE_BYTE uint8_t
#define FBR_IPC_TARGET_METHOD uint16_t
#define FBR_IPC_TARGET_METHOD_PARAM uint8_t

typedef struct FbrIPCBufferElement {
    FBR_IPC_TYPE_BYTE type;
    FBR_IPC_TARGET_METHOD targetMethod;
    FBR_IPC_TARGET_METHOD_PARAM targetMethodParam;
    union {
        int inValue;
        float floatValue;
        char charValue;
        void* ptrValue;
        long longValue;
        long long longLongValue;
    };
} FbrIPCBufferElement;

typedef struct FbrIPCBuffer {
    uint8_t head;
    uint8_t tail;
    FbrIPCBufferElement pRingBuffer[FBR_IPC_BUFFER_COUNT];
} FbrIPCBuffer;

typedef struct FbrIPC {
    HANDLE hMapFile;
    FbrIPCBuffer *pBuffer;
} FbrIPC;

bool fbrIPCDequeAvailable(const FbrIPC *pIPC);

int fbrIPCDequePtr(const FbrIPC *pIPC, void **pPtr);

int fbrIPCEnquePtr(const FbrIPC *pIPC, void *ptr);

int fbrIPCEnqueInt(const FbrIPC *pIPC, int intValue);

int fbrCreateProducerIPC(FbrIPC **ppAllocIPC);

int fbrCreateReceiverIPC(FbrIPC **ppAllocIPC);

int fbrDestroyIPC(FbrIPC *pIPC);

#endif //FABRIC_IPC_H
