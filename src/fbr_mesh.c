#include "fbr_mesh.h"
#include "fbr_app.h"
#include "fbr_buffer.h"
#include "fbr_camera.h"
#include "fbr_vulkan.h"

#include <memory.h>

const Vertex vertices[] = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f,  -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f,  0.5f},  {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const uint16_t indices[] = {
        0, 1, 2, 2, 3, 0
};

static void createVertexBuffer(const FbrVulkan *pVulkan, FbrMesh *pMeshState) {
    VkDeviceSize bufferSize = (sizeof(Vertex) * FBR_TEST_VERTICES_COUNT);
    fbrCreatePopulateBufferViaStaging(pVulkan,
                                      vertices,
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      &pMeshState->vertexBuffer,
                                      &pMeshState->vertexBufferMemory,
                                      bufferSize);
}

static void createIndexBuffer(const FbrVulkan *pVulkan, FbrMesh *pMeshState) {
    VkDeviceSize bufferSize = (sizeof(uint16_t) * FBR_TEST_INDICES_COUNT);
    fbrCreatePopulateBufferViaStaging(pVulkan,
                                      indices,
                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                      &pMeshState->indexBuffer,
                                      &pMeshState->indexBufferMemory,
                                      bufferSize);
}

void fbrMeshUpdateCameraUBO(FbrMesh *pMeshState, FbrCamera *pCameraState) {
//    vec3 add = {.0001f,0,0,};
//    glm_vec3_add(pMeshState->transformState.pos, add, pMeshState->transformState.pos);

    fbrUpdateTransformMatrix(&pMeshState->transform);
    glm_mat4_copy(pMeshState->transform.matrix, pCameraState->mvp.model);

    // TODO this is getting copied multiple places.. in fbr_camera.c too
    memcpy(pCameraState->mvpUBO.pUniformBufferMapped, &pCameraState->mvp, sizeof(FbrMVP));
}


void fbrCreateMesh(const FbrVulkan *pVulkan, FbrMesh **ppAllocMeshState) {
    *ppAllocMeshState = calloc(1, sizeof(FbrMesh));
    FbrMesh *pMeshState = *ppAllocMeshState;
    createVertexBuffer(pVulkan, pMeshState);
    createIndexBuffer(pVulkan, pMeshState);
}

void fbrCleanupMesh(const FbrVulkan *pVulkan, FbrMesh *pMeshState) {
    vkDestroyBuffer(pVulkan->device, pMeshState->indexBuffer, NULL);
    vkFreeMemory(pVulkan->device, pMeshState->indexBufferMemory, NULL);
    vkDestroyBuffer(pVulkan->device, pMeshState->vertexBuffer, NULL);
    vkFreeMemory(pVulkan->device, pMeshState->vertexBufferMemory, NULL);
    free(pMeshState);
}