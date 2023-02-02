#include <assert.h>
#include "fbr_ipc.h"
#include "fbr_log.h"
#include "fbr_ipc_targets.h"

const char sharedMemoryName[] = "FbrIPC";

int fbrIPCPollDeque(FbrApp *pApp, FbrIPC *pIPC) {
    // TODO this needs to actually cycle around the ring buffer, this is only half done

    FbrIPCBuffer *pIPCBuffer = pIPC->pIPCBuffer;

    if (pIPCBuffer->head == pIPCBuffer->tail)
        return 1;

    FBR_LOG_DEBUG("IPC Polling.", pIPCBuffer->head, pIPCBuffer->tail);

    FbrIPCTargetType target = pIPCBuffer->pRingBuffer[pIPCBuffer->tail];

    // TODO do you copy it out of the IPC or just send that chunk of shared memory on through?
    // If consumer consumes too slow then producer might run out of data in a stream?
    // From trusted parent app sending shared memory through is probably fine
//    void *param = malloc(fbrIPCTargetParamSize(target));
//    memcpy(param, pIPCBuffer->pRingBuffer + pIPCBuffer->tail + FBR_IPC_HEADER_SIZE, fbrIPCTargetParamSize(target));
    void *param = pIPCBuffer->pRingBuffer + pIPCBuffer->tail + FBR_IPC_HEADER_SIZE;

    if (pIPCBuffer->tail + FBR_IPC_HEADER_SIZE + fbrIPCTargetParamSize(target) > FBR_IPC_BUFFER_SIZE){
        FBR_LOG_ERROR("IPC BYTE ARRAY REACHED END!!!");
    }

    pIPC->pTargetFuncs[target](pApp, param);

    pIPCBuffer->tail = pIPCBuffer->tail + FBR_IPC_HEADER_SIZE + fbrIPCTargetParamSize(target);

    return 0;
}

void fbrIPCEnque(FbrIPC *pIPC, FbrIPCTargetType target, void *param) {
    FbrIPCBuffer *pIPCBuffer = pIPC->pIPCBuffer;
    pIPCBuffer->pRingBuffer[pIPCBuffer->head] = target;
    memcpy(pIPCBuffer->pRingBuffer + pIPCBuffer->head + FBR_IPC_HEADER_SIZE, param, fbrIPCTargetParamSize(target));
    pIPCBuffer->head = pIPCBuffer->head + FBR_IPC_HEADER_SIZE + fbrIPCTargetParamSize(target);
}

int fbrCreateProducerIPC(FbrIPC **ppAllocIPC) {
    FBR_LOG_DEBUG("Creating Producer IPC", FBR_IPC_BUFFER_SIZE, FBR_IPC_BUFFER_COUNT);

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

    fbrIPCSetTargets(pIPC);

    return 0;
}

int fbrDestroyIPC(FbrIPC *pIPC) {
    UnmapViewOfFile(pIPC->pIPCBuffer);
    CloseHandle(pIPC->hMapFile);
    free(pIPC);

    return 0;
}