#ifndef FABRIC_APP_H
#define FABRIC_APP_H

#include "cglm/cglm.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>

typedef struct FbrCameraState FbrCameraState;
typedef struct FbrMeshState FbrMeshState;
typedef struct FbrPipeline FbrPipeline;
typedef struct FbrTexture FbrTexture;

typedef struct FbrTimeState{
    double currentTime;
    double deltaTime;
} FbrTimeState;

#define FBR_APP_PARAM const FbrAppState *restrict pAppState

typedef struct FbrAppState {

    FbrCameraState *pCameraState;
    FbrTimeState *pTimeState;
    FbrMeshState *pMeshState;
    FbrTexture *pTexture;

    // GFLW
    int screenWidth;
    int screenHeight;

    GLFWwindow *pWindow;

    // Vulkan
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

    FbrPipeline *pPipeline;

    VkDescriptorPool descriptorPool;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

} FbrAppState;

#endif //FABRIC_APP_H
