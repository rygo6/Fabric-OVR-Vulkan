#ifndef FABRIC_VULKAN_H
#define FABRIC_VULKAN_H

#include "fbr_app.h"
#include "windows.h"
#include "fbr_timeline_semaphore.h"
//#include "vulkan/vk_enum_string_helper.h"

//typedef enum FbrResult {
//    FBR_SUCCESS = 0,
//    FBR_FAIL = 1,
//} FbrResult;

typedef enum FbrResultFlags {
    FBR_FAIL = 1111000001,
} FbrResult;

#define FBR_SUCCESS VK_SUCCESS
#define FBR_RESULT VkResult

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

//1000001003
//2147483647
// string_VkResult(result)

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

#define FBR_ALLOCATOR NULL

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

    VkQueue queue;
    uint32_t graphicsQueueFamilyIndex;

    VkRenderPass renderPass;

    //todo should go elsewhere?
    FbrTimelineSemaphore *pMainSemaphore;

    VkDescriptorPool descriptorPool;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

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
