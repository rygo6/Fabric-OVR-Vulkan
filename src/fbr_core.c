#include "fbr_core.h"
#include "fbr_mesh.h"
#include "fbr_buffer.h"
#include "fbr_pipeline.h"
#include "fbr_texture.h"
#include "fbr_log.h"

#include "cglm/cglm.h"
#include "fbr_input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const uint32_t validationLayersCount = 1;
const char *pValidationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
};

const uint32_t requiredExtensionCount = 1;
const char *pRequiredExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void fbrInitWindow(FbrApp *pApp) {
    FBR_LOG_DEBUG("initializing window!");

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    pApp->pWindow = glfwCreateWindow(pApp->screenWidth, pApp->screenHeight, "Fabric", NULL, NULL);
    if (pApp->pWindow == NULL) {
        FBR_LOG_ERROR("unable to initialize GLFW Window!");
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void *pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        FBR_LOG_DEBUG("validation layer", *pCallbackData->pMessage);
    }
    return VK_FALSE;
}

static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo) {
    pCreateInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    pCreateInfo->messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    pCreateInfo->messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    pCreateInfo->pfnUserCallback = debugCallback;
}

static void getRequiredExtensions(FbrApp *pApp, uint32_t *extensionCount, const char **pExtensions) {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (pExtensions == NULL) {
        *extensionCount = glfwExtensionCount + (pApp->enableValidationLayers ? 1 : 0);
        return;
    }

    for (int i = 0; i < glfwExtensionCount; ++i) {
        pExtensions[i] = glfwExtensions[i];
    }

    if (pApp->enableValidationLayers) {
        pExtensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
}

static bool checkValidationLayerSupport() {
    uint32_t availableLayerCount;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);

    VkLayerProperties availableLayers[availableLayerCount];
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers);

    for (int i = 0; i < validationLayersCount; ++i) {
        bool layerFound = false;

        for (int j = 0; j < availableLayerCount; ++j) {
            if (strcmp(pValidationLayers[i], availableLayers[j].layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

static void createInstance(FbrApp *pApp) {
    if (pApp->enableValidationLayers && !checkValidationLayerSupport()) {
        FBR_LOG_DEBUG("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Fabric Vulkan",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = NULL,
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0,
    };

    VkInstanceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
    };

    uint32_t extensionCount = 0;
    getRequiredExtensions(pApp, &extensionCount, NULL);

    const char *extensions[extensionCount];
    getRequiredExtensions(pApp, &extensionCount, extensions);

    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 0;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    if (pApp->enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = pValidationLayers;

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = NULL;
    }

    if (vkCreateInstance(&createInfo, NULL, &pApp->instance) != VK_SUCCESS) {
        FBR_LOG_DEBUG("unable to initialize Vulkan!");
    }
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkDebugUtilsMessengerEXT *pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                                         "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void setupDebugMessenger(FbrApp *pApp) {
    if (!pApp->enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(&createInfo);

    if (CreateDebugUtilsMessengerEXT(pApp->instance, &createInfo, NULL, &pApp->debugMessenger) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to set up debug messenger!");
    }
}

static bool createSurface(FbrApp *pApp) {
    if (glfwCreateWindowSurface(pApp->instance, pApp->pWindow, NULL, &pApp->surface) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create window surface!");
        return false;
    }

    return true;
}

static void pickPhysicalDevice(FbrApp *pApp) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(pApp->instance, &deviceCount, NULL);

    if (deviceCount == 0) {
        FBR_LOG_DEBUG("failed to find GPUs with Vulkan support!");
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(pApp->instance, &deviceCount, devices);

    // Todo Implement Query OpenVR for the physical device to use
    // If no OVR fallback to first one. OVR Vulkan used this logic, its much simpler than vulkan example, is it correct? Seemed to be on my 6950xt
    pApp->physicalDevice = devices[0];
}

static bool findQueueFamilies(FbrApp *pApp) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pApp->physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(pApp->physicalDevice, &queueFamilyCount,
                                             (VkQueueFamilyProperties *) &queueFamilies);

    if (queueFamilyCount == 0) {
        FBR_LOG_DEBUG("Failed to get queue properties.");
    }

    // Taking a cue from SteamVR Vulkan example and just assuming queue that supports both graphics and present is the only one we want. Don't entirely know if that's right.
    for (int i = 0; i < queueFamilyCount; ++i) {
        VkBool32 graphicsSupport = queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(pApp->physicalDevice, i, pApp->surface, &presentSupport);

        if (graphicsSupport && presentSupport) {
            pApp->graphicsQueueFamilyIndex = i;
            return true;
        }
    }

    FBR_LOG_DEBUG("Failed to find a queue that supports both graphics and present!");
    return false;
}

static bool createLogicalDevice(FbrApp *pApp) {
    if (!findQueueFamilies(pApp)) {
        return false;
    }

    const uint32_t queueFamilyCount = 1;
    VkDeviceQueueCreateInfo queueCreateInfos[queueFamilyCount];
    uint32_t uniqueQueueFamilies[] = {pApp->graphicsQueueFamilyIndex};

    float queuePriority = 1.0f;
    for (int i = 0; i < queueFamilyCount; ++i) {
        VkDeviceQueueCreateInfo queueCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = uniqueQueueFamilies[i],
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
        };
        queueCreateInfos[i] = queueCreateInfo;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = queueFamilyCount,
            .pQueueCreateInfos = queueCreateInfos,
            .pEnabledFeatures = &deviceFeatures,
            .enabledExtensionCount = requiredExtensionCount,
            .ppEnabledExtensionNames = pRequiredExtensions,
    };

    if (pApp->enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = pValidationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(pApp->physicalDevice, &createInfo, NULL, &pApp->device) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create logical device!");
        return false;
    }

    vkGetDeviceQueue(pApp->device, pApp->graphicsQueueFamilyIndex, 0, &pApp->queue);

    return true;
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats, uint32_t formatCount) {
    // Favor sRGB if it's available
    for (int i = 0; i < formatCount; ++i) {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormats[i];
        }
    }

    // Default to the first one if no sRGB
    return availableFormats[0];
}

static VkPresentModeKHR
chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes, uint32_t presentModeCount) {
    // This logic taken from OVR Vulkan Example
    // VK_PRESENT_MODE_FIFO_KHR - equivalent of eglSwapInterval(1).  The presentation engine waits for the next vertical blanking period to update
    // the current image. Tearing cannot be observed. This mode must be supported by all implementations.
    VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (int i = 0; i < presentModeCount; ++i) {
        // Order of preference for no vsync:
        // 1. VK_PRESENT_MODE_IMMEDIATE_KHR - The presentation engine does not wait for a vertical blanking period to update the current image,
        //                                    meaning this mode may result in visible tearing
        // 2. VK_PRESENT_MODE_MAILBOX_KHR - The presentation engine waits for the next vertical blanking period to update the current image. Tearing cannot be observed.
        //                                  An internal single-entry queue is used to hold pending presentation requests.
        // 3. VK_PRESENT_MODE_FIFO_RELAXED_KHR - equivalent of eglSwapInterval(-1).
        if (availablePresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            // The presentation engine does not wait for a vertical blanking period to update the
            // current image, meaning this mode may result in visible tearing
            swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        } else if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapChainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        } else if ((swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
                   (availablePresentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR)) {
            // VK_PRESENT_MODE_FIFO_RELAXED_KHR - equivalent of eglSwapInterval(-1)
            swapChainPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }
    }

    return swapChainPresentMode;
}

static VkExtent2D chooseSwapExtent(FbrApp *pApp, const VkSurfaceCapabilitiesKHR capabilities) {
    // Logic from OVR Vulkan sample. Logic little different from vulkan tutorial
    VkExtent2D extents;
    if (capabilities.currentExtent.width == -1) {
        // If the surface size is undefined, the size is set to the size of the images requested.
        extents.width = pApp->screenWidth;
        extents.height = pApp->screenHeight;
    } else {
        // If the surface size is defined, the swap chain size must match
        extents = capabilities.currentExtent;
    }

    return extents;
}

static void createSwapChain(FbrApp *pApp) {
    // Logic from OVR Vulkan example
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pApp->physicalDevice, pApp->surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pApp->physicalDevice, pApp->surface, &formatCount, NULL);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(pApp->physicalDevice, pApp->surface, &formatCount,
                                         (VkSurfaceFormatKHR *) &formats);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pApp->physicalDevice, pApp->surface, &presentModeCount, NULL);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(pApp->physicalDevice, pApp->surface, &presentModeCount,
                                              (VkPresentModeKHR *) &presentModes);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats, formatCount);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes, presentModeCount);
    VkExtent2D extent = chooseSwapExtent(pApp, capabilities);

    FBR_LOG_DEBUG("min swap count", capabilities.minImageCount);
    FBR_LOG_DEBUG("max swap count", capabilities.maxImageCount);

    // Have a swap queue depth of at least three frames
    pApp->swapChainImageCount = capabilities.minImageCount;
    if (pApp->swapChainImageCount < 2) {
        pApp->swapChainImageCount = 2;
    }
    if ((capabilities.maxImageCount > 0) && (pApp->swapChainImageCount > capabilities.maxImageCount)) {
        // Application must settle for fewer images than desired:
        pApp->swapChainImageCount = capabilities.maxImageCount;
    }
    FBR_LOG_DEBUG("swapChainImageCount selected count", pApp->swapChainImageCount);

    VkSurfaceTransformFlagsKHR preTransform;
    if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = capabilities.currentTransform;
    }

    VkImageUsageFlags nImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        nImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    } else {
        printf("Vulkan swapchain does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT. Some operations may not be supported.\n");
    }

    VkSwapchainCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = pApp->surface,
            .minImageCount = pApp->swapChainImageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageUsage = nImageUsageFlags,
            .preTransform = preTransform,
            .imageArrayLayers = 1,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = NULL,
            .presentMode = presentMode,
            .clipped = VK_TRUE
    };
    if ((capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) != 0) {
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    } else if ((capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) != 0) {
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    } else {
        printf("Unexpected value for VkSurfaceCapabilitiesKHR.compositeAlpha: %x\n",
               capabilities.supportedCompositeAlpha);
    }

    if (vkCreateSwapchainKHR(pApp->device, &createInfo, NULL, &pApp->swapChain) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(pApp->device, pApp->swapChain, &pApp->swapChainImageCount, NULL);
    pApp->pSwapChainImages = calloc(pApp->swapChainImageCount, sizeof(VkImage));
    vkGetSwapchainImagesKHR(pApp->device, pApp->swapChain, &pApp->swapChainImageCount, pApp->pSwapChainImages);

    pApp->swapChainImageFormat = surfaceFormat.format;
    pApp->swapChainExtent = extent;
}

