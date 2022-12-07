#ifndef FABRIC_MESH_H
#define FABRIC_MESH_H

#include <vulkan/vulkan.h>
#include "cglm/cglm.h"

#include "fbr_transform.h"
#include "fbr_app.h"

typedef struct Vertex {
    vec2 pos;
    vec3 color;
} Vertex;

typedef struct FbrMeshState {
    FbrEntityState entityState;
    FbrTransformState transformState;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

} FbrMeshState;

#define FBR_ATTRIBUTE_DESCRIPTION_COUNT 2
#define FBR_TEST_INDICES_COUNT 6
#define FBR_TEST_VERTICES_COUNT 4

VkVertexInputBindingDescription getBindingDescription();

void getAttributeDescriptions(VkVertexInputAttributeDescription attributeDescriptions[FBR_ATTRIBUTE_DESCRIPTION_COUNT]);

void fbrMeshUpdateCameraUBO(FbrMeshState *pMeshState, FbrCameraState *pCameraState);

void fbrAllocMesh(const FbrAppState* pAppState, FbrMeshState **ppAllocMeshState);

void fbrFreeMesh(const FbrAppState* pAppState, FbrMeshState *pMeshState);

#endif //FABRIC_MESH_H
