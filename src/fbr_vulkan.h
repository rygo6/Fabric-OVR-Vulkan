#ifndef FABRIC_VULKAN_H
#define FABRIC_VULKAN_H

#include "fbr_app.h"

#define FBR_VK_CHECK(command)\
    do { \
        VkResult result = command;\
        if (result != VK_SUCCESS) {\
            printf("VKCheck Fail! - %s - %s - %d\n", __FUNCTION__, #command, result);\
            }\
    } while (0)

// todo there needs to be some mechanic of dealloc if this fails
#define FBR_VK_CHECK_RETURN(command)\
    do { \
        VkResult result = command;\
        if (result != VK_SUCCESS) {\
            printf("VKCheck Fail! - %s - %s - %d\n", __FUNCTION__, #command, result);\
            return result;\
            }\
    } while (0)

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
    uint32_t swapchainImageCount;
    VkImage *pSwapChainImages;
    VkImageView *pSwapChainImageViews;
    VkFormat swapChainImageFormat;
    VkImageUsageFlags swapchainUsage;
    VkExtent2D swapChainExtent;

//    VkFramebuffer *pSwapChainFramebuffers;
    VkFramebuffer imagelessFramebuffer;
    VkRenderPass swapRenderPass;
    VkDescriptorSet swapDescriptorSet;

    VkDescriptorPool descriptorPool;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore acquireCompleteSemaphore;
    VkSemaphore renderCompleteSemaphore;
    VkFence inFlightFence;

    uint64_t timelineValue;
    VkSemaphore timelineSemaphore;

    VkSampler sampler;

    VkPhysicalDeviceMemoryProperties memProperties;

} FbrVulkan;

void fbrCreateVulkan(const FbrApp *pApp,
                     FbrVulkan **ppAllocVulkan,
                     int screenWidth,
                     int screenHeight,
                     bool enableValidationLayers);

void fbrCleanupVulkan(FbrVulkan *pVulkan);

#endif //FABRIC_VULKAN_H
