#ifndef FABRIC_NODE_H
#define FABRIC_NODE_H

#include "fbr_app.h"
#include "fbr_transform.h"
#include "fbr_mesh.h"

#define FBR_NODE_VERTEX_COUNT 4

typedef struct FbrNodeVertex {
    vec2 pos;
    vec2 texCoord;
} FbrNodeVertex;

typedef struct FbrNode {
    FbrTransform transform;

    char *pName;

    float radius;

    FbrProcess *pProcess;

    FbrFramebuffer *pFramebuffer;

    FbrIPC *pProducerIPC;
    FbrIPC *pReceiverIPC;

    uint32_t indexCount;
    uint32_t vertexCount;
    void *pIndexMappedBuffer;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    Vertex nodeVertices[FBR_NODE_VERTEX_COUNT];
    void *pVertexMappedBuffer;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

} FbrNode;

void fbrUpdateNodeMesh(const FbrVulkan *pVulkan, FbrCamera *pCamera, FbrNode *pNode);

void fbrCreateNode(const FbrApp *pApp, const char *pName, FbrNode **ppAllocNode);

void fbrDestroyNode(const FbrVulkan *pVulkan, FbrNode *pNode);

#endif //FABRIC_NODE_H
