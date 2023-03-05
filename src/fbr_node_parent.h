#ifndef FABRIC_NODE_PARENT_H
#define FABRIC_NODE_PARENT_H

#include "fbr_app.h"
#include <windows.h>

typedef struct FbrNodeParent {
    FbrIPC *pReceiverIPC;

    FbrFramebuffer *pFramebuffer;
    FbrTimelineSemaphore *pParentSemaphore;
    FbrTimelineSemaphore *pChildSemaphore;
    FbrCamera *pCamera;

    VkDescriptorSet parentFramebufferDescriptorSet; // todo this doesnt belong here
} FbrNodeParent;

void fbrCreateNodeParent(FbrNodeParent **ppAllocNodeParent);

void fbrDestroyNodeParent(FbrVulkan *pVulkan, FbrNodeParent *pNodeParent);

// IPC

typedef struct FbrIPCParamImportNodeParent {
    uint16_t framebufferWidth;
    uint16_t framebufferHeight;
    HANDLE framebufferExternalHandle;
    HANDLE cameraExternalHandle;
    HANDLE parentSemaphoreExternalHandle;
    HANDLE childSemaphoreExternalHandle;
} FbrIPCParamImportNodeParent;

void fbrIPCTargetImportNodeParent(FbrApp *pApp, FbrIPCParamImportNodeParent *pParam);

#endif //FABRIC_NODE_PARENT_H