static void createImageViews(FbrApp *pApp) {
    pApp->pSwapChainImageViews = calloc(pApp->swapChainImageCount, sizeof(VkImageView));

    for (size_t i = 0; i < pApp->swapChainImageCount; i++) {
        VkImageViewCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = pApp->pSwapChainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = pApp->swapChainImageFormat,
                .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
                .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .subresourceRange.baseMipLevel = 0,
                .subresourceRange.levelCount = 1,
                .subresourceRange.baseArrayLayer = 0,
                .subresourceRange.layerCount = 1,
        };

        if (vkCreateImageView(pApp->device, &createInfo, NULL, &pApp->pSwapChainImageViews[i]) != VK_SUCCESS) {
            FBR_LOG_DEBUG("failed to create image views!");
        }
    }
}

static void createRenderPass(FbrApp *pApp) {
    VkAttachmentDescription colorAttachment = {
            .format = pApp->swapChainImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
    };

    // OVR example doesn't have this
    VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency,
    };

    if (vkCreateRenderPass(pApp->device, &renderPassInfo, NULL, &pApp->renderPass) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create render pass!");
    }
}

static void createFramebuffers(FbrApp *pApp) {
    pApp->pSwapChainFramebuffers = calloc(pApp->swapChainImageCount, sizeof(VkFramebuffer));

    for (size_t i = 0; i < pApp->swapChainImageCount; i++) {
        VkImageView attachments[] = {
                pApp->pSwapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = pApp->renderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = pApp->swapChainExtent.width,
                .height = pApp->swapChainExtent.height,
                .layers = 1,
        };

        if (vkCreateFramebuffer(pApp->device, &framebufferInfo, NULL, &pApp->pSwapChainFramebuffers[i]) !=
            VK_SUCCESS) {
            FBR_LOG_DEBUG("failed to create framebuffer!");
        }
    }
}

