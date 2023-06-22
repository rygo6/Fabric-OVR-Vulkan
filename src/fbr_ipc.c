#include <assert.h>
#include "fbr_ipc.h"
#include "fbr_log.h"
#include "fbr_ipc_targets.h"

const char sharedMemoryName[] = "FbrIPCRingBuffer";
const char sharedTempCamMemoryName[] = "FbrIPCCamera";

static int createIPCBuffer(int bufferSize, const char *pSharedMemoryName, HANDLE *phMapFile, void **pBuffer)
{
    HANDLE hMapFile;
    LPVOID pBuf;

    hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // use paging file
            NULL,                    // default security
            PAGE_READWRITE,          // read/write access
            0,                       // maximum object size (high-order DWORD)
            bufferSize,                // maximum object size (low-order DWORD)
            pSharedMemoryName);                 // name of mapping object

    if (hMapFile == NULL) {
        FBR_LOG_DEBUG("Could not create file mapping object (%lu)", GetLastError());
        return 1;
    }
    pBuf = MapViewOfFile(hMapFile,
                         FILE_MAP_ALL_ACCESS,
                         0,
                         0,
                         bufferSize);

    memset(pBuf, 0, bufferSize);

    if (pBuf == NULL) {
        printf(TEXT("Could not map view of file (%lu).\n"), GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    *phMapFile = hMapFile;
    *pBuffer = pBuf;

    return 0;
}

int createImportIPCBuffer(int bufferSize, const char *pSharedMemoryName, HANDLE *phMapFile, void **pBuffer)
{
    HANDLE hMapFile;
    LPVOID pBuf;

    // need to using handle somehow!? phMapFile
    hMapFile = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,   // read/write access
            FALSE,                 // do not inherit the name
            pSharedMemoryName);               // name of mapping object

    if (hMapFile == NULL) {
        FBR_LOG_DEBUG("Could not create file mapping object", GetLastError());
        return 1;
    }

    pBuf = MapViewOfFile(hMapFile,
                         FILE_MAP_ALL_ACCESS,
                         0,
                         0,
                         bufferSize);

    if (pBuf == NULL) {
        FBR_LOG_DEBUG("Could not map view of file", GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    *phMapFile = hMapFile;
    *pBuffer = pBuf;

    return 0;
}

int fbrIPCPollDeque(FbrApp *pApp, FbrIPCRingBuffer *pIPC)
{
    // TODO this needs to actually cycle around the ring buffer, this is only half done

    FbrRingBuffer *pIPCBuffer = pIPC->pRingBuffer;

    if (pIPCBuffer->head == pIPCBuffer->tail)
        return 1;

    FBR_LOG_DEBUG("IPC Polling.", pIPCBuffer->head, pIPCBuffer->tail);

    FbrIPCTargetType target = pIPCBuffer->pRingBuffer[pIPCBuffer->tail];

    // TODO do you copy it out of the IPC or just send that chunk of shared memory on through?
    // If consumer consumes too slow then producer might run out of data in a stream?
    // From trusted parent app sending shared memory through is probably fine
//    void *param = malloc(fbrIPCTargetParamSize(target));
//    memcpy(param, pRingBuffer->pRingBuffer + pRingBuffer->tail + FBR_IPC_RING_HEADER_SIZE, fbrIPCTargetParamSize(target));
    void *param = pIPCBuffer->pRingBuffer + pIPCBuffer->tail + FBR_IPC_RING_HEADER_SIZE;

    if (pIPCBuffer->tail + FBR_IPC_RING_HEADER_SIZE + fbrIPCTargetParamSize(target) > FBR_IPC_RING_BUFFER_SIZE){
        FBR_LOG_ERROR("IPC BYTE ARRAY REACHED END!!!");
    }

    FBR_LOG_DEBUG("Calling IPC Target", target);
    pIPC->pTargetFuncs[target](pApp, param);

    pIPCBuffer->tail = pIPCBuffer->tail + FBR_IPC_RING_HEADER_SIZE + fbrIPCTargetParamSize(target);

    return 0;
}

void fbrIPCEnque(FbrIPCRingBuffer *pIPC, FbrIPCTargetType target, void *param)
{
    FbrRingBuffer *pIPCBuffer = pIPC->pRingBuffer;
    pIPCBuffer->pRingBuffer[pIPCBuffer->head] = target;
    memcpy(pIPCBuffer->pRingBuffer + pIPCBuffer->head + FBR_IPC_RING_HEADER_SIZE, param, fbrIPCTargetParamSize(target));
    pIPCBuffer->head = pIPCBuffer->head + FBR_IPC_RING_HEADER_SIZE + fbrIPCTargetParamSize(target);
}

int fbrCreateProducerIPCRingBuffer(FbrIPCRingBuffer **ppAllocIPC)
{
    FBR_LOG_DEBUG("Creating Producer IPC Ring", FBR_IPC_RING_BUFFER_SIZE, FBR_IPC_RING_BUFFER_COUNT);

    *ppAllocIPC = calloc(1, sizeof(FbrIPCRingBuffer));
    FbrIPCRingBuffer *pIPC = *ppAllocIPC;

    createIPCBuffer(FBR_IPC_RING_BUFFER_SIZE, sharedMemoryName, &pIPC->hMapFile, (void **) &pIPC->pRingBuffer);

    return 0;
}

int fbrCreateReceiverIPCRingBuffer(FbrIPCRingBuffer **ppAllocIPC)
{
    FBR_LOG_DEBUG("Creating Receiver IPC Ring", FBR_IPC_RING_BUFFER_SIZE, FBR_IPC_RING_BUFFER_COUNT);

    *ppAllocIPC = calloc(1, sizeof(FbrIPCRingBuffer));
    FbrIPCRingBuffer *pIPC = *ppAllocIPC;

    createImportIPCBuffer(FBR_IPC_RING_BUFFER_SIZE, sharedMemoryName, &pIPC->hMapFile, (void **) &pIPC->pRingBuffer);

    fbrIPCSetTargets(pIPC);

    return 0;
}

int fbrCreateIPCBuffer(FbrIPCBuffer **ppAllocIPC, int bufferSize)
{
    FBR_LOG_DEBUG("Creating Producer IPC", bufferSize);

    *ppAllocIPC = calloc(1, sizeof(FbrIPCRingBuffer));
    FbrIPCBuffer *pIPC = *ppAllocIPC;

    createIPCBuffer(bufferSize, sharedTempCamMemoryName, &pIPC->hMapFile, &pIPC->pBuffer);

    return 0;
}

int fbrImportIPCBuffer(FbrIPCBuffer **ppAllocIPC,  int bufferSize)
{
    FBR_LOG_DEBUG("Creating Receiver IPC", bufferSize);

    *ppAllocIPC = calloc(1, sizeof(FbrIPCRingBuffer));
    FbrIPCBuffer *pIPC = *ppAllocIPC;

    createImportIPCBuffer(bufferSize, sharedTempCamMemoryName, &pIPC->hMapFile, &pIPC->pBuffer);

    return 0;
}

void fbrDestroyIPCBuffer(FbrIPCBuffer *pIPC)
{
    UnmapViewOfFile(pIPC->pBuffer);
    CloseHandle(pIPC->hMapFile);
    free(pIPC);
}

void fbrDestroyIPCRingBuffer(FbrIPCRingBuffer *pIPC)
{
    UnmapViewOfFile(pIPC->pRingBuffer);
    CloseHandle(pIPC->hMapFile);
    free(pIPC);
}