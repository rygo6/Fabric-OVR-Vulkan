#ifndef FABRIC_FBR_FRAMEBUFFER_H
#define FABRIC_FBR_FRAMEBUFFER_H

#include "fbr_app.h"
#include "fbr_texture.h"

typedef struct FbrFramebuffer {
//    VkImage image;
//    VkImageView imageView;
//    VkDeviceMemory deviceMemory;

    FbrTexture *pTexture;

    VkExtent2D extent;
    VkSampleCountFlagBits samples;
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

} FbrFramebuffer;

void fbrTransitionForRender(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer);

void fbrTransitionForDisplay(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer);

void fbrCreateFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer **ppAllocFramebuffer);

void fbrImportFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer **ppAllocFramebuffer, HANDLE externalMemory, int width, int height);

void fbrDestroyFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer);

// IPC

typedef struct FbrImportFrameBufferIPCParam {
    HANDLE handle;
    uint16_t width;
    uint16_t height;
} FbrIPCParamImportFrameBuffer;

void fbrIPCTargetImportFrameBuffer(FbrApp *pApp, FbrIPCParamImportFrameBuffer *pParam);

#endif //FABRIC_FBR_FRAMEBUFFER_H
