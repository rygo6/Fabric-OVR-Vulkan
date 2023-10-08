#ifndef FABRIC_NODE_H
#define FABRIC_NODE_H

#include "fbr_app.h"
#include "fbr_transform.h"
#include "fbr_mesh.h"
#include "fbr_framebuffer.h"
#include "fbr_buffer.h"
#include "fbr_camera.h"

#define FBR_NODE_FRAMEBUFFER_COUNT 2

typedef struct FbrNodeCameraIPCBuffer {
    mat4 view;
    mat4 proj;
    mat4 invView;
    mat4 invProj;
    mat4 model;
    uint32_t width;
    uint32_t height;
} FbrNodeCameraIPCBuffer;

typedef struct FbrNode {
    FbrTransform *pTransform;

    FbrCamera *pCamera;

    char *pName;

    FbrProcess *pProcess;

    FbrIPCRingBuffer *pProducerIPC;
    FbrIPCRingBuffer *pReceiverIPC;

    FbrNodeCameraIPCBuffer *pRenderingNodeCameraIPCBuffer;
    FbrIPCBuffer *pCameraIPCBuffer;

    FbrTimelineSemaphore *pChildSemaphore;

    FbrFramebuffer *pFramebuffers[FBR_NODE_FRAMEBUFFER_COUNT];

} FbrNode;

void fbrUpdateNodeMesh(const FbrVulkan *pVulkan, FbrCameraBuffer camUboData, versor camRot, int timelineSwitch, FbrNode *pNode);

VkResult fbrCreateNode(const FbrApp *pApp, const char *pName, FbrNode **ppAllocNode);

void fbrDestroyNode(const FbrVulkan *pVulkan, FbrNode *pNode);

#endif //FABRIC_NODE_H
