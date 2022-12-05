#ifndef MOXAIC_MXC_APP_H
#define MOXAIC_MXC_APP_H

#include "cglm/cglm.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>

typedef struct MxcCameraState MxcCameraState;
typedef struct MxcMeshState MxcMeshState;

typedef struct MxcTimeState{
    double currentTime;
    double deltaTime;
} MxcTimeState;

typedef struct MxcAppState {

    MxcCameraState *pCameraState;
    MxcTimeState *pTimeState;
    MxcMeshState *pMeshState;

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
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

} MxcAppState;

#endif //MOXAIC_MXC_APP_H
