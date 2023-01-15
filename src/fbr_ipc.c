#include <assert.h>
#include <stdbool.h>
#include "fbr_ipc.h"
#include "fbr_log.h"

const char sharedMemoryName[] = "FbrIPC";

bool fbrIPCDequeAvailable(const FbrIPC *pIPC) {
    return pIPC->pBuffer->pRingBuffer->type != FBR_IPC_TYPE_NONE;
}

int fbrIPCDeque(FbrIPCBuffer *pBuffer, void **pPtr) {
    while (pBuffer->pRingBuffer[pBuffer->tail].type != FBR_IPC_TYPE_NONE) {
        FbrIPCBufferElement buffer;
        memcpy(&buffer, pBuffer + pBuffer->tail, sizeof(FbrIPCBufferElement));
    }

    return 0;
}

int fbrIPCDequePtr(const FbrIPC *pIPC, void **pPtr) {
    FbrIPCBufferElement buffer;
    memcpy(&buffer, pIPC->pBuffer, sizeof(FbrIPCBufferElement));
    *pPtr = buffer.ptrValue;
    return 0;
}

int fbrIPCEnquePtr(const FbrIPC *pIPC, void *ptr){
    FbrIPCBufferElement buffer = {
            .type = FBR_IPC_TYPE_PTR,
            .ptrValue = ptr
    };
    memcpy(pIPC->pBuffer, &buffer, sizeof(FbrIPCBufferElement));
    return 0;
}

int fbrIPCEnqueInt(const FbrIPC *pIPC, int intValue){
    FbrIPCBufferElement buffer = {
      .type = FBR_IPC_TYPE_INT,
      .inValue = intValue
    };
    memcpy(pIPC->pBuffer, &buffer, sizeof(int));
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
    pIPC->pBuffer = pBuf;

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
    pIPC->pBuffer = pBuf;

    return 0;
}

int fbrDestroyIPC(FbrIPC *pIPC) {
    UnmapViewOfFile(pIPC->pBuffer);
    CloseHandle(pIPC->hMapFile);
    free(pIPC);

    return 0;
}