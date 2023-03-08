#ifndef FABRIC_NODE_H
#define FABRIC_NODE_H

#include "fbr_app.h"
#include "fbr_transform.h"
#include "fbr_mesh.h"
#include "fbr_framebuffer.h"
#include "fbr_buffer.h"

#define FBR_NODE_VERTEX_COUNT 4
#define FBR_NODE_VERTEX_BUFFER_SIZE (sizeof(Vertex) * FBR_NODE_VERTEX_COUNT)
#define FBR_NODE_INDEX_COUNT 6
#define FBR_NODE_INDEX_BUFFER_SIZE (sizeof(uint16_t) * FBR_NODE_INDEX_COUNT)
#define FBR_NODE_FRAMEBUFFER_COUNT 2

typedef struct FbrNodeVertex {
    vec2 pos;
    vec2 texCoord;
} FbrNodeVertex;

typedef struct FbrNode {
    FbrTransform transform;

    char *pName;

    float radius;

    FbrProcess *pProcess;

    FbrIPC *pProducerIPC;
    FbrIPC *pReceiverIPC;

    FbrTimelineSemaphore *pChildSemaphore;

    FbrFramebuffer *pFramebuffers[FBR_NODE_FRAMEBUFFER_COUNT];
    FbrUniformBufferObject *pVertexUBOs[FBR_NODE_FRAMEBUFFER_COUNT];

    FbrUniformBufferObject *pIndexUBO;

    Vertex nodeVerticesBuffer[FBR_NODE_VERTEX_COUNT];

} FbrNode;

void fbrUpdateNodeMesh(const FbrVulkan *pVulkan, FbrCamera *pCamera, FbrNode *pNode);

VkResult fbrCreateNode(const FbrApp *pApp, const char *pName, FbrNode **ppAllocNode);

void fbrDestroyNode(const FbrVulkan *pVulkan, FbrNode *pNode);

#endif //FABRIC_NODE_H
