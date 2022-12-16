#include "fbr_vulkan.h"
#include "fbr_log.h"

#include <string.h>

const uint32_t validationLayersCount = 1;
const char *pValidationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
};

const uint32_t requiredExtensionCount = 1;
const char *pRequiredExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void *pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        FBR_LOG_DEBUG("validation layer", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback,
    };
    return createInfo;
}

static void getRequiredExtensions(FbrVulkan *pVulkan, uint32_t *extensionCount, const char **pExtensions) {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (pExtensions == NULL) {
        *extensionCount = glfwExtensionCount + (pVulkan->enableValidationLayers ? 1 : 0);
        return;
    }

    for (int i = 0; i < glfwExtensionCount; ++i) {
        pExtensions[i] = glfwExtensions[i];
    }

    if (pVulkan->enableValidationLayers) {
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


static void createInstance(FbrVulkan *pVulkan) {
    if (pVulkan->enableValidationLayers && !checkValidationLayerSupport()) {
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
    getRequiredExtensions(pVulkan, &extensionCount, NULL);

    const char *extensions[extensionCount];
    getRequiredExtensions(pVulkan, &extensionCount, extensions);

    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 0;

    if (pVulkan->enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = pValidationLayers;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = debugMessengerCreateInfo();
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = NULL;
    }

    if (vkCreateInstance(&createInfo, NULL, &pVulkan->instance) != VK_SUCCESS) {
        FBR_LOG_DEBUG("unable to initialize Vulkan!");
    }
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkDebugUtilsMessengerEXT *pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void setupDebugMessenger(FbrVulkan *pVulkan) {
    if (!pVulkan->enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = debugMessengerCreateInfo();

    if (CreateDebugUtilsMessengerEXT(pVulkan->instance, &createInfo, NULL, &pVulkan->debugMessenger) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to set up debug messenger!");
    }
}

static void pickPhysicalDevice(FbrVulkan *pVulkan) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(pVulkan->instance, &deviceCount, NULL);

    if (deviceCount == 0) {
        FBR_LOG_DEBUG("failed to find GPUs with Vulkan support!");
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(pVulkan->instance, &deviceCount, devices);

    // Todo Implement Query OpenVR for the physical device to use
    // If no OVR fallback to first one. OVR Vulkan used this logic, its much simpler than vulkan example, is it correct? Seemed to be on my 6950xt
    pVulkan->physicalDevice = devices[0];
}

static void findQueueFamilies(FbrVulkan *pVulkan) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pVulkan->physicalDevice,
                                             &queueFamilyCount,
                                             NULL);
    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(pVulkan->physicalDevice,
                                             &queueFamilyCount,
                                             (VkQueueFamilyProperties *) &queueFamilies);

    if (queueFamilyCount == 0) {
        FBR_LOG_DEBUG("Failed to get queue properties.");
    }

    // Taking a cue from SteamVR Vulkan example and just assuming queue that supports both graphics and present is the only one we want. Don't entirely know if that's right.
    for (int i = 0; i < queueFamilyCount; ++i) {
        VkBool32 graphicsSupport = queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(pVulkan->physicalDevice, i, pVulkan->surface, &presentSupport);

        if (graphicsSupport && presentSupport) {
            pVulkan->graphicsQueueFamilyIndex = i;
            return;
        }
    }

    FBR_LOG_DEBUG("Failed to find a queue that supports both graphics and present!");
}

static void createLogicalDevice(FbrVulkan *pVulkan) {
    findQueueFamilies(pVulkan);

    const uint32_t queueFamilyCount = 1;
    VkDeviceQueueCreateInfo queueCreateInfos[queueFamilyCount];
    uint32_t uniqueQueueFamilies[] = {pVulkan->graphicsQueueFamilyIndex};

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

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(pVulkan->physicalDevice, &supportedFeatures);

    //force robust buffer access??
    VkPhysicalDeviceFeatures enabledFeatures = {
            .samplerAnisotropy = true
    };

    VkDeviceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = queueFamilyCount,
            .pQueueCreateInfos = queueCreateInfos,
            .pEnabledFeatures = &enabledFeatures,
            .enabledExtensionCount = requiredExtensionCount,
            .ppEnabledExtensionNames = pRequiredExtensions,
    };

    if (pVulkan->enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = pValidationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(pVulkan->physicalDevice, &createInfo, NULL, &pVulkan->device) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create logical device!");
    }

    vkGetDeviceQueue(pVulkan->device, pVulkan->graphicsQueueFamilyIndex, 0, &pVulkan->queue);
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

static VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes, uint32_t presentModeCount) {
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

static void createSwapChain(FbrVulkan *pVulkan) {
    // Logic from OVR Vulkan example
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pVulkan->physicalDevice, pVulkan->surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pVulkan->physicalDevice, pVulkan->surface, &formatCount, NULL);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(pVulkan->physicalDevice, pVulkan->surface, &formatCount, (VkSurfaceFormatKHR *) &formats);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pVulkan->physicalDevice, pVulkan->surface, &presentModeCount, NULL);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(pVulkan->physicalDevice, pVulkan->surface, &presentModeCount, (VkPresentModeKHR *) &presentModes);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats, formatCount);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes, presentModeCount);
    VkExtent2D extent = chooseSwapExtent(pVulkan, capabilities);

    FBR_LOG_DEBUG("min swap count", capabilities.minImageCount);
    FBR_LOG_DEBUG("max swap count", capabilities.maxImageCount);

    // Have a swap queue depth of at least three frames
    pVulkan->swapChainImageCount = capabilities.minImageCount;
    if (pVulkan->swapChainImageCount < 2) {
        pVulkan->swapChainImageCount = 2;
    }
    if ((capabilities.maxImageCount > 0) && (pVulkan->swapChainImageCount > capabilities.maxImageCount)) {
        // Application must settle for fewer images than desired:
        pVulkan->swapChainImageCount = capabilities.maxImageCount;
    }
    FBR_LOG_DEBUG("swapChainImageCount selected count", pVulkan->swapChainImageCount);

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
            .surface = pVulkan->surface,
            .minImageCount = pVulkan->swapChainImageCount,
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

    if (vkCreateSwapchainKHR(pVulkan->device, &createInfo, NULL, &pVulkan->swapChain) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(pVulkan->device, pVulkan->swapChain, &pVulkan->swapChainImageCount, NULL);
    pVulkan->pSwapChainImages = calloc(pVulkan->swapChainImageCount, sizeof(VkImage));
    vkGetSwapchainImagesKHR(pVulkan->device, pVulkan->swapChain, &pVulkan->swapChainImageCount, pVulkan->pSwapChainImages);

    pVulkan->swapChainImageFormat = surfaceFormat.format;
    pVulkan->swapChainExtent = extent;
}

