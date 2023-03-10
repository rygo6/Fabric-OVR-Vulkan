#ifndef FABRIC_FRAMEBUFFER_H
#define FABRIC_FRAMEBUFFER_H

#include "fbr_app.h"
#include "fbr_texture.h"

#define FBR_EXTERNAL_FRAMEBUFFER_USAGE (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
#define FBR_FRAMEBUFFER_USAGE (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)

typedef struct FbrFramebuffer {

    FbrTexture *pTexture;

    VkSampleCountFlagBits samples;
//    VkRenderPass renderPass;
    VkFramebuffer framebuffer;

//    uint64_t waitValue;
//    VkSemaphore semaphore;

} FbrFramebuffer;

void fbrTransitionForRender(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer);

void fbrTransitionForDisplay(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer);

void fbrCreateFrameBuffer(const FbrVulkan *pVulkan,
                          bool external,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer);

void fbrImportFrameBuffer(const FbrVulkan *pVulkan,
                          HANDLE externalMemory,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer);

void fbrDestroyFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer);

// IPC

typedef struct FbrImportFrameBufferIPCParam {
    HANDLE handle;
    uint16_t width;
    uint16_t height;
} FbrIPCParamImportFrameBuffer;

void fbrIPCTargetImportFrameBuffer(FbrApp *pApp, FbrIPCParamImportFrameBuffer *pParam);

#endif //FABRIC_FRAMEBUFFER_H
