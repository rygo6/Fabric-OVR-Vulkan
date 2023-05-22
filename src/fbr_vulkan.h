#ifndef FABRIC_VULKAN_H
#define FABRIC_VULKAN_H

#include "fbr_app.h"
#include "windows.h"
#include "fbr_timeline_semaphore.h"
//#include "vulkan/vk_enum_string_helper.h"

typedef enum FbrResultFlags {
    FBR_FAIL = 1111000001,
} FbrResult;

#define FBR_SUCCESS VK_SUCCESS
#define FBR_RESULT VkResult
#define FBR_ALLOCATOR NULL

// TODO switch all these to FBR_ACK
#define FBR_VK_CHECK(command)\
    do { \
        VkResult result = command; \
        if (result != VK_SUCCESS) { \
            printf("VKCheck Fail! - %s - %s - %d\n", __FUNCTION__, #command, result); \
            } \
    } while (0)

#define FBR_ACK(command)                                                                                                \
({                                                                                                                       \
    VkResult result = command;                                                                                        \
    if (result != FBR_SUCCESS) {                                                                                         \
        printf("VKCheck Fail! - %s - %s - %d\n", __FUNCTION__, #command, result);                                       \
        return result;                                                                                                  \
    }                                                                                                                   \
})

// If a VK command fails with device loss force a soft exit. Technically these should be able to
// try and recreate themselves if this happens.
#define FBR_ACK_EXIT(command)\
    do { \
        VkResult result = command;   \
        if (result == VK_ERROR_DEVICE_LOST) { \
            printf("VKCheck Command Fail! DEVICE LOST! Exiting! - %s - %s - %d\n", __FUNCTION__, #command, result); \
            if (pVulkan->isChild) {  \
                _exit(0); \
            } \
        } \
        if (result != VK_SUCCESS) { \
            printf("VKCheck Fail! - %s - %s - %d\n", __FUNCTION__, #command, result); \
        } \
    } while (0)

typedef struct FbrVulkan {
    // todo none of these should be here?
    int screenWidth;
    int screenHeight;
    bool isChild;
    bool enableValidationLayers;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamilyIndex;

    VkQueue computeQueue;
    uint32_t computeQueueFamilyIndex;

    VkRenderPass renderPass;

    //todo should go elsewhere?
    FbrTimelineSemaphore *pMainTimelineSemaphore;

    VkDescriptorPool descriptorPool;

    VkCommandPool graphicsCommandPool;
    VkCommandBuffer graphicsCommandBuffer;

    VkCommandPool computeCommandPool;
    VkCommandBuffer computeCommandBuffer;

    VkSampler sampler;

    VkPhysicalDeviceProperties  physicalDeviceProperties;
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

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