static void createImageViews(FbrVulkan *pVulkan) {
    pVulkan->pSwapChainImageViews = calloc(pVulkan->swapChainImageCount, sizeof(VkImageView));

    for (size_t i = 0; i < pVulkan->swapChainImageCount; i++) {
        VkImageViewCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = pVulkan->pSwapChainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = pVulkan->swapChainImageFormat,
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

        if (vkCreateImageView(pVulkan->device, &createInfo, NULL, &pVulkan->pSwapChainImageViews[i]) != VK_SUCCESS) {
            FBR_LOG_DEBUG("failed to create image views!");
        }
    }
}

static void createRenderPass(FbrVulkan *pVulkan) {
    VkAttachmentDescription colorAttachment = {
            .format = pVulkan->swapChainImageFormat,
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

    if (vkCreateRenderPass(pVulkan->device, &renderPassInfo, NULL, &pVulkan->renderPass) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create render pass!");
    }
}

static void createFramebuffers(FbrVulkan *pVulkan) {
    pVulkan->pSwapChainFramebuffers = calloc(pVulkan->swapChainImageCount, sizeof(VkFramebuffer));

    for (size_t i = 0; i < pVulkan->swapChainImageCount; i++) {
        VkImageView attachments[] = {
                pVulkan->pSwapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = pVulkan->renderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = pVulkan->swapChainExtent.width,
                .height = pVulkan->swapChainExtent.height,
                .layers = 1,
        };

        if (vkCreateFramebuffer(pVulkan->device, &framebufferInfo, NULL, &pVulkan->pSwapChainFramebuffers[i]) != VK_SUCCESS) {
            FBR_LOG_DEBUG("failed to create framebuffer!");
        }
    }
}

static void createCommandPool(FbrVulkan *pVulkan) {
    VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
    };

    if (vkCreateCommandPool(pVulkan->device, &poolInfo, NULL, &pVulkan->commandPool) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create command pool!");
    }
}

