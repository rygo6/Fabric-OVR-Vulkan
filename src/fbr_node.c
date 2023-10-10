#include "fbr_node.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"
#include "fbr_process.h"
#include "fbr_ipc.h"
#include "fbr_swap.h"

void fbrNodeUpdateCameraIPCFromCamera(const FbrVulkan *pVulkan, FbrNode *pNode, FbrCamera *pFromCamera)
{
    vec3 viewPosition;
    glm_mat4_mulv3(pFromCamera->bufferData.view, pNode->pTransform->pos, 1, viewPosition);
    float viewDistanceToCenter = -viewPosition[2];
    float offset = pNode->size * 0.5f;
    float nearZ = viewDistanceToCenter - offset;
    float farZ = viewDistanceToCenter + offset;
    if (nearZ < FBR_CAMERA_NEAR_DEPTH) {
        nearZ = FBR_CAMERA_NEAR_DEPTH;
    }
    FbrNodeCamera *pRenderingCameraBuffer = pNode->pRenderingCameraBuffer;
    glm_perspective(FBR_CAMERA_FOV, pVulkan->screenFOV, nearZ, farZ, pRenderingCameraBuffer->proj);
    glm_mat4_inv(pRenderingCameraBuffer->proj, pRenderingCameraBuffer->invProj);
    glm_mat4_copy(pFromCamera->bufferData.view, pRenderingCameraBuffer->view);
    glm_mat4_copy(pFromCamera->bufferData.invView, pRenderingCameraBuffer->invView);
    glm_mat4_copy(pFromCamera->pTransform->uboData.model, pRenderingCameraBuffer->model);
    pRenderingCameraBuffer->width = pFromCamera->bufferData.width;
    pRenderingCameraBuffer->height = pFromCamera->bufferData.height;
    memcpy(pNode->pCameraIPCBuffer->pBuffer, pRenderingCameraBuffer, sizeof(FbrNodeCamera));
}

void fbrNodeUpdateCompositingCameraFromRenderingCamera(FbrNode *pNode)
{
    FbrNodeCamera *pRenderingCameraBuffer = pNode->pRenderingCameraBuffer;
    glm_mat4_copy(pRenderingCameraBuffer->proj, pNode->pCompositingCamera->bufferData.proj);
    glm_mat4_copy(pRenderingCameraBuffer->invProj, pNode->pCompositingCamera->bufferData.invProj);
    glm_mat4_copy(pRenderingCameraBuffer->view, pNode->pCompositingCamera->bufferData.view);
    glm_mat4_copy(pRenderingCameraBuffer->invView, pNode->pCompositingCamera->bufferData.invView);
    glm_mat4_copy(pRenderingCameraBuffer->model, pNode->pCompositingCamera->pTransform->uboData.model);
    pNode->pCompositingCamera->bufferData.width = pRenderingCameraBuffer->width;
    pNode->pCompositingCamera->bufferData.height = pRenderingCameraBuffer->height;
    fbrUpdateCameraUBO(pNode->pCompositingCamera);
}

VkResult fbrCreateNode(const FbrApp *pApp, const char *pName, FbrNode **ppAllocNode) {
    *ppAllocNode = calloc(1, sizeof(FbrNode));
    FbrNode *pNode = *ppAllocNode;
    pNode->pName = strdup(pName);
    pNode->size = 1.0f;

    FbrVulkan *pVulkan = pApp->pVulkan;

    fbrCreateTransform(pVulkan, &pNode->pTransform);

    fbrCreateTimelineSemaphore(pVulkan, true, false, &pNode->pChildSemaphore);

    fbrCreateProcess(&pNode->pProcess);

    if (fbrCreateProducerIPCRingBuffer(&pNode->pProducerIPC) != 0){
        FBR_LOG_ERROR("fbrCreateProducerIPCRingBuffer fail");
        return VK_ERROR_UNKNOWN;
    }
    // todo create receiverIPC

    for (int i = 0; i < FBR_NODE_FRAMEBUFFER_COUNT; ++i) {
        fbrCreateFrameBuffer(pApp->pVulkan,
                             true,
                             pApp->pSwap->format,
                             pApp->pSwap->extent,
                             &pNode->pFramebuffers[i]);
    }

    pNode->pRenderingCameraBuffer = calloc(1, sizeof(FbrNodeCamera));
    fbrCreateIPCBuffer(&pNode->pCameraIPCBuffer, sizeof(FbrNodeCamera));

    fbrCreateCamera(pVulkan, &pNode->pCompositingCamera);
}

void fbrDestroyNode(const FbrVulkan *pVulkan, FbrNode *pNode) {
    free(pNode->pName);

    fbrDestroyProcess(pNode->pProcess);

    fbrDestroyIPCRingBuffer(pNode->pProducerIPC);
    fbrDestroyIPCRingBuffer(pNode->pReceiverIPC);

    for (int i = 0; i < FBR_NODE_FRAMEBUFFER_COUNT; ++i) {
        fbrDestroyFrameBuffer(pVulkan, pNode->pFramebuffers[i]);
    }

    free(pNode);
}
