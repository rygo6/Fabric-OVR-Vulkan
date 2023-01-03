#ifndef FABRIC_VULKAN_H
#define FABRIC_VULKAN_H

#include "fbr_app.h"

#define VK_CHECK(alloc_expression) if (alloc_expression) != VK_SUCCESS) { FBR_LOG_DEBUG("Failed #alloc_expression"); }

typedef struct FbrVulkan {
    int screenWidth;
    int screenHeight;

    bool enableValidationLayers;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue queue;
    uint32_t graphicsQueueFamilyIndex;

    VkSwapchainKHR swapChain;
    uint32_t swapChainImageCount;
    VkImage *pSwapChainImages;
    VkImageView *pSwapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkFramebuffer *pSwapChainFramebuffers;

    VkRenderPass renderPass;

    VkDescriptorPool descriptorPool;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;


} FbrVulkan;

void fbrCreateVulkan(const FbrApp *pApp,
                     FbrVulkan **ppAllocVulkan,
                     int screenWidth,
                     int screenHeight,
                     bool enableValidationLayers);

void fbrCleanupVulkan(FbrVulkan *pVulkan);

#endif //FABRIC_VULKAN_H
