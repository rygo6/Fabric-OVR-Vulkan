#ifndef FABRIC_VULKAN_H
#define FABRIC_VULKAN_H

#include "fbr_app.h"
#include "windows.h"
#include "fbr_timeline_semaphore.h"

#define FBR_VK_CHECK(command)\
    do { \
        VkResult result = command; \
        if (result != VK_SUCCESS) { \
            printf("VKCheck Fail! - %s - %s - %d\n", __FUNCTION__, #command, result); \
            } \
    } while (0)

// todo there needs to be some mechanic of dealloc if this fails
#define FBR_VK_CHECK_RETURN(command) \
    do { \
        VkResult result = command; \
        if (result != VK_SUCCESS) { \
            printf("VKCheck Fail! - %s - %s - %d\n", __FUNCTION__, #command, result); \
            return result; \
            } \
    } while (0)

#define FBR_VK_CHECK_COMMAND(command)\
    do { \
        VkResult result = command;   \
        if (result == VK_ERROR_DEVICE_LOST) { \
            printf("VKCheck Command Fail! DEVICE LOST! - %s - %s - %d\n", __FUNCTION__, #command, result); \
            if (pVulkan->isChild) {  \
                exit(0); \
            } \
            printf("Attempting logical device recreation!\n"); \
            VkResult createLogicalDeviceResult = createLogicalDevice(pVulkan); \
            if (result == VK_ERROR_DEVICE_LOST) { \
                printf("Create new logical device fail! DEVICE LOST! - %s - %s - %d\n", __FUNCTION__, #command, result); \
            } \
        } \
        if (result != VK_SUCCESS) { \
            printf("VKCheck Fail! - %s - %s - %d\n", __FUNCTION__, #command, result); \
        } \
    } while (0)

typedef struct FbrVulkan {
    int screenWidth;
    int screenHeight;

    bool isChild;

    bool enableValidationLayers;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue queue;
    uint32_t graphicsQueueFamilyIndex;

    VkRenderPass renderPass;

    // go in swap object
    VkSwapchainKHR swapChain;
    uint32_t swapImageCount;
    VkImage *pSwapImages;
    VkImageView *pSwapImageViews;
    VkFormat swapImageFormat;
    VkImageUsageFlags swapUsage;
    VkExtent2D swapExtent;
    VkFramebuffer swapFramebuffer;
    VkDescriptorSet swapDescriptorSet;
    VkSemaphore swapAcquireComplete;
    VkSemaphore renderCompleteSemaphore;


    //todo should go elsewhere?
    FbrTimelineSemaphore *pMainSemaphore;

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

VkResult createLogicalDevice(FbrVulkan *pVulkan);

void fbrCleanupVulkan(FbrVulkan *pVulkan);

// IPC

typedef struct FbrIPCParamImportTimelineSemaphore {
    HANDLE handle;
} FbrIPCParamImportTimelineSemaphore;

void fbrIPCTargetImportMainSemaphore(FbrApp *pApp, FbrIPCParamImportTimelineSemaphore *pParam);

#endif //FABRIC_VULKAN_H
