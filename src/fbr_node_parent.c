#include "fbr_node_parent.h"
#include "fbr_framebuffer.h"
#include "fbr_timeline_semaphore.h"
#include "fbr_camera.h"
#include "fbr_vulkan.h"
#include "fbr_ipc.h"
#include "fbr_log.h"
#include "fbr_swap.h"

const Vertex nodeVertices2[FBR_NODE_VERTEX_COUNT] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f,  -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f,  0.5f, 0.0f},  {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f},  {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

void fbrUpdateNodeParentMesh(const FbrVulkan *pVulkan, FbrCamera *pCamera, int dynamicCameraIndex, int timelineSwitch, FbrNodeParent *pNode)
{
    uint32_t dynamicOffset = dynamicCameraIndex * pCamera->pUBO->dynamicAlignment;
    memcpy(&pCamera->uboData, pCamera->pUBO->pUniformBufferMapped + dynamicOffset, sizeof(FbrCameraUBO));
    glm_mat4_copy(pCamera->uboData.trs, pCamera->pTransform->uboData.model);

//    printf("child\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
//           pCamera->transform.matrix[0][0],pCamera->transform.matrix[0][1],pCamera->transform.matrix[0][2],pCamera->transform.matrix[0][3],
//           pCamera->transform.matrix[1][0],pCamera->transform.matrix[1][1],pCamera->transform.matrix[1][2],pCamera->transform.matrix[1][3],
//           pCamera->transform.matrix[2][0],pCamera->transform.matrix[2][1],pCamera->transform.matrix[2][2],pCamera->transform.matrix[2][3],
//           pCamera->transform.matrix[3][0],pCamera->transform.matrix[3][1],pCamera->transform.matrix[3][2],pCamera->transform.matrix[3][3]);

    vec4 pos;
    mat4 rot;
    vec3 scale;
    glm_decompose(pCamera->pTransform->uboData.model, pos, rot, scale);
    glm_mat4_quat(rot, pCamera->pTransform->rot);
    glm_vec3_copy(pos, pCamera->pTransform->pos);

    mat4 viewProj;
    glm_mat4_mul(pCamera->uboData.proj, pCamera->uboData.view, viewProj);
    mat4 mvp;
    glm_mat4_mul(viewProj, pNode->pTransform->uboData.model, mvp);

    vec4 viewport = {0.0f, 0.0f, 1.0f, 1.0f};

    vec3 up = {0.0f, 1.0f, 0.0f};
    glm_quat_rotatev(pCamera->pTransform->rot, up, up);
    glm_vec3_scale(up, 0.5f, up);

    vec3 right = {1.0f, 0.0f, 0.0f};
    glm_quat_rotatev(pCamera->pTransform->rot, right, right);
    glm_vec3_scale(right, 0.5f, right);

    vec3 forward = {0.0f, 0.0f, 1.0f};
    glm_quat_rotatev(pCamera->pTransform->rot, forward, forward);

    vec3 ll;
    glm_vec3_sub(GLM_VEC3_ZERO, up,ll);
    glm_vec3_sub(ll, right,ll);
    glm_vec3_copy(ll, pNode->nodeVerticesBuffer[0].pos);
    vec3 llScreen;
    glm_project(ll, mvp, viewport, llScreen);
    glm_vec2_copy(llScreen, pNode->nodeVerticesBuffer[0].uv);
    glm_vec3_copy(forward, pNode->nodeVerticesBuffer[0].normal);

    vec3 lr;
    glm_vec3_sub(GLM_VEC3_ZERO, up,lr);
    glm_vec3_add(lr, right,lr);
    glm_vec3_copy(lr, pNode->nodeVerticesBuffer[1].pos);
    vec3 lrScreen;
    glm_project(lr, mvp, viewport, lrScreen);
    glm_vec2_copy(lrScreen, pNode->nodeVerticesBuffer[1].uv);
    glm_vec3_copy(forward, pNode->nodeVerticesBuffer[1].normal);

    vec3 ur;
    glm_vec3_add(GLM_VEC3_ZERO, up,ur);
    glm_vec3_add(ur, right,ur);
    glm_vec3_copy(ur, pNode->nodeVerticesBuffer[2].pos);
    vec3 urScreen;
    glm_project(ur, mvp, viewport, urScreen);
    glm_vec2_copy(urScreen, pNode->nodeVerticesBuffer[2].uv);
    glm_vec3_copy(forward, pNode->nodeVerticesBuffer[2].normal);

    vec3 ul;
    glm_vec3_add(GLM_VEC3_ZERO, up,ul);
    glm_vec3_sub(ul, right,ul);
    glm_vec3_copy(ul, pNode->nodeVerticesBuffer[3].pos);
    vec3 ulScreen;
    glm_project(ul, mvp, viewport, ulScreen);
    glm_vec2_copy(ulScreen, pNode->nodeVerticesBuffer[3].uv);
    glm_vec3_copy(forward, pNode->nodeVerticesBuffer[3].normal);

    memcpy(pNode->pVertexUBOs[timelineSwitch]->pUniformBufferMapped, pNode->nodeVerticesBuffer, FBR_NODE_VERTEX_BUFFER_SIZE);

//    VkMappedMemoryRange memoryRange = {
//            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
//            .pNext = NULL,
//            .offset = 0,
//            .size = FBR_NODE_VERTEX_BUFFER_SIZE,
//            .memory = pNode->pVertexUBOs[timelineSwitch]->uniformBufferMemory
//    };
//    vkFlushMappedMemoryRanges(pVulkan->device, 1, &memoryRange);
}

void fbrCreateNodeParent(const FbrVulkan *pVulkan, FbrNodeParent **ppAllocNodeParent) {
    *ppAllocNodeParent = calloc(1, sizeof(FbrNodeParent));
    FbrNodeParent *pNodeParent = *ppAllocNodeParent;
    fbrCreateReceiverIPC(&pNodeParent->pReceiverIPC);

    fbrCreateTransform(pVulkan, &pNodeParent->pTransform);

    memcpy(pNodeParent->nodeVerticesBuffer, nodeVertices2, FBR_NODE_VERTEX_BUFFER_SIZE);
}

void fbrDestroyNodeParent(const FbrVulkan *pVulkan, FbrNodeParent *pNodeParent) {
    // Should I be destroy imported things???
    fbrDestroyFrameBuffer(pVulkan, pNodeParent->pFramebuffers[0]);
    fbrDestroyFrameBuffer(pVulkan, pNodeParent->pFramebuffers[1]);
    fbrDestroyTimelineSemaphore(pVulkan, pNodeParent->pParentSemaphore);
    fbrDestroyTimelineSemaphore(pVulkan, pNodeParent->pChildSemaphore);
    fbrDestroyCamera(pVulkan, pNodeParent->pCamera);
    fbrDestroyIPC(pNodeParent->pReceiverIPC);
}

void fbrIPCTargetImportNodeParent(FbrApp *pApp, FbrIPCParamImportNodeParent *pParam) {
    FbrNodeParent *pNodeParent = pApp->pNodeParent;
    FbrVulkan *pVulkan = pApp->pVulkan;
    VkFormat swapFormat = chooseSwapSurfaceFormat(pVulkan).format;

    FBR_LOG_DEBUG("Importing Camera.", pParam->cameraExternalHandle);
    fbrImportCamera(pVulkan,
                    FBR_DYNAMIC_MAIN_CAMERA_COUNT,
                    pParam->cameraExternalHandle,
                    &pNodeParent->pCamera);

    FBR_LOG_DEBUG("Importing Framebuffer0.", pParam->colorFramebuffer0ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    FBR_LOG_DEBUG("Importing Framebuffer0.", pParam->normalFramebuffer0ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    FBR_LOG_DEBUG("Importing Framebuffer0.", pParam->depthFramebuffer0ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    fbrImportFrameBuffer(pVulkan,
                         pParam->colorFramebuffer0ExternalHandle,
                         pParam->normalFramebuffer0ExternalHandle,
                         pParam->depthFramebuffer0ExternalHandle,
                         swapFormat,
                         (VkExtent2D) {pParam->framebufferWidth, pParam->framebufferHeight},
                         &pNodeParent->pFramebuffers[0]);
    FBR_LOG_DEBUG("Importing Framebuffer1.", pParam->colorFramebuffer1ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    FBR_LOG_DEBUG("Importing Framebuffer1.", pParam->normalFramebuffer1ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    FBR_LOG_DEBUG("Importing Framebuffer1.", pParam->depthFramebuffer1ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    fbrImportFrameBuffer(pVulkan,
                         pParam->colorFramebuffer1ExternalHandle,
                         pParam->normalFramebuffer1ExternalHandle,
                         pParam->depthFramebuffer1ExternalHandle,
                         swapFormat,
                         (VkExtent2D) {pParam->framebufferWidth, pParam->framebufferHeight},
                         &pNodeParent->pFramebuffers[1]);

    FBR_LOG_DEBUG("Importing pVertexUBOs.", pParam->vertexUBO0ExternalHandle);
    fbrImportUBO(pVulkan,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 FBR_NODE_VERTEX_BUFFER_SIZE,
                 FBR_NO_DYNAMIC_BUFFER,
                 pParam->vertexUBO0ExternalHandle,
                 &pNodeParent->pVertexUBOs[0]);
    fbrMemCopyMappedUBO(pNodeParent->pVertexUBOs[0], pNodeParent->nodeVerticesBuffer, FBR_NODE_VERTEX_BUFFER_SIZE);

    FBR_LOG_DEBUG("Importing pVertexUBOs.", pParam->vertexUBO1ExternalHandle);
    fbrImportUBO(pVulkan,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 FBR_NODE_VERTEX_BUFFER_SIZE,
                 FBR_NO_DYNAMIC_BUFFER,
                 pParam->vertexUBO1ExternalHandle,
                 &pNodeParent->pVertexUBOs[1]);
    fbrMemCopyMappedUBO(pNodeParent->pVertexUBOs[1], pNodeParent->nodeVerticesBuffer, FBR_NODE_VERTEX_BUFFER_SIZE);

    FBR_LOG_DEBUG("ImportTimelineSemaphore", pParam->parentSemaphoreExternalHandle);
    fbrImportTimelineSemaphore(pVulkan,
                               true,
                               pParam->parentSemaphoreExternalHandle,
                               &pNodeParent->pParentSemaphore);

    FBR_LOG_DEBUG("ImportTimelineSemaphore", pParam->childSemaphoreExternalHandle);
    fbrImportTimelineSemaphore(pVulkan,
                               false,
                               pParam->childSemaphoreExternalHandle,
                               &pNodeParent->pChildSemaphore);
}