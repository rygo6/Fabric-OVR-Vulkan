#include <assert.h>
#include <stdbool.h>
#include "fbr_ipc.h"
#include "fbr_log.h"

const char sharedMemoryName[] = "FbrIPC";

bool fbrIPCDequeAvailable(const FbrIPCBuffer *pIPCBuffer) {
    return pIPCBuffer->pRingBuffer[pIPCBuffer->head] != pIPCBuffer->pRingBuffer[pIPCBuffer->tail];
}

int fbrIPCDequePtr(FbrIPCBuffer *pIPCBuffer, void **pPtr) {
    if (pIPCBuffer->pRingBuffer[pIPCBuffer->head] == pIPCBuffer->pRingBuffer[pIPCBuffer->tail])
        return 1;

    FbrIPCType type = pIPCBuffer->pRingBuffer[pIPCBuffer->tail];
    FBR_IPC_TARGET_NAMESPACE namespace = pIPCBuffer->pRingBuffer[pIPCBuffer->tail + 1];
    FBR_IPC_TARGET_METHOD method = pIPCBuffer->pRingBuffer[pIPCBuffer->tail + 2];
    FBR_IPC_TARGET_METHOD_PARAM methodParam = pIPCBuffer->pRingBuffer[pIPCBuffer->tail + 3];
    void* ptr;
    memcpy(&ptr, pIPCBuffer->pRingBuffer + 4, sizeof(void*));
    *pPtr = ptr;

    pIPCBuffer->tail = FBR_IPC_HEADER_SIZE + sizeof(void*);
    return 0;
}

int fbrIPCEnquePtr(FbrIPCBuffer *pIPCBuffer, void *ptr){
    pIPCBuffer->pRingBuffer[pIPCBuffer->head] = FBR_IPC_TYPE_PTR;
    pIPCBuffer->pRingBuffer[pIPCBuffer->head + 1] = 0;
    pIPCBuffer->pRingBuffer[pIPCBuffer->head + 2] = 0;
    pIPCBuffer->pRingBuffer[pIPCBuffer->head + 3] = 0;
    memcpy(pIPCBuffer->pRingBuffer + 4, &ptr, sizeof(void*));

    pIPCBuffer->head = FBR_IPC_HEADER_SIZE + sizeof(void*);
    return 0;
}

int fbrCreateProducerIPC(FbrIPC **ppAllocIPC) {
    FBR_LOG_DEBUG("Creating Producer IPC", FBR_IPC_BUFFER_SIZE, sizeof(FbrIPCBufferElement));

    HANDLE hMapFile;
    LPVOID pBuf;

    hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // use paging file
            NULL,                    // default security
            PAGE_READWRITE,          // read/write access
            0,                       // maximum object size (high-order DWORD)
            FBR_IPC_BUFFER_SIZE,                // maximum object size (low-order DWORD)
            sharedMemoryName);                 // name of mapping object

    if (hMapFile == NULL) {
        FBR_LOG_DEBUG("Could not create file mapping object (%lu)", GetLastError());
        return 1;
    }
    pBuf = MapViewOfFile(hMapFile,
                         FILE_MAP_ALL_ACCESS,
                         0,
                         0,
                         FBR_IPC_BUFFER_SIZE);

    memset(pBuf, 0, FBR_IPC_BUFFER_SIZE);

    if (pBuf == NULL) {
        printf(TEXT("Could not map view of file (%lu).\n"), GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    *ppAllocIPC = calloc(1, sizeof(FbrIPC));
    FbrIPC *pIPC = *ppAllocIPC;

    pIPC->hMapFile = hMapFile;
    pIPC->pIPCBuffer = pBuf;

    return 0;
}

int fbrCreateReceiverIPC(FbrIPC **ppAllocIPC) {
    HANDLE hMapFile;
    LPVOID pBuf;

    hMapFile = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,   // read/write access
            FALSE,                 // do not inherit the name
            sharedMemoryName);               // name of mapping object

    if (hMapFile == NULL) {
        FBR_LOG_DEBUG("Could not create file mapping object", GetLastError());
        return 1;
    }

    pBuf = MapViewOfFile(hMapFile,
                                  FILE_MAP_ALL_ACCESS,
                                  0,
                                  0,
                         FBR_IPC_BUFFER_SIZE);

    if (pBuf == NULL) {
        FBR_LOG_DEBUG("Could not map view of file", GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    *ppAllocIPC = calloc(1, sizeof(FbrIPC));
    FbrIPC *pIPC = *ppAllocIPC;

    pIPC->hMapFile = hMapFile;
    pIPC->pIPCBuffer = pBuf;

    return 0;
}

int fbrDestroyIPC(FbrIPC *pIPC) {
    UnmapViewOfFile(pIPC->pIPCBuffer);
    CloseHandle(pIPC->hMapFile);
    free(pIPC);

    return 0;
}