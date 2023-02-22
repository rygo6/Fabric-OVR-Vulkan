#ifndef FABRIC_MESH_H
#define FABRIC_MESH_H

#include "fbr_transform.h"
#include "fbr_app.h"

typedef struct Vertex {
    vec2 pos;
    vec3 color;
    vec2 texCoord;
} Vertex;

typedef struct FbrMesh {
    FbrTransform transform;

    uint32_t indexCount;
    uint32_t vertexCount;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

} FbrMesh;

void fbrCreateMesh(const FbrVulkan *pVulkan, FbrMesh **ppAllocMeshState);

void fbrCleanupMesh(const FbrVulkan *pVulkan, FbrMesh *pMeshState);

#endif //FABRIC_MESH_H
