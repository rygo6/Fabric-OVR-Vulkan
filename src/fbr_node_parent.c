#include "fbr_node_parent.h"
#include "fbr_framebuffer.h"
#include "fbr_timeline_semaphore.h"
#include "fbr_camera.h"
#include "fbr_vulkan.h"
#include "fbr_ipc.h"
#include "fbr_log.h"

void fbrCreateNodeParent(FbrNodeParent **ppAllocNodeParent) {
    *ppAllocNodeParent = calloc(1, sizeof(FbrNodeParent));
    FbrNodeParent *pNodeParent = *ppAllocNodeParent;
    fbrCreateReceiverIPC(&pNodeParent->pReceiverIPC);
}

void fbrDestroyNodeParent(FbrVulkan *pVulkan, FbrNodeParent *pNodeParent) {
    // Should I be destroy imported things???
    vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &pNodeParent->parentFramebufferDescriptorSet);
    fbrDestroyFrameBuffer(pVulkan, pNodeParent->pFramebuffer);
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
//    fbrCreateCamera(pVulkan, &pNodeParent->pCamera);

    FBR_LOG_DEBUG("Importing Framebuffer.", pParam->framebufferExternalHandle, pParam->framebufferWidth, pParam->framebufferHeight);
    fbrImportFrameBuffer(pVulkan,
                         pParam->framebufferExternalHandle,
                         (VkExtent2D) {pParam->framebufferWidth, pParam->framebufferHeight},
                         &pNodeParent->pFramebuffer);
//    fbrCreateFrameBuffer(pVulkan,
//                         false,
//                         (VkExtent2D) {pParam->framebufferWidth, pParam->framebufferHeight},
//                         &pNodeParent->pFramebuffer);

    FBR_LOG_DEBUG("ImportTimelineSemaphore", pParam->parentSemaphoreExternalHandle);
    fbrImportTimelineSemaphore(pVulkan, true, pParam->parentSemaphoreExternalHandle, &pNodeParent->pParentSemaphore);

    FBR_LOG_DEBUG("ImportTimelineSemaphore", pParam->childSemaphoreExternalHandle);
    fbrImportTimelineSemaphore(pVulkan, false, pParam->childSemaphoreExternalHandle, &pNodeParent->pChildSemaphore);
}