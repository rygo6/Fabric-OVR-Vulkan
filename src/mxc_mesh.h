#ifndef MOXAIC_MESH_H
#define MOXAIC_MESH_H

#include <vulkan/vulkan.h>
#include "cglm/cglm.h"

#include "mxc_transform.h"
#include "mxc_app.h"

typedef struct Vertex {
    vec2 pos;
    vec3 color;
} Vertex;

typedef struct MxcMeshState {
    MxcEntityState entityState;
    MxcTransformState transformState;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

} MxcMeshState;

#define MXC_ATTRIBUTE_DESCRIPTION_COUNT 2
#define MXC_TEST_INDICES_COUNT 6
#define MXC_TEST_VERTICES_COUNT 4

VkVertexInputBindingDescription getBindingDescription();

void getAttributeDescriptions(VkVertexInputAttributeDescription attributeDescriptions[MXC_ATTRIBUTE_DESCRIPTION_COUNT]);

void mxcAllocMesh(const MxcAppState* pAppState, MxcMeshState **ppAllocMeshState);

void mxcFreeMesh(const MxcAppState* pAppState, MxcMeshState *pMeshState);

#endif //MOXAIC_MESH_H
