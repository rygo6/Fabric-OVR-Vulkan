#include "fbr_mesh.h"
#include "fbr_app.h"
#include "fbr_buffer.h"
#include "fbr_camera.h"

#include <memory.h>

const Vertex vertices[] = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const uint16_t indices[] = {
        0, 1, 2, 2, 3, 0
};

static void createVertexBuffer(const FbrAppState* pState, FbrMeshState *pMeshState) {
    VkDeviceSize bufferSize = (sizeof(Vertex) * FBR_TEST_VERTICES_COUNT);
    createBuffer(pState,
                 bufferSize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &pMeshState->vertexBuffer,
                 &pMeshState->vertexBufferMemory);

    void* data;
    vkMapMemory(pState->device, pMeshState->vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices,bufferSize);
    vkUnmapMemory(pState->device, pMeshState->vertexBufferMemory);
}

static void createIndexBuffer(const FbrAppState* pState, FbrMeshState *pMeshState) {
    VkDeviceSize bufferSize = (sizeof(uint16_t) * FBR_TEST_INDICES_COUNT);
    createBuffer(pState,
                 bufferSize,
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &pMeshState->indexBuffer,
                 &pMeshState->indexBufferMemory);

    void* data;
    vkMapMemory(pState->device, pMeshState->indexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices,bufferSize);
    vkUnmapMemory(pState->device, pMeshState->indexBufferMemory);
}

VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    return bindingDescription;
}

void fbrMeshUpdateCameraUBO(FbrMeshState *pMeshState, FbrCameraState *pCameraState) {
//    vec3 add = {.0001f,0,0,};
//    glm_vec3_add(pMeshState->transformState.pos, add, pMeshState->transformState.pos);

    fbrUpdateTransformMatrix(&pMeshState->transformState);
    glm_mat4_copy(pMeshState->transformState.matrix, pCameraState->mvp.model);

    // TODO this is getting copied multiple places.. in fbr_camera.c too
    memcpy(pCameraState->mvpUBO.pUniformBufferMapped, &pCameraState->mvp, sizeof(FbrMVP));
}


void fbrCreateMesh(const FbrAppState* pAppState, FbrMeshState **ppAllocMeshState) {
    *ppAllocMeshState = malloc(sizeof(FbrMeshState));
    FbrMeshState* pMeshState = *ppAllocMeshState;

    createVertexBuffer(pAppState, pMeshState);
    createIndexBuffer(pAppState, pMeshState);
}

void fbrFreeMesh(const FbrAppState* pAppState, FbrMeshState *pMeshState) {
    vkDestroyBuffer(pAppState->device, pMeshState->indexBuffer, NULL);
    vkFreeMemory(pAppState->device, pMeshState->indexBufferMemory, NULL);

    vkDestroyBuffer(pAppState->device, pMeshState->vertexBuffer, NULL);
    vkFreeMemory(pAppState->device, pMeshState->vertexBufferMemory, NULL);

    free(pMeshState);
}