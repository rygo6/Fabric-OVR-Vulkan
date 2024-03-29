#ifndef FABRIC_NODE_PARENT_H
#define FABRIC_NODE_PARENT_H

#include "fbr_app.h"
#include "fbr_node.h"
#include <windows.h>

typedef struct FbrNodeParent {
    FbrTransform *pTransform;

    FbrIPCRingBuffer *pReceiverIPC;

    FbrTimelineSemaphore *pParentSemaphore;
    FbrTimelineSemaphore *pChildSemaphore;

    FbrIPCBuffer *pCameraIPCBuffer;

} FbrNodeParent;

void fbrUpdateNodeParentMesh(const FbrVulkan *pVulkan, FbrCamera *pCamera, int timelineSwitch, FbrNodeParent *pNode);

void fbrCreateNodeParent(const FbrVulkan *pVulkan, FbrNodeParent **ppAllocNodeParent);

void fbrDestroyNodeParent(const FbrVulkan *pVulkan, FbrNodeParent *pNodeParent);

// IPC

typedef struct FbrIPCParamImportNodeParent {
    uint16_t framebufferWidth;
    uint16_t framebufferHeight;
    HANDLE colorFramebuffer0ExternalHandle;
    HANDLE colorFramebuffer1ExternalHandle;
    HANDLE normalFramebuffer0ExternalHandle;
    HANDLE normalFramebuffer1ExternalHandle;
    HANDLE gbufferFramebuffer0ExternalHandle;
    HANDLE gbufferFramebuffer1ExternalHandle;
    HANDLE depthFramebuffer0ExternalHandle;
    HANDLE depthFramebuffer1ExternalHandle;
    HANDLE parentSemaphoreExternalHandle;
    HANDLE childSemaphoreExternalHandle;
} FbrIPCParamImportNodeParent;

void fbrIPCTargetImportNodeParent(FbrApp *pApp, FbrIPCParamImportNodeParent *pParam);

#endif //FABRIC_NODE_PARENT_H
