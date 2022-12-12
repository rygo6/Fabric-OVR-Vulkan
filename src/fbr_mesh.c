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

static void createPopulateBufferViaStaging(const FbrAppState *restrict pAppState,
                                           const void *restrict srcData,
                                           VkBufferUsageFlagBits usage,
                                           VkBuffer *restrict buffer,
                                           VkDeviceMemory *restrict bufferMemory,
                                           VkDeviceSize bufferSize) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    fbrCreateBuffer(pAppState,
                    bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &stagingBuffer,
                    &stagingBufferMemory);

    void* dstData;
    vkMapMemory(pAppState->device, stagingBufferMemory, 0, bufferSize, 0, &dstData);
    memcpy(dstData, srcData,bufferSize);
    vkUnmapMemory(pAppState->device, stagingBufferMemory);

    fbrCreateBuffer(pAppState,
                    bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    buffer,
                    bufferMemory);

    fbrCopyBuffer(pAppState, stagingBuffer, *buffer, bufferSize);

    vkDestroyBuffer(pAppState->device, stagingBuffer, NULL);
    vkFreeMemory(pAppState->device, stagingBufferMemory, NULL);
}

static void createVertexBuffer(const FbrAppState *restrict pAppState, FbrMeshState *restrict pMeshState) {
    VkDeviceSize bufferSize = (sizeof(Vertex) * FBR_TEST_VERTICES_COUNT);
    createPopulateBufferViaStaging(pAppState,
                                   vertices,
                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   &pMeshState->vertexBuffer,
                                   &pMeshState->vertexBufferMemory,
                                   bufferSize);
}

static void createIndexBuffer(const FbrAppState *restrict pAppState, FbrMeshState *restrict pMeshState) {
    VkDeviceSize bufferSize = (sizeof(uint16_t) * FBR_TEST_INDICES_COUNT);
    createPopulateBufferViaStaging(pAppState,
                                   indices,
                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                   &pMeshState->indexBuffer,
                                   &pMeshState->indexBufferMemory,
                                   bufferSize);
}

void fbrMeshUpdateCameraUBO(FbrMeshState *restrict pMeshState, FbrCameraState *restrict pCameraState) {
//    vec3 add = {.0001f,0,0,};
//    glm_vec3_add(pMeshState->transformState.pos, add, pMeshState->transformState.pos);

    fbrUpdateTransformMatrix(&pMeshState->transformState);
    glm_mat4_copy(pMeshState->transformState.matrix, pCameraState->mvp.model);

    // TODO this is getting copied multiple places.. in fbr_camera.c too
    memcpy(pCameraState->mvpUBO.pUniformBufferMapped, &pCameraState->mvp, sizeof(FbrMVP));
}


void fbrCreateMesh(const FbrAppState *restrict pAppState, FbrMeshState **restrict ppAllocMeshState) {
    *ppAllocMeshState = calloc(1, sizeof(FbrMeshState));
    FbrMeshState* pMeshState = *ppAllocMeshState;

    createVertexBuffer(pAppState, pMeshState);
    createIndexBuffer(pAppState, pMeshState);
}

void fbrFreeMesh(const FbrAppState *restrict pAppState, FbrMeshState *restrict pMeshState) {
    vkDestroyBuffer(pAppState->device, pMeshState->indexBuffer, NULL);
    vkFreeMemory(pAppState->device, pMeshState->indexBufferMemory, NULL);

    vkDestroyBuffer(pAppState->device, pMeshState->vertexBuffer, NULL);
    vkFreeMemory(pAppState->device, pMeshState->vertexBufferMemory, NULL);

    free(pMeshState);
}