#include "fbr_node_parent.h"
#include "fbr_framebuffer.h"
#include "fbr_timeline_semaphore.h"
#include "fbr_camera.h"
#include "fbr_vulkan.h"
#include "fbr_ipc.h"
#include "fbr_log.h"

const Vertex nodeVertices2[FBR_NODE_VERTEX_COUNT] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f,  -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f,  0.5f, 0.0f},  {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f},  {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

void fbrUpdateNodeParentMesh(const FbrVulkan *pVulkan, FbrCamera *pCamera,int timelineSwitch, FbrNodeParent *pNode) {
    fbrUpdateTransformMatrix(&pNode->transform);

    memcpy(&pCamera->uboData, pCamera->pUBO->pUniformBufferMapped, sizeof(FbrCameraUBO));
    glm_mat4_copy(pCamera->uboData.trs, pCamera->transform.matrix);

//    printf("child\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
//           pCamera->transform.matrix[0][0],pCamera->transform.matrix[0][1],pCamera->transform.matrix[0][2],pCamera->transform.matrix[0][3],
//           pCamera->transform.matrix[1][0],pCamera->transform.matrix[1][1],pCamera->transform.matrix[1][2],pCamera->transform.matrix[1][3],
//           pCamera->transform.matrix[2][0],pCamera->transform.matrix[2][1],pCamera->transform.matrix[2][2],pCamera->transform.matrix[2][3],
//           pCamera->transform.matrix[3][0],pCamera->transform.matrix[3][1],pCamera->transform.matrix[3][2],pCamera->transform.matrix[3][3]);

    vec4 pos;
    mat4 rot;
    vec3 scale;
    glm_decompose(pCamera->transform.matrix, pos, rot, scale);
    glm_mat4_quat(rot, pCamera->transform.rot);
    glm_vec3_copy(pos, pCamera->transform.pos);

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
    glm_vec3_copy(ll, pNode->nodeVerticesBuffer[0].pos);
    vec3 llScreen;
    glm_project(ll, mvp, viewport, llScreen);
    glm_vec2_copy(llScreen, pNode->nodeVerticesBuffer[0].texCoord);

    vec3 lr;
    glm_vec3_sub(GLM_VEC3_ZERO, up,lr);
    glm_vec3_add(lr, right,lr);
    glm_vec3_copy(lr, pNode->nodeVerticesBuffer[1].pos);
    vec3 lrScreen;
    glm_project(lr, mvp, viewport, lrScreen);
    glm_vec2_copy(lrScreen, pNode->nodeVerticesBuffer[1].texCoord);

    vec3 ur;
    glm_vec3_add(GLM_VEC3_ZERO, up,ur);
    glm_vec3_add(ur, right,ur);
    glm_vec3_copy(ur, pNode->nodeVerticesBuffer[2].pos);
    vec3 urScreen;
    glm_project(ur, mvp, viewport, urScreen);
    glm_vec2_copy(urScreen, pNode->nodeVerticesBuffer[2].texCoord);

    vec3 ul;
    glm_vec3_add(GLM_VEC3_ZERO, up,ul);
    glm_vec3_sub(ul, right,ul);
    glm_vec3_copy(ul, pNode->nodeVerticesBuffer[3].pos);
    vec3 ulScreen;
    glm_project(ul, mvp, viewport, ulScreen);
    glm_vec2_copy(ulScreen, pNode->nodeVerticesBuffer[3].texCoord);

    memcpy(pNode->pVertexUBOs[timelineSwitch]->pUniformBufferMapped, pNode->nodeVerticesBuffer, FBR_NODE_VERTEX_BUFFER_SIZE);
}

void fbrCreateNodeParent(FbrNodeParent **ppAllocNodeParent) {
    *ppAllocNodeParent = calloc(1, sizeof(FbrNodeParent));
    FbrNodeParent *pNodeParent = *ppAllocNodeParent;
    fbrCreateReceiverIPC(&pNodeParent->pReceiverIPC);

    fbrInitTransform(&pNodeParent->transform);

    memcpy(pNodeParent->nodeVerticesBuffer, nodeVertices2, FBR_NODE_VERTEX_BUFFER_SIZE);
}

void fbrDestroyNodeParent(FbrVulkan *pVulkan, FbrNodeParent *pNodeParent) {
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

    FBR_LOG_DEBUG("Importing Camera.", pParam->cameraExternalHandle);
    fbrImportCamera(pVulkan, &pNodeParent->pCamera,pParam->cameraExternalHandle);

    FBR_LOG_DEBUG("Importing Framebuffer0.", pParam->framebuffer0ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    fbrImportFrameBuffer(pVulkan,
                         pParam->framebuffer0ExternalHandle,
                         (VkExtent2D) {pParam->framebufferWidth, pParam->framebufferHeight},
                         &pNodeParent->pFramebuffers[0]);

    FBR_LOG_DEBUG("Importing Framebuffer1.", pParam->framebuffer1ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    fbrImportFrameBuffer(pVulkan,
                         pParam->framebuffer1ExternalHandle,
                         (VkExtent2D) {pParam->framebufferWidth, pParam->framebufferHeight},
                         &pNodeParent->pFramebuffers[1]);

    FBR_LOG_DEBUG("Importing pVertexUBOs.", pParam->vertexUBO0ExternalHandle);
    fbrImportUBO(pVulkan,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 FBR_NODE_VERTEX_BUFFER_SIZE,
                 pParam->vertexUBO0ExternalHandle,
                 &pNodeParent->pVertexUBOs[0]);
    fbrMemCopyMappedUBO(pNodeParent->pVertexUBOs[0], pNodeParent->nodeVerticesBuffer, FBR_NODE_VERTEX_BUFFER_SIZE);

    FBR_LOG_DEBUG("Importing pVertexUBOs.", pParam->vertexUBO1ExternalHandle);
    fbrImportUBO(pVulkan,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 FBR_NODE_VERTEX_BUFFER_SIZE,
                 pParam->vertexUBO1ExternalHandle,
                 &pNodeParent->pVertexUBOs[1]);
    fbrMemCopyMappedUBO(pNodeParent->pVertexUBOs[1], pNodeParent->nodeVerticesBuffer, FBR_NODE_VERTEX_BUFFER_SIZE);

    FBR_LOG_DEBUG("ImportTimelineSemaphore", pParam->parentSemaphoreExternalHandle);
    fbrImportTimelineSemaphore(pVulkan, true, pParam->parentSemaphoreExternalHandle, &pNodeParent->pParentSemaphore);

    FBR_LOG_DEBUG("ImportTimelineSemaphore", pParam->childSemaphoreExternalHandle);
    fbrImportTimelineSemaphore(pVulkan, false, pParam->childSemaphoreExternalHandle, &pNodeParent->pChildSemaphore);
}