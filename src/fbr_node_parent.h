#ifndef FABRIC_NODE_PARENT_H
#define FABRIC_NODE_PARENT_H

#include "fbr_app.h"
#include "fbr_node.h"
#include <windows.h>

typedef struct FbrNodeParent {
    FbrIPC *pReceiverIPC;

    FbrFramebuffer *pFramebuffer;

    FbrFramebuffer *pFramebuffers[FBR_NODE_FRAMEBUFFER_COUNT];
    FbrUniformBufferObject *pVertexUBOs[FBR_NODE_FRAMEBUFFER_COUNT];

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
    HANDLE framebuffer0ExternalHandle;
    HANDLE framebuffer1ExternalHandle;
    HANDLE vertexUBO0ExternalHandle;
    HANDLE vertexUBO1ExternalHandle;
    HANDLE cameraExternalHandle;
    HANDLE parentSemaphoreExternalHandle;
    HANDLE childSemaphoreExternalHandle;
} FbrIPCParamImportNodeParent;

void fbrIPCTargetImportNodeParent(FbrApp *pApp, FbrIPCParamImportNodeParent *pParam);

#endif //FABRIC_NODE_PARENT_H
