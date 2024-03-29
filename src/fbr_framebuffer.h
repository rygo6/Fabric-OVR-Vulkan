#ifndef FABRIC_FRAMEBUFFER_H
#define FABRIC_FRAMEBUFFER_H

#include "fbr_app.h"
#include "fbr_texture.h"

//#define FBR_EXTERNAL_COLOR_BUFFER_USAGE (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT)
//#define FBR_EXTERNAL_NORMAL_BUFFER_USAGE (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
//#define FBR_EXTERNAL_DEPTH_BUFFER_USAGE (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
#define FBR_COLOR_BUFFER_USAGE (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
#define FBR_NORMAL_BUFFER_USAGE (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
#define FBR_G_BUFFER_USAGE (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
#define FBR_DEPTH_BUFFER_USAGE (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)

#define FBR_COLOR_BUFFER_FORMAT VK_FORMAT_R8G8B8A8_UNORM
#define FBR_NORMAL_BUFFER_FORMAT VK_FORMAT_R16G16B16A16_SFLOAT
#define FBR_G_BUFFER_FORMAT VK_FORMAT_R16G16B16A16_SFLOAT
#define FBR_DEPTH_BUFFER_FORMAT VK_FORMAT_D32_SFLOAT

typedef struct FbrFramebuffer {
    FbrTexture *pColorTexture;
    FbrTexture *pNormalTexture;
    FbrTexture *pGBufferTexture;
    FbrTexture *pDepthTexture;
    VkFramebuffer framebuffer;
    VkSampleCountFlagBits samples;
    VkSemaphore renderCompleteSemaphore;
} FbrFramebuffer;

void fbrTransitionForRender(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer);

void fbrTransitionForDisplay(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer);

void fbrCreateFrameBufferFromImage(const FbrVulkan *pVulkan,
                                   VkFormat colorFormat,
                                   VkExtent2D extent,
                                   VkImage image,
                                   FbrFramebuffer **ppAllocFramebuffer);

// Can someone of be consolidated to fewer methods?
void fbrReleaseFramebufferFromGraphicsAttachToExternalRead(const FbrVulkan *pVulkan,
                                                           const FbrFramebuffer *pFramebuffer);

void fbrAcquireFramebufferFromExternalAttachToComputeRead(const FbrVulkan *pVulkan,
                                                          const FbrFramebuffer *pFramebuffer);

void fbrAcquireFramebufferFromExternalToGraphicsAttach(const FbrVulkan *pVulkan,
                                                       const FbrFramebuffer *pFramebuffer);

void fbrReleaseFramebufferFromGraphicsAttachToComputeRead(const FbrVulkan *pVulkan,
                                                          const FbrFramebuffer *pFramebuffer);

void fbrAcquireFramebufferFromGraphicsAttachToComputeRead(const FbrVulkan *pVulkan,
                                                          const FbrFramebuffer *pFramebuffer);

void fbrAcquireFramebufferFromExternalAttachToGraphicsRead(const FbrVulkan *pVulkan,
                                                           const FbrFramebuffer *pFramebuffer);

void fbrTransitionFramebufferFromIgnoredReadToGraphicsAttach(const FbrVulkan *pVulkan,
                                                             const FbrFramebuffer *pFramebuffer);

void fbrCreateFrameBuffer(const FbrVulkan *pVulkan,
                          bool external,
                          VkFormat colorFormat,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer);

void fbrImportFrameBuffer(const FbrVulkan *pVulkan,
                          HANDLE colorExternalMemory,
                          HANDLE normalExternalMemory,
                          HANDLE gbufferExternalMemory,
                          HANDLE depthExternalMemory,
                          VkFormat colorFormat,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer);

void fbrDestroyFrameBuffer(const FbrVulkan *pVulkan,
                           FbrFramebuffer *pFramebuffer);

// IPC

typedef struct FbrImportFrameBufferIPCParam {
    HANDLE handle;
    uint16_t width;
    uint16_t height;
} FbrIPCParamImportFrameBuffer;

void fbrIPCTargetImportFrameBuffer(FbrApp *pApp, FbrIPCParamImportFrameBuffer *pParam);

#endif //FABRIC_FRAMEBUFFER_H
