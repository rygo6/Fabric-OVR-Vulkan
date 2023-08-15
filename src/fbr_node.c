#include "fbr_node.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"
#include "fbr_process.h"
#include "fbr_ipc.h"
#include "fbr_swap.h"

const Vertex nodeVertices[FBR_NODE_VERTEX_COUNT] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f,  -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f,  0.5f, 0.0f},  {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f},  {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};
const uint16_t nodeIndices[] = {
        0, 1, 2, 3,
};

void fbrUpdateNodeMesh(const FbrVulkan *pVulkan, FbrCameraBuffer camUboData, versor camRot, int timelineSwitch, FbrNode *pNode) {
//    mat4 viewProj;
//    glm_mat4_mul(camUboData.proj, camUboData.view, viewProj);
//    mat4 mvp;
//    glm_mat4_mul(viewProj, pNode->pTransform->uboData.model, mvp);
//
//    vec4 viewport = {0.0f, 0.0f, 1.0f, 1.0f};
//
//    vec3 up = {0.0f, 1.0f, 0.0f};
//    glm_quat_rotatev(camRot, up, up);
//    glm_vec3_scale(up, 0.5f, up);
//
//    vec3 right = {1.0f, 0.0f, 0.0f};
//    glm_quat_rotatev(camRot, right, right);
//    glm_vec3_scale(right, 0.5f, right);
//
//    vec3 ll;
//    glm_vec3_sub(GLM_VEC3_ZERO, up,ll);
//    glm_vec3_sub(ll, right,ll);
//    glm_vec3_copy(ll, pNode->nodeVerticesBuffer[0].pos);
//    vec3 llScreen;
//    glm_project(ll, mvp, viewport, llScreen);
//    glm_vec2_copy(llScreen, pNode->nodeVerticesBuffer[0].uv);
//
//    vec3 lr;
//    glm_vec3_sub(GLM_VEC3_ZERO, up,lr);
//    glm_vec3_add(lr, right,lr);
//    glm_vec3_copy(lr, pNode->nodeVerticesBuffer[1].pos);
//    vec3 lrScreen;
//    glm_project(lr, mvp, viewport, lrScreen);
//    glm_vec2_copy(lrScreen, pNode->nodeVerticesBuffer[1].uv);
//
//    vec3 ur;
//    glm_vec3_add(GLM_VEC3_ZERO, up,ur);
//    glm_vec3_add(ur, right,ur);
//    glm_vec3_copy(ur, pNode->nodeVerticesBuffer[2].pos);
//    vec3 urScreen;
//    glm_project(ur, mvp, viewport, urScreen);
//    glm_vec2_copy(urScreen, pNode->nodeVerticesBuffer[2].uv);
//
//    vec3 ul;
//    glm_vec3_add(GLM_VEC3_ZERO, up,ul);
//    glm_vec3_sub(ul, right,ul);
//    glm_vec3_copy(ul, pNode->nodeVerticesBuffer[3].pos);
//    vec3 ulScreen;
//    glm_project(ul, mvp, viewport, ulScreen);
//    glm_vec2_copy(ulScreen, pNode->nodeVerticesBuffer[3].uv);
//
//    memcpy(pNode->pVertexUBOs[timelineSwitch]->pUniformBufferMapped, pNode->nodeVerticesBuffer, FBR_NODE_VERTEX_BUFFER_SIZE);
}

VkResult fbrCreateNode(const FbrApp *pApp, const char *pName, FbrNode **ppAllocNode) {
    *ppAllocNode = calloc(1, sizeof(FbrNode));
    FbrNode *pNode = *ppAllocNode;
    pNode->pName = strdup(pName);
//    pNode->radius = 1.0f;

    FbrVulkan *pVulkan = pApp->pVulkan;

    fbrCreateTransform(pVulkan, &pNode->pTransform);

    fbrCreateTimelineSemaphore(pVulkan, true, false, &pNode->pChildSemaphore);

    fbrCreateProcess(&pNode->pProcess);

    if (fbrCreateProducerIPCRingBuffer(&pNode->pProducerIPC) != 0){
        FBR_LOG_ERROR("fbrCreateProducerIPCRingBuffer fail");
        return VK_ERROR_UNKNOWN;
    }
    // todo create receiverIPC

    memcpy(pNode->nodeVerticesBuffer, nodeVertices, FBR_NODE_VERTEX_BUFFER_SIZE);

    for (int i = 0; i < FBR_NODE_FRAMEBUFFER_COUNT; ++i) {
        fbrCreateFrameBuffer(pApp->pVulkan,
                             true,
                             pApp->pSwap->format,
                             pApp->pSwap->extent,
                             &pNode->pFramebuffers[i]);
        FBR_ACK(fbrCreateUBO(pVulkan,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                             FBR_NODE_VERTEX_BUFFER_SIZE,
                             true,
                             &pNode->pVertexUBOs[i]));
//        fbrMemCopyMappedUBO(pNode->pVertexUBOs[i], pNode->nodeVerticesBuffer, FBR_NODE_VERTEX_BUFFER_SIZE);
    }

    FBR_ACK(fbrCreateUBO(pVulkan,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // host cached too?
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                         FBR_NODE_INDEX_BUFFER_SIZE,
                         false,
                         &pNode->pIndexUBO));
    fbrMemCopyMappedUBO(pNode->pIndexUBO, nodeIndices, FBR_NODE_INDEX_BUFFER_SIZE);

    fbrCreateIPCBuffer(&pNode->pCameraIPCBuffer, sizeof(FbrCameraBuffer));

    fbrCreateCamera(pVulkan, &pNode->pCamera);
}

void fbrDestroyNode(const FbrVulkan *pVulkan, FbrNode *pNode) {
    free(pNode->pName);

    for (int i = 0; i < FBR_NODE_FRAMEBUFFER_COUNT; ++i) {
        fbrDestroyUBO(pVulkan, pNode->pVertexUBOs[i]);
    }
    fbrDestroyUBO(pVulkan, pNode->pIndexUBO);

    fbrDestroyProcess(pNode->pProcess);

    fbrDestroyIPCRingBuffer(pNode->pProducerIPC);
    fbrDestroyIPCRingBuffer(pNode->pReceiverIPC);

    for (int i = 0; i < FBR_NODE_FRAMEBUFFER_COUNT; ++i) {
        fbrDestroyFrameBuffer(pVulkan, pNode->pFramebuffers[i]);
    }

    free(pNode);
}
