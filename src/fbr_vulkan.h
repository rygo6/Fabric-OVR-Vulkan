#ifndef FABRIC_VULKAN_H
#define FABRIC_VULKAN_H

#include "fbr_app.h"

#include "windows.h"

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

    bool isChild;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue queue;
    uint32_t graphicsQueueFamilyIndex;

    VkSwapchainKHR swapChain;
    uint32_t swapImageCount;
    VkImage *pSwapImages;
    VkImageView *pSwapImageViews;
    VkFormat swapImageFormat;
    VkImageUsageFlags swapUsage;
    VkExtent2D swapExtent;
    VkFramebuffer swapFramebuffer;
    VkRenderPass swapRenderPass;
    VkDescriptorSet swapDescriptorSet;

    VkSemaphore swapAcquireComplete;
    VkSemaphore renderCompleteSemaphore;

    VkFence queueFence;

    uint64_t timelineValue;
    VkSemaphore timelineSemaphore;
    HANDLE externalTimelineSemaphore;

    VkDescriptorPool descriptorPool;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;


    VkSampler sampler;

    VkPhysicalDeviceMemoryProperties memProperties;

} FbrVulkan;

void fbrCreateVulkan(const FbrApp *pApp,
                     FbrVulkan **ppAllocVulkan,
                     int screenWidth,
                     int screenHeight,
                     bool enableValidationLayers);

void fbrCleanupVulkan(FbrVulkan *pVulkan);

// IPC

typedef struct FbrIPCParamImportTimelineSemaphore {
    HANDLE handle;
} FbrIPCParamImportTimelineSemaphore;

void fbrIPCTargetImportTimelineSemaphore(FbrApp *pApp, FbrIPCParamImportTimelineSemaphore *pParam);

#endif //FABRIC_VULKAN_H
