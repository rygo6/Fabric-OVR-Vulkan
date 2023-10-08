#include "fbr_node.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"
#include "fbr_process.h"
#include "fbr_ipc.h"
#include "fbr_swap.h"

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

    for (int i = 0; i < FBR_NODE_FRAMEBUFFER_COUNT; ++i) {
        fbrCreateFrameBuffer(pApp->pVulkan,
                             true,
                             pApp->pSwap->format,
                             pApp->pSwap->extent,
                             &pNode->pFramebuffers[i]);
    }

    pNode->pRenderingNodeCameraIPCBuffer = calloc(1, sizeof(FbrNodeCameraIPCBuffer));
    fbrCreateIPCBuffer(&pNode->pCameraIPCBuffer, sizeof(FbrNodeCameraIPCBuffer));

    fbrCreateCamera(pVulkan, &pNode->pCamera);
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
