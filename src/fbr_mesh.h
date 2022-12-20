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
    FbrEntity entity;
    FbrTransform transform;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

} FbrMesh;

#define FBR_TEST_INDICES_COUNT 6
#define FBR_TEST_VERTICES_COUNT 4

void fbrMeshUpdateCameraUBO(FbrMesh *pMeshState, FbrCamera *pCameraState);

void fbrCreateMesh(const FbrVulkan *pVulkan, FbrMesh **ppAllocMeshState);

void fbrCleanupMesh(const FbrVulkan *pVulkan, FbrMesh *pMeshState);

#endif //FABRIC_MESH_H
