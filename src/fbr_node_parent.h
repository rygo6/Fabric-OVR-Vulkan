#ifndef FABRIC_NODE_PARENT_H
#define FABRIC_NODE_PARENT_H

#include "fbr_app.h"
#include "fbr_node.h"
#include <windows.h>

typedef struct FbrNodeParent {
    FbrTransform *pTransform;

    FbrIPC *pReceiverIPC;

    FbrFramebuffer *pFramebuffers[FBR_NODE_FRAMEBUFFER_COUNT];
    FbrUniformBufferObject *pVertexUBOs[FBR_NODE_FRAMEBUFFER_COUNT];

    FbrTimelineSemaphore *pParentSemaphore;
    FbrTimelineSemaphore *pChildSemaphore;
    FbrCamera *pCamera;

    Vertex nodeVerticesBuffer[FBR_NODE_VERTEX_COUNT];

} FbrNodeParent;

void fbrUpdateNodeParentMesh(const FbrVulkan *pVulkan, FbrCamera *pCamera, int dynamicCameraIndex, int timelineSwitch, FbrNodeParent *pNode);

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
    HANDLE depthFramebuffer0ExternalHandle;
    HANDLE depthFramebuffer1ExternalHandle;
    HANDLE vertexUBO0ExternalHandle;
    HANDLE vertexUBO1ExternalHandle;
    HANDLE cameraExternalHandle;
    HANDLE parentSemaphoreExternalHandle;
    HANDLE childSemaphoreExternalHandle;
} FbrIPCParamImportNodeParent;

void fbrIPCTargetImportNodeParent(FbrApp *pApp, FbrIPCParamImportNodeParent *pParam);

#endif //FABRIC_NODE_PARENT_H
