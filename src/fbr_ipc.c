#include <assert.h>
#include <stdbool.h>
#include "fbr_ipc.h"
#include "fbr_log.h"

#include "fbr_mesh.h"
#include "fbr_texture.h"
#include "fbr_pipeline.h"
#include "fbr_vulkan.h"

const char sharedMemoryName[] = "FbrIPC";

const int FbrIPCTargetParamSize[] = {
        sizeof(FBR_IPC_TARGET_SIGNATURE(0)),
};

static void externalTextureTarget(FbrApp *pApp, FbrIPCExternalTextureParam *pParam){

    printf("external texture handle d %lld\n", pParam->handle);
    printf("external texture handle p %p\n", pParam->handle);

    fbrCreateMesh(pApp->pVulkan, &pApp->pMesh);
    fbrImportTexture(pApp->pVulkan, &pApp->pTexture, pParam->handle, pParam->width, pParam->height);
    fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTexture->imageView, pApp->pVulkan->renderPass, &pApp->pPipeline);

    fbrCreateMesh(pApp->pVulkan, &pApp->pMeshExternalTest);
    fbrImportTexture(pApp->pVulkan, &pApp->pTextureExternalTest, pParam->handle, pParam->width, pParam->height);
    glm_vec3_add(pApp->pMeshExternalTest->transform.pos, (vec3) {1,0,0}, pApp->pMeshExternalTest->transform.pos);
    fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTextureExternalTest->imageView, pApp->pVulkan->renderPass, &pApp->pPipelineExternalTest);
}

int fbrIPCPollDeque(FbrApp *pApp, FbrIPC *pIPC) {
    FbrIPCBuffer *pIPCBuffer = pIPC->pIPCBuffer;

    if (pIPCBuffer->head == pIPCBuffer->tail)
        return 1;

    FbrIPCTargetType target = pIPCBuffer->pRingBuffer[pIPCBuffer->tail];

    void *param;
    memcpy(param, pIPCBuffer->pRingBuffer + pIPCBuffer->tail + FBR_IPC_HEADER_SIZE, FbrIPCTargetParamSize[target]);
    pIPC->pTargetFuncs[FBR_IPC_TARGET_EXTERNAL_TEXTURE](pApp, param);

    pIPCBuffer->tail = pIPCBuffer->tail + FBR_IPC_HEADER_SIZE + FbrIPCTargetParamSize[target];;

    return 0;
}

void fbrIPCEnque(FbrIPCBuffer *pIPCBuffer, FbrIPCTargetType target, void *param) {
    pIPCBuffer->pRingBuffer[pIPCBuffer->head] = target;
    memcpy(pIPCBuffer->pRingBuffer + pIPCBuffer->head + FBR_IPC_HEADER_SIZE, param, FbrIPCTargetParamSize[target]);
    pIPCBuffer->head = pIPCBuffer->head + FBR_IPC_HEADER_SIZE + FbrIPCTargetParamSize[target];;
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

    pIPC->pTargetFuncs[FBR_IPC_TARGET_EXTERNAL_TEXTURE] = (void (*)(FbrApp *, void *)) externalTextureTarget;

    return 0;
}

int fbrDestroyIPC(FbrIPC *pIPC) {
    UnmapViewOfFile(pIPC->pIPCBuffer);
    CloseHandle(pIPC->hMapFile);
    free(pIPC);

    return 0;
}