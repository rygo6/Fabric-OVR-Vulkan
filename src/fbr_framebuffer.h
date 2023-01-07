#ifndef FABRIC_FBR_FRAMEBUFFER_H
#define FABRIC_FBR_FRAMEBUFFER_H

#include "fbr_app.h"

typedef struct FbrFramebuffer {
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory deviceMemory;
    VkFormat imageFormat;
    VkExtent2D extent;
    VkSampleCountFlagBits samples;
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

} FbrFramebuffer;

void fbrCreateFramebuffer(const FbrVulkan *pVulkan, FbrFramebuffer **ppAllocFramebuffer);

void fbrDestroyFramebuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer);

#endif //FABRIC_FBR_FRAMEBUFFER_H
