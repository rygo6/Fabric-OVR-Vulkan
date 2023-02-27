#include "fbr_mesh.h"
#include "fbr_app.h"
#include "fbr_buffer.h"
#include "fbr_vulkan.h"

const Vertex vertices[4] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f,  -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f,  0.5f, 0.0f},  {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f},  {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const uint16_t indices[6] = {
        0, 1, 2, 2, 3, 0
};

static void createVertexBuffer(const FbrVulkan *pVulkan, FbrMesh *pMeshState) {
    pMeshState->vertexCount = 4;
    VkDeviceSize bufferSize = (sizeof(Vertex) * pMeshState->vertexCount);
    fbrCreatePopulateBufferViaStaging(pVulkan,
                                      vertices,
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      &pMeshState->vertexBuffer,
                                      &pMeshState->vertexBufferMemory,
                                      bufferSize);
}

static void createIndexBuffer(const FbrVulkan *pVulkan, FbrMesh *pMeshState) {
    pMeshState->indexCount = 6;
    VkDeviceSize bufferSize = (sizeof(uint16_t) * pMeshState->indexCount);
    fbrCreatePopulateBufferViaStaging(pVulkan,
                                      indices,
                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                      &pMeshState->indexBuffer,
                                      &pMeshState->indexBufferMemory,
                                      bufferSize);
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