static void createCommandPool(FbrApp *pApp) {
    VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = pApp->graphicsQueueFamilyIndex,
    };

    if (vkCreateCommandPool(pApp->device, &poolInfo, NULL, &pApp->commandPool) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create command pool!");
    }
}

static void createDescriptorPool(FbrApp *pApp) {
    VkDescriptorPoolSize poolSize = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
    };

    VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize,
            .maxSets = 1,
    };

    if (vkCreateDescriptorPool(pApp->device, &poolInfo, NULL, &pApp->descriptorPool) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create descriptor pool!");
    }
}

static void createCommandBuffer(FbrApp *pApp) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pApp->commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    if (vkAllocateCommandBuffers(pApp->device, &allocInfo, &pApp->commandBuffer) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to allocate command buffers!");
    }
}

static void createSyncObjects(FbrApp *pApp) {
    VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    if (vkCreateSemaphore(pApp->device, &semaphoreInfo, NULL, &pApp->imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(pApp->device, &semaphoreInfo, NULL, &pApp->renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(pApp->device, &fenceInfo, NULL, &pApp->inFlightFence) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create synchronization objects for a frame!");
    }
}

void fbrInitVulkan(FbrApp *pApp) {
    FBR_LOG_DEBUG("initializing vulkan!");

    // app
    createInstance(pApp);
    setupDebugMessenger(pApp);
    createSurface(pApp);

    // device
    pickPhysicalDevice(pApp);
    createLogicalDevice(pApp);

    // render
    createSwapChain(pApp);
    createImageViews(pApp);
    createRenderPass(pApp);
    createFramebuffers(pApp);
    createCommandPool(pApp);
    createCommandBuffer(pApp);
    createSyncObjects(pApp);
    createDescriptorPool(pApp);

    // entities
    fbrCreateCamera(pApp, &pApp->pCamera);
    fbrCreateMesh(pApp, &pApp->pMesh);
    fbrCreateTexture(pApp, &pApp->pTexture);

    // Pipeline
    fbrCreatePipeline(pApp, pApp->pCamera, &pApp->pPipeline);
}

static void recordCommandBuffer(FbrApp *pApp, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };

    if (vkBeginCommandBuffer(pApp->commandBuffer, &beginInfo) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pApp->renderPass,
            .framebuffer = pApp->pSwapChainFramebuffers[imageIndex],
            .renderArea.offset = {0, 0},
            .renderArea.extent = pApp->swapChainExtent,
    };

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(pApp->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(pApp->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pApp->pPipeline->graphicsPipeline);

        VkViewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = (float) pApp->swapChainExtent.width,
                .height = (float) pApp->swapChainExtent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
        };
        vkCmdSetViewport(pApp->commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {
                .offset = {0, 0},
                .extent = pApp->swapChainExtent,
        };
        vkCmdSetScissor(pApp->commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {pApp->pMesh->vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(pApp->commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(pApp->commandBuffer, pApp->pMesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(pApp->commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pApp->pPipeline->pipelineLayout,
                                0,
                                1,
                                &pApp->pPipeline->descriptorSet,
                                0,
                                NULL);

        vkCmdDrawIndexed(pApp->commandBuffer, FBR_TEST_INDICES_COUNT, 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(pApp->commandBuffer);

    if (vkEndCommandBuffer(pApp->commandBuffer) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to record command buffer!");
    }
}

static void waitForLastFrame(FbrApp *pApp) {
    vkWaitForFences(pApp->device, 1, &pApp->inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(pApp->device, 1, &pApp->inFlightFence);
}

static void processInputFrame(FbrApp *pApp) {
    fbrProcessInput();
    for (int i = 0; i < fbrInputEventCount(); ++i) {
        fbrUpdateCamera(pApp->pCamera, fbrGetKeyEvent(i), pApp->pTime);
    }
}

static void drawFrame(FbrApp *pApp) {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(pApp->device, pApp->swapChain, UINT64_MAX, pApp->imageAvailableSemaphore,
                          VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(pApp->commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(pApp, imageIndex);

    VkSubmitInfo submitInfo = {
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO
    };

    VkSemaphore waitSemaphores[] = {pApp->imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pApp->commandBuffer;

    VkSemaphore signalSemaphores[] = {pApp->renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(pApp->queue, 1, &submitInfo, pApp->inFlightFence) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR
    };

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {pApp->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(pApp->queue, &presentInfo);
}

void fbrMainLoop(FbrApp *pApp) {
    FBR_LOG_DEBUG("mainloop starting!");

    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(pApp->pWindow)) {

        pApp->pTime->currentTime = glfwGetTime();
        pApp->pTime->deltaTime = pApp->pTime->currentTime - lastFrameTime;
        lastFrameTime = pApp->pTime->currentTime;

        waitForLastFrame(pApp);
        processInputFrame(pApp);
        fbrMeshUpdateCameraUBO(pApp->pMesh, pApp->pCamera); //todo I dont like this
        drawFrame(pApp);
    }

    vkDeviceWaitIdle(pApp->device);
}

static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks *pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                                           "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debugMessenger, pAllocator);
    }
}

static void cleanupSwapChain(FbrApp *pApp) {

    FBR_LOG_DEBUG("cleaning up swapchain!");

    for (int i = 0; i < pApp->swapChainImageCount; ++i) {
        vkDestroyFramebuffer(pApp->device, pApp->pSwapChainFramebuffers[i], NULL);
    }

    for (int i = 0; i < pApp->swapChainImageCount; ++i) {
        vkDestroyImageView(pApp->device, pApp->pSwapChainImageViews[i], NULL);
    }

    vkDestroySwapchainKHR(pApp->device, pApp->swapChain, NULL);
}

void fbrCleanup(FbrApp *pApp) {
    FBR_LOG_DEBUG("cleaning up!");

    cleanupSwapChain(pApp);

    fbrCleanupTexture(pApp, pApp->pTexture);

    fbrFreeCamera(pApp, pApp->pCamera);

    vkDestroyDescriptorPool(pApp->device, pApp->descriptorPool, NULL);

    fbrFreePipeline(pApp, pApp->pPipeline);

    vkDestroyRenderPass(pApp->device, pApp->renderPass, NULL);

    free(pApp->pSwapChainImages);
    free(pApp->pSwapChainImageViews);

    fbrFreeMesh((const FbrApp *) pApp->device, pApp->pMesh);

    vkDestroySemaphore(pApp->device, pApp->renderFinishedSemaphore, NULL);
    vkDestroySemaphore(pApp->device, pApp->imageAvailableSemaphore, NULL);
    vkDestroyFence(pApp->device, pApp->inFlightFence, NULL);

    vkDestroyCommandPool(pApp->device, pApp->commandPool, NULL);

    vkDestroyDevice(pApp->device, NULL);

    if (pApp->enableValidationLayers) {
        destroyDebugUtilsMessengerEXT(pApp->instance, pApp->debugMessenger, NULL);
    }

    vkDestroySurfaceKHR(pApp->instance, pApp->surface, NULL);
    vkDestroyInstance(pApp->instance, NULL);

    glfwDestroyWindow(pApp->pWindow);

    free(pApp);

    glfwTerminate();
}