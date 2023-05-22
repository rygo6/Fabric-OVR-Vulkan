#ifndef FABRIC_SWAP_H
#define FABRIC_SWAP_H

#include "fbr_app.h"
#include "fbr_framebuffer.h"

// Want to always force 2 for minimal latency
#define FBR_SWAP_COUNT 2

typedef struct FbrSwap {
    VkSwapchainKHR swapChain;
    VkFormat format;
    VkImageUsageFlags usage;
    VkExtent2D extent;
    VkSemaphore acquireCompleteSemaphore;
    VkSemaphore renderCompleteSemaphore;
    VkImage pSwapImages[FBR_SWAP_COUNT];
    VkImageView pSwapImageViews[FBR_SWAP_COUNT];
} FbrSwap;

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const FbrVulkan *pVulkan);

void fbrCreateSwap(const FbrVulkan *pVulkan,
                   VkExtent2D extent,
                   FbrSwap **ppAllocSwap);

void fbrDestroySwap(const FbrVulkan *pVulkan, FbrSwap *pSwap);

#endif //FABRIC_SWAP_H
