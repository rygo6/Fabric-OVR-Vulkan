#include "fbr_node.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"
#include "fbr_process.h"
#include "fbr_framebuffer.h"
#include "fbr_ipc.h"
#include "fbr_camera.h"

#define FBR_NODE_INDEX_COUNT 6

const Vertex nodeVertices[FBR_NODE_VERTEX_COUNT] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f,  -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f,  0.5f, 0.0f},  {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f},  {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};
const uint16_t nodeIndices[] = {
        0, 1, 2, 2, 3, 0
};
const VkDeviceSize vertexBufferSize = (sizeof(Vertex) * FBR_NODE_VERTEX_COUNT);

void fbrUpdateNodeMesh(const FbrVulkan *pVulkan, FbrCamera *pCamera, FbrNode *pNode) {
//    glm_quat_copy(pCamera->transform.rot, pNode->transform.rot);
    fbrUpdateTransformMatrix(&pNode->transform);

    mat4 viewProj;
    glm_mat4_mul(pCamera->uboData.proj, pCamera->uboData.view, viewProj);
    mat4 mvp;
    glm_mat4_mul(viewProj, pNode->transform.matrix, mvp);

    vec4 viewport = {0.0f, 0.0f, 1.0f, 1.0f};

    vec3 up = {0.0f, 1.0f, 0.0f};
    glm_quat_rotatev(pCamera->transform.rot, up, up);
    glm_vec3_scale(up, 0.5f, up);

    vec3 right = {1.0f, 0.0f, 0.0f};
    glm_quat_rotatev(pCamera->transform.rot, right, right);
    glm_vec3_scale(right, 0.5f, right);

    vec3 ll;
    glm_vec3_sub(GLM_VEC3_ZERO, up,ll);
    glm_vec3_sub(ll, right,ll);
    glm_vec3_copy(ll, pNode->nodeVertices[0].pos);
    vec3 llScreen;
    glm_project(ll, mvp, viewport, llScreen);
    glm_vec2_copy(llScreen, pNode->nodeVertices[0].texCoord);

    vec3 lr;
    glm_vec3_sub(GLM_VEC3_ZERO, up,lr);
    glm_vec3_add(lr, right,lr);
    glm_vec3_copy(lr, pNode->nodeVertices[1].pos);
    vec3 lrScreen;
    glm_project(lr, mvp, viewport, lrScreen);
    glm_vec2_copy(lrScreen, pNode->nodeVertices[1].texCoord);

    vec3 ur;
    glm_vec3_add(GLM_VEC3_ZERO, up,ur);
    glm_vec3_add(ur, right,ur);
    glm_vec3_copy(ur, pNode->nodeVertices[2].pos);
    vec3 urScreen;
    glm_project(ur, mvp, viewport, urScreen);
    glm_vec2_copy(urScreen, pNode->nodeVertices[2].texCoord);

    vec3 ul;
    glm_vec3_add(GLM_VEC3_ZERO, up,ul);
    glm_vec3_sub(ul, right,ul);
    glm_vec3_copy(ul, pNode->nodeVertices[3].pos);
    vec3 ulScreen;
    glm_project(ul, mvp, viewport, ulScreen);
    glm_vec2_copy(ulScreen, pNode->nodeVertices[3].texCoord);

    memcpy(pNode->pVertexMappedBuffer, pNode->nodeVertices, vertexBufferSize);
}

void fbrCreateNode(const FbrApp *pApp, const char *pName, FbrNode **ppAllocNode) {
    *ppAllocNode = calloc(1, sizeof(FbrNode));
    FbrNode *pNode = *ppAllocNode;
    pNode->pName = strdup(pName);
    pNode->radius = 1.0f;

    fbrInitTransform(&pNode->transform);

    FbrVulkan *pVulkan = pApp->pVulkan;

    fbrCreateProcess(&pNode->pProcess);

    if (fbrCreateProducerIPC(&pNode->pProducerIPC) != 0){
        FBR_LOG_ERROR("fbrCreateProducerIPC fail");
        return;
    }
    // todo create receiverIPC

    fbrCreateExternalFrameBuffer(pApp->pVulkan, &pNode->pFramebuffer, pApp->pVulkan->swapExtent);

    pNode->vertexCount = FBR_NODE_VERTEX_COUNT;
    fbrCreateBuffer(pVulkan,
                    vertexBufferSize,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // host cached too?
                    &pNode->vertexBuffer,
                    &pNode->vertexBufferMemory);
    vkMapMemory(pVulkan->device, pNode->vertexBufferMemory, 0, vertexBufferSize, 0, &pNode->pVertexMappedBuffer);
    memcpy(pNode->nodeVertices, nodeVertices, sizeof(nodeVertices));
    memcpy(pNode->pVertexMappedBuffer, pNode->nodeVertices, vertexBufferSize);

    pNode->indexCount = FBR_NODE_INDEX_COUNT;
    VkDeviceSize indexBufferSize = (sizeof(Vertex) * pNode->indexCount);
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
