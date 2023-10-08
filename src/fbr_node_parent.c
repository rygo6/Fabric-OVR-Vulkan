#include "fbr_node_parent.h"
#include "fbr_framebuffer.h"
#include "fbr_timeline_semaphore.h"
#include "fbr_camera.h"
#include "fbr_vulkan.h"
#include "fbr_ipc.h"
#include "fbr_log.h"
#include "fbr_swap.h"

void fbrCreateNodeParent(const FbrVulkan *pVulkan, FbrNodeParent **ppAllocNodeParent) {
    *ppAllocNodeParent = calloc(1, sizeof(FbrNodeParent));
    FbrNodeParent *pNodeParent = *ppAllocNodeParent;
    fbrCreateReceiverIPCRingBuffer(&pNodeParent->pReceiverIPC);
    fbrCreateTransform(pVulkan, &pNodeParent->pTransform);
}

void fbrDestroyNodeParent(const FbrVulkan *pVulkan, FbrNodeParent *pNodeParent) {
    fbrDestroyTransform(pVulkan, pNodeParent->pTransform);
    fbrDestroyTimelineSemaphore(pVulkan, pNodeParent->pParentSemaphore);
    fbrDestroyTimelineSemaphore(pVulkan, pNodeParent->pChildSemaphore);
    fbrDestroyIPCRingBuffer(pNodeParent->pReceiverIPC);
    fbrDestroyIPCBuffer(pNodeParent->pCameraIPCBuffer);
}

void fbrIPCTargetImportNodeParent(FbrApp *pApp, FbrIPCParamImportNodeParent *pParam)
{
    FbrNodeParent *pNodeParent = pApp->pNodeParent;
    FbrVulkan *pVulkan = pApp->pVulkan;
    VkFormat swapFormat = chooseSwapSurfaceFormat(pVulkan).format;

    FBR_LOG_MESSAGE("Importing Node Parent");

    fbrImportIPCBuffer(&pNodeParent->pCameraIPCBuffer,
                       sizeof(FbrCameraBuffer));

    FBR_LOG_DEBUG(pParam->colorFramebuffer0ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    FBR_LOG_DEBUG(pParam->normalFramebuffer0ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    FBR_LOG_DEBUG(pParam->gbufferFramebuffer0ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    FBR_LOG_DEBUG(pParam->depthFramebuffer0ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    fbrImportFrameBuffer(pVulkan,
                         pParam->colorFramebuffer0ExternalHandle,
                         pParam->normalFramebuffer0ExternalHandle,
                         pParam->gbufferFramebuffer0ExternalHandle,
                         pParam->depthFramebuffer0ExternalHandle,
                         swapFormat,
                         (VkExtent2D) {pParam->framebufferWidth, pParam->framebufferHeight},
                         &pApp->pFramebuffers[0]);
    FBR_LOG_DEBUG(pParam->colorFramebuffer1ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    FBR_LOG_DEBUG(pParam->normalFramebuffer1ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    FBR_LOG_DEBUG(pParam->gbufferFramebuffer1ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    FBR_LOG_DEBUG(pParam->depthFramebuffer1ExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    fbrImportFrameBuffer(pVulkan,
                         pParam->colorFramebuffer1ExternalHandle,
                         pParam->normalFramebuffer1ExternalHandle,
                         pParam->gbufferFramebuffer1ExternalHandle,
                         pParam->depthFramebuffer1ExternalHandle,
                         swapFormat,
                         (VkExtent2D) {pParam->framebufferWidth, pParam->framebufferHeight},
                         &pApp->pFramebuffers[1]);

    FBR_LOG_DEBUG(pParam->parentSemaphoreExternalHandle);
    fbrImportTimelineSemaphore(pVulkan,
                               true,
                               pParam->parentSemaphoreExternalHandle,
                               &pNodeParent->pParentSemaphore);

    FBR_LOG_DEBUG(pParam->childSemaphoreExternalHandle);
    fbrImportTimelineSemaphore(pVulkan,
                               false,
                               pParam->childSemaphoreExternalHandle,
                               &pNodeParent->pChildSemaphore);
}