static void createDescriptorPool(FbrVulkan *pVulkan) {
    const uint32_t frameCount = 1;
    const uint32_t poolSizeCount = 2;
    VkDescriptorPoolSize poolSizes[] = {
            {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = frameCount,
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = frameCount,
            }
    };

    VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = poolSizeCount,
            .pPoolSizes = poolSizes,
            .maxSets = frameCount,
    };

    if (vkCreateDescriptorPool(pVulkan->device, &poolInfo, NULL, &pVulkan->descriptorPool) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create descriptor pool!");
    }
}

static void createCommandBuffer(FbrVulkan *pVulkan) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pVulkan->commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    if (vkAllocateCommandBuffers(pVulkan->device, &allocInfo, &pVulkan->commandBuffer) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to allocate command buffers!");
    }
}

static void createSyncObjects(FbrVulkan *pVulkan) {
    VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    if (vkCreateSemaphore(pVulkan->device, &semaphoreInfo, NULL, &pVulkan->imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(pVulkan->device, &semaphoreInfo, NULL, &pVulkan->renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(pVulkan->device, &fenceInfo, NULL, &pVulkan->inFlightFence) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create synchronization objects for a frame!");
    }
}

static void createSurface(const FbrApp *pApp, FbrVulkan *pVulkan) {
    if (glfwCreateWindowSurface(pVulkan->instance, pApp->pWindow, NULL, &pVulkan->surface) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create window surface!");
    }
}

void initVulkan(const FbrApp *pApp, FbrVulkan *pVulkan) {
    FBR_LOG_DEBUG("initializing vulkan!");

    // app
    createInstance(pVulkan);
    createSurface(pApp, pVulkan);

    // device
    setupDebugMessenger(pVulkan);
    pickPhysicalDevice(pVulkan);
    createLogicalDevice(pVulkan);

    // render
    createSwapChain(pVulkan);
    createImageViews(pVulkan);
    createRenderPass(pVulkan);
    createFramebuffers(pVulkan);
    createCommandPool(pVulkan);
    createCommandBuffer(pVulkan);
    createSyncObjects(pVulkan);
    createDescriptorPool(pVulkan);
}

void fbrCreateVulkan(const FbrApp *pApp, FbrVulkan **ppAllocVulkan, bool enableValidationLayers) {
    *ppAllocVulkan = calloc(1, sizeof(FbrVulkan));
    FbrVulkan *pVulkan = *ppAllocVulkan;
    pVulkan->enableValidationLayers = enableValidationLayers;
    initVulkan(pApp, pVulkan);
}

static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks *pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                                           "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debugMessenger, pAllocator);
    }
}

static void cleanupSwapChain(FbrVulkan *pVulkan) {
    FBR_LOG_DEBUG("cleaning up swapchain!");

    for (int i = 0; i < pVulkan->swapChainImageCount; ++i) {
        vkDestroyFramebuffer(pVulkan->device, pVulkan->pSwapChainFramebuffers[i], NULL);
    }

    for (int i = 0; i < pVulkan->swapChainImageCount; ++i) {
        vkDestroyImageView(pVulkan->device, pVulkan->pSwapChainImageViews[i], NULL);
    }

    vkDestroySwapchainKHR(pVulkan->device, pVulkan->swapChain, NULL);
}

void fbrCleanupVulkan(FbrVulkan *pVulkan) {
    FBR_LOG_DEBUG("cleaning up!");

    cleanupSwapChain(pVulkan);

    vkDestroyDescriptorPool(pVulkan->device, pVulkan->descriptorPool, NULL);

    vkDestroyRenderPass(pVulkan->device, pVulkan->renderPass, NULL);

    free(pVulkan->pSwapChainImages);
    free(pVulkan->pSwapChainImageViews);

    vkDestroySemaphore(pVulkan->device, pVulkan->renderFinishedSemaphore, NULL);
    vkDestroySemaphore(pVulkan->device, pVulkan->imageAvailableSemaphore, NULL);
    vkDestroyFence(pVulkan->device, pVulkan->inFlightFence, NULL);

    vkDestroyCommandPool(pVulkan->device, pVulkan->commandPool, NULL);

    vkDestroyDevice(pVulkan->device, NULL);

    if (pVulkan->enableValidationLayers) {
        destroyDebugUtilsMessengerEXT(pVulkan->instance, pVulkan->debugMessenger, NULL);
    }

    vkDestroySurfaceKHR(pVulkan->instance, pVulkan->surface, NULL);
    vkDestroyInstance(pVulkan->instance, NULL);

    free(pVulkan);

    glfwTerminate();
}

