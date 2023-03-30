#ifndef FABRIC_FRAMEBUFFER_H
#define FABRIC_FRAMEBUFFER_H

#include "fbr_app.h"
#include "fbr_texture.h"

#define FBR_EXTERNAL_COLOR_BUFFER_USAGE (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
#define FBR_EXTERNAL_DEPTH_BUFFER_USAGE (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
#define FBR_COLOR_BUFFER_USAGE (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
#define FBR_DEPTH_BUFFER_USAGE (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)

typedef struct FbrFramebuffer {
    uint32_t count;
    FbrTexture *pColorTexture;
    FbrTexture *pDepthTexture;
    VkFramebuffer framebuffer;
    VkSampleCountFlagBits samples;
} FbrFramebuffer;

void fbrTransitionForRender(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer);

void fbrTransitionForDisplay(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer);

void fbrCreateFrameBufferFromImage(const FbrVulkan *pVulkan,
                                   VkFormat colorFormat,
                                   VkExtent2D extent,
                                   VkImage image,
                                   FbrFramebuffer **ppAllocFramebuffer);

void fbrCreateFrameBuffer(const FbrVulkan *pVulkan,
                          bool external,
                          VkFormat colorFormat,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer);

void fbrImportFrameBuffer(const FbrVulkan *pVulkan,
                          HANDLE colorExternalMemory,
                          HANDLE depthExternalMemory,
                          VkFormat colorFormat,
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
