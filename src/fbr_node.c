#include "fbr_node.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"
#include "fbr_mesh.h"
#include "fbr_process.h"
#include "fbr_framebuffer.h"
#include "fbr_ipc.h"
#include "fbr_camera.h"

const Vertex nodeVertices[] = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f,  -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f,  0.5f},  {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};
const uint16_t nodeIndices[] = {
        0, 1, 2, 2, 3, 0
};

void fbrUpdateNodeMesh(FbrNode *pNode, FbrCamera *pCamera) {
    glm_quat_copy(pCamera->transform.rot, pNode->transform.rot);
    fbrUpdateTransformMatrix(&pNode->transform);
}

void fbrCreateNode(const FbrApp *pApp, const char *pName, FbrNode **ppAllocNode) {
    *ppAllocNode = calloc(1, sizeof(FbrNode));
    FbrNode *pNode = *ppAllocNode;
    pNode->pName = strdup(pName);

    FbrVulkan *pVulkan = pApp->pVulkan;

    fbrCreateProcess(&pNode->pProcess);

    if (fbrCreateProducerIPC(&pNode->pProducerIPC) != 0){
        FBR_LOG_ERROR("fbrCreateProducerIPC fail");
        return;
    }
    // todo create receiverIPC

    fbrCreateExternalFrameBuffer(pApp->pVulkan, &pNode->pFramebuffer, pApp->pVulkan->swapExtent);

    pNode->vertexCount = 4;
    VkDeviceSize vertexBufferSize = (sizeof(Vertex) * pNode->vertexCount);
    fbrCreateBuffer(pVulkan,
                    vertexBufferSize,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // host cached too?
                    &pNode->vertexBuffer,
                    &pNode->vertexBufferMemory);
    vkMapMemory(pVulkan->device, pNode->vertexBufferMemory, 0, vertexBufferSize, 0, &pNode->pVertexMappedBuffer);
    memcpy(pNode->pVertexMappedBuffer, nodeVertices, vertexBufferSize);

    pNode->indexCount = 6;
    VkDeviceSize indexBufferSize = (sizeof(Vertex) * pNode->vertexCount);
    fbrCreateBuffer(pVulkan,
                    indexBufferSize,
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // host cached too?
                    &pNode->indexBuffer,
                    &pNode->indexBufferMemory);
    vkMapMemory(pVulkan->device, pNode->indexBufferMemory, 0, indexBufferSize, 0, &pNode->pIndexMappedBuffer);
    memcpy(pNode->pIndexMappedBuffer, nodeIndices, indexBufferSize);
}

void fbrDestroyNode(const FbrVulkan *pVulkan, FbrNode *pNode) {
    free(pNode->pName);

    vkUnmapMemory(pVulkan->device, pNode->pIndexMappedBuffer);
    vkDestroyBuffer(pVulkan->device, pNode->indexBuffer, NULL);
    vkFreeMemory(pVulkan->device, pNode->indexBufferMemory, NULL);

    vkUnmapMemory(pVulkan->device, pNode->pVertexMappedBuffer);
    vkDestroyBuffer(pVulkan->device, pNode->vertexBuffer, NULL);
    vkFreeMemory(pVulkan->device, pNode->vertexBufferMemory, NULL);

    fbrDestroyProcess(pNode->pProcess);

    fbrDestroyIPC(pNode->pProducerIPC);
    fbrDestroyIPC(pNode->pReceiverIPC);

    fbrDestroyFrameBuffer(pVulkan, pNode->pFramebuffer);

    free(pNode);
}
