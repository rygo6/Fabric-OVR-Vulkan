#include "fbr_vulkan.h"
#include "fbr_log.h"

#include <string.h>

#ifdef WIN32
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#ifdef X11
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#endif

//#define FBR_LOG_VULKAN_CAPABILITIES

const uint32_t requiredInstanceLayerCount = 1;
const char *pRequiredInstanceLayers[] = {
        "VK_LAYER_KHRONOS_validation"
};

const uint32_t requiredInstanceExtensionCount = 4;
const char *pRequiredInstanceExtensions[] = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME
};

const uint32_t requiredDeviceExtensionCount = 9;
const char *pRequiredDeviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
        VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
#ifdef WIN32
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
        VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME
#endif
#ifdef X11
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME
#endif
};

// any other way to get this into validation layer?
static bool isChid;
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void *pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        FBR_LOG_DEBUG(isChid ? "Child Validation Layer" : "Parent Validation Layer", pCallbackData->pMessage);
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

static void getRequiredInstanceExtensions(FbrVulkan *pVulkan, uint32_t *extensionCount, const char *pExtensions[]) {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    *extensionCount = glfwExtensionCount + requiredInstanceExtensionCount + (pVulkan->enableValidationLayers ? 1 : 0);

    if (pExtensions == NULL) {
        return;
    }

    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
        pExtensions[i] = glfwExtensions[i];
    }

    for (uint32_t i = glfwExtensionCount; i < *extensionCount; ++i) {
        pExtensions[i] = pRequiredInstanceExtensions[i - glfwExtensionCount];
    }

    if (pVulkan->enableValidationLayers) {
        pExtensions[*extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
}

static bool checkValidationLayerSupport(VkLayerProperties availableLayers[], uint32_t availableLayerCount) {
    for (int i = 0; i < requiredInstanceLayerCount; ++i) {
        bool layerFound = false;

        for (int j = 0; j < availableLayerCount; ++j) {
            if (strcmp(pRequiredInstanceLayers[i], availableLayers[j].layerName) == 0) {
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

static VkResult createInstance(FbrVulkan *pVulkan) {
    if (pVulkan->enableValidationLayers) {
        uint32_t availableLayerCount = 0;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL));
        VkLayerProperties availableLayers[availableLayerCount];
        VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers));

#ifdef FBR_LOG_VULKAN_CAPABILITIES
        FBR_LOG_DEBUG("Available Layer Count: ", availableLayerCount);
        for (int i = 0; i < availableLayerCount; ++i){
            char* layerName = availableLayers[i].layerName;
            uint32_t specVersion = availableLayers[i].specVersion;
            uint32_t implementationVersion = availableLayers[i].implementationVersion;
            char* description = availableLayers[i].description;
            FBR_LOG_DEBUG("Available Layer", layerName, specVersion, implementationVersion, description);
        }
#endif
        if (!checkValidationLayerSupport(availableLayers, availableLayerCount)) {
            FBR_LOG_DEBUG("validation layers requested, but not available!");
        }
    }

    VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Fabric",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Fabric Vulkan",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_HEADER_VERSION_COMPLETE,
    };

    VkInstanceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
    };

#ifdef FBR_LOG_VULKAN_CAPABILITIES
    uint32_t availableExtensionCount = 0;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL));
    VkExtensionProperties availableExtensions[availableExtensionCount];
    VK_CHECK(vkEnumerateInstanceExtensionProperties( NULL, &availableExtensionCount, availableExtensions));
    FBR_LOG_DEBUG("Available Instance Extension Count: ", availableExtensionCount);
    for (int i = 0; i < availableExtensionCount; ++i){
        char* extensionName = availableExtensions[i].extensionName;
        uint32_t specVersion = availableExtensions[i].specVersion;
        FBR_LOG_DEBUG("Available Instance Extension", extensionName, specVersion);
    }
#endif

    uint32_t extensionCount = 0;
    getRequiredInstanceExtensions(pVulkan, &extensionCount, NULL);
    const char *extensions[extensionCount];
    getRequiredInstanceExtensions(pVulkan, &extensionCount, extensions);

    FBR_LOG_DEBUG("Required Instance Extension Count: ", extensionCount);
    for (int i = 0; i < extensionCount; ++i){
        FBR_LOG_DEBUG("Required Instance Extension",extensions[i]);
    }

    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 0;

    if (pVulkan->enableValidationLayers) {
        createInfo.enabledLayerCount = requiredInstanceLayerCount;
        createInfo.ppEnabledLayerNames = pRequiredInstanceLayers;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = debugMessengerCreateInfo();
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = NULL;
    }

    VK_CHECK(vkCreateInstance(&createInfo, NULL, &pVulkan->instance));

    return VK_SUCCESS;
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

    FBR_VK_CHECK(CreateDebugUtilsMessengerEXT(pVulkan->instance, &createInfo, NULL, &pVulkan->debugMessenger));
}

static void pickPhysicalDevice(FbrVulkan *pVulkan) {
    uint32_t deviceCount = 0;
    FBR_VK_CHECK(vkEnumeratePhysicalDevices(pVulkan->instance, &deviceCount, NULL));
    if (deviceCount == 0) {
        FBR_LOG_DEBUG("failed to find GPUs with Vulkan support!");
    }
    VkPhysicalDevice devices[deviceCount];
    FBR_VK_CHECK(vkEnumeratePhysicalDevices(pVulkan->instance, &deviceCount, devices));

    // Todo Implement Query OpenVR for the physical device to use
    // If no OVR fallback to first one. OVR Vulkan used this logic, its much simpler than vulkan example, is it correct? Seemed to be on my 6950xt and 4090
    pVulkan->physicalDevice = devices[0];

    vkGetPhysicalDeviceMemoryProperties(pVulkan->physicalDevice, &pVulkan->memProperties);
}

static void findQueueFamilies(FbrVulkan *pVulkan) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(pVulkan->physicalDevice,
                                             &queueFamilyCount,
                                             NULL);
    VkQueueFamilyGlobalPriorityPropertiesEXT queueFamilyGlobalPriorityProperties[queueFamilyCount];
    VkQueueFamilyProperties2 queueFamilies[queueFamilyCount];
    for (int i = 0; i < queueFamilyCount; ++i) {
        queueFamilyGlobalPriorityProperties[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_EXT;
        queueFamilyGlobalPriorityProperties[i].pNext = NULL;
        queueFamilies[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        queueFamilies[i].pNext = &queueFamilyGlobalPriorityProperties[i];
    }
    vkGetPhysicalDeviceQueueFamilyProperties2(pVulkan->physicalDevice,
                                             &queueFamilyCount,
                                              (VkQueueFamilyProperties2 *) &queueFamilies);

    if (queueFamilyCount == 0) {
        FBR_LOG_DEBUG("Failed to get queue properties.");
    }

    // Taking a cue from SteamVR Vulkan example and just assuming queue that supports both graphics and present is the only one we want. Don't entirely know if that's right.
    for (int i = 0; i < queueFamilyCount; ++i) {
        VkBool32 queueSupport = queueFamilyGlobalPriorityProperties[i].priorityCount > 0;

        VkBool32 graphicsSupport = queueFamilies[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(pVulkan->physicalDevice, i, pVulkan->surface, &presentSupport);

        if (graphicsSupport && presentSupport) {
            pVulkan->graphicsQueueFamilyIndex = i;
            return;
        }
    }

    FBR_LOG_DEBUG("Failed to find a queue that supports both graphics and present!");
}

VkResult createLogicalDevice(FbrVulkan *pVulkan) {
    findQueueFamilies(pVulkan);

    const uint32_t queueFamilyCount = 1;
    VkDeviceQueueCreateInfo queueCreateInfos[queueFamilyCount];
    uint32_t uniqueQueueFamilies[] = {pVulkan->graphicsQueueFamilyIndex };

    float queuePriority[] =  {pVulkan->isChild ? 0.0f : 1.0f, pVulkan->isChild ? 0.0f : 1.0f };
//    float queuePriority[] =  {1.0f };
    for (int i = 0; i < queueFamilyCount; ++i) {
        FBR_LOG_DEBUG("Creating queue with family.", uniqueQueueFamilies[i]);
        VkDeviceQueueGlobalPriorityCreateInfoEXT queueGlobalPriorityCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_EXT,
                .globalPriority = pVulkan->isChild ? VK_QUEUE_GLOBAL_PRIORITY_LOW_EXT : VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_EXT
        };
        VkDeviceQueueCreateInfo queueCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = &queueGlobalPriorityCreateInfo,
                .queueFamilyIndex = uniqueQueueFamilies[i],
                .queueCount = 2,
                .pQueuePriorities = queuePriority
        };
        queueCreateInfos[i] = queueCreateInfo;
    }

    // TODO come up with something better for this
    VkPhysicalDeviceGlobalPriorityQueryFeaturesEXT supportedPhysicalDeviceGlobalPriorityQueryFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_EXT
    };
    VkPhysicalDeviceRobustness2FeaturesEXT supportedPhysicalDeviceRobustness2Features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
            .pNext = &supportedPhysicalDeviceGlobalPriorityQueryFeatures
    };
    VkPhysicalDeviceVulkan13Features supportedPhysicalDeviceVulkan13Features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &supportedPhysicalDeviceRobustness2Features
    };
    VkPhysicalDeviceVulkan12Features supportedPhysicalDeviceVulkan12Features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .pNext = &supportedPhysicalDeviceVulkan13Features
    };
    VkPhysicalDeviceVulkan11Features supportedPhysicalDeviceVulkan11Features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
            .pNext = &supportedPhysicalDeviceVulkan12Features
    };
    VkPhysicalDeviceFeatures2 supportedPhysicalDeviceFeatures2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &supportedPhysicalDeviceVulkan11Features,
    };
    vkGetPhysicalDeviceFeatures2(pVulkan->physicalDevice, &supportedPhysicalDeviceFeatures2);
    if (!supportedPhysicalDeviceFeatures2.features.robustBufferAccess)
        FBR_LOG_ERROR("robustBufferAccess no support!");
    if (!supportedPhysicalDeviceFeatures2.features.samplerAnisotropy)
        FBR_LOG_ERROR("samplerAnisotropy no support!");
    if (!supportedPhysicalDeviceRobustness2Features.robustImageAccess2)
        FBR_LOG_ERROR("robustImageAccess2 no support!");
    if (!supportedPhysicalDeviceRobustness2Features.robustBufferAccess2)
        FBR_LOG_ERROR("robustBufferAccess2 no support!");
    if (!supportedPhysicalDeviceRobustness2Features.nullDescriptor)
        FBR_LOG_ERROR("nullDescriptor no support!");
    if (!supportedPhysicalDeviceVulkan13Features.synchronization2)
        FBR_LOG_ERROR("synchronization2 no support!");
    if (!supportedPhysicalDeviceVulkan13Features.robustImageAccess)
        FBR_LOG_ERROR("robustImageAccess no support!");
    if (!supportedPhysicalDeviceVulkan13Features.shaderDemoteToHelperInvocation)
        FBR_LOG_ERROR("shaderDemoteToHelperInvocation no support!");
    if (!supportedPhysicalDeviceVulkan13Features.shaderTerminateInvocation)
        FBR_LOG_ERROR("shaderTerminateInvocation no support!");
    if (!supportedPhysicalDeviceGlobalPriorityQueryFeatures.globalPriorityQuery)
        FBR_LOG_ERROR("globalPriorityQuery no support!");


    VkPhysicalDeviceGlobalPriorityQueryFeaturesEXT physicalDeviceGlobalPriorityQueryFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_EXT,
            .globalPriorityQuery = true,
    };
    VkPhysicalDeviceRobustness2FeaturesEXT physicalDeviceRobustness2Features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
            .pNext = &physicalDeviceGlobalPriorityQueryFeatures,
            .robustBufferAccess2 = true,
            .robustImageAccess2 = true,
            .nullDescriptor = true,
    };
    // TODO enable robust buffer access 2 ??
    VkPhysicalDeviceVulkan13Features enabledFeatures13 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .synchronization2 = true,
            .robustImageAccess = true,
            .shaderTerminateInvocation = true,
            .shaderDemoteToHelperInvocation = true,
            .pNext = &physicalDeviceRobustness2Features,
    };
    // TODO enabel swapFramebuffer ?
    VkPhysicalDeviceVulkan12Features enabledFeatures12 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .timelineSemaphore = true,
            .imagelessFramebuffer = true,
            .pNext = &enabledFeatures13,
    };
    VkPhysicalDeviceVulkan11Features enabledFeatures11 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
            .pNext = &enabledFeatures12,
    };
    VkPhysicalDeviceFeatures2 enabledFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR,
            .pNext = &enabledFeatures11,
            .features = {
                    .samplerAnisotropy = true,
                    .robustBufferAccess = true,
                    .fragmentStoresAndAtomics = true,
                    .vertexPipelineStoresAndAtomics = true,
                    .tessellationShader = true,
            }
    };

#ifdef FBR_LOG_VULKAN_CAPABILITIES
    uint32_t availableExtensionCount = 0;
    if (vkEnumerateDeviceExtensionProperties(pVulkan->physicalDevice, NULL, &availableExtensionCount, NULL ) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Could not get the number of device extensions!");
    }
    VkExtensionProperties availableExtensions[availableExtensionCount];
    if (vkEnumerateDeviceExtensionProperties(pVulkan->physicalDevice, NULL, &availableExtensionCount, availableExtensions)) {
        FBR_LOG_DEBUG("Could not enumerate device extensions!");
    }
    FBR_LOG_DEBUG("Available Device Extension Count: ", availableExtensionCount);
    for (int i = 0; i < availableExtensionCount; ++i){
        char* extensionName = availableExtensions[i].extensionName;
        uint32_t specVersion = availableExtensions[i].specVersion;
        FBR_LOG_DEBUG("Available Device Extension", extensionName, specVersion);
    }
#endif

    FBR_LOG_DEBUG("Required Device Extension Count: ", requiredDeviceExtensionCount);
    for (int i = 0; i < requiredDeviceExtensionCount; ++i){
        FBR_LOG_DEBUG("Required Device Extension",pRequiredDeviceExtensions[i]);
    }

    const VkDeviceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &enabledFeatures,
            .queueCreateInfoCount = queueFamilyCount,
            .pQueueCreateInfos = queueCreateInfos,
            .pEnabledFeatures = NULL,
            .enabledExtensionCount = requiredDeviceExtensionCount,
            .ppEnabledExtensionNames = pRequiredDeviceExtensions,
            .ppEnabledLayerNames = pVulkan->enableValidationLayers ? pRequiredInstanceLayers : NULL,
            .enabledLayerCount = pVulkan->enableValidationLayers ? requiredInstanceLayerCount : 0,

    };
    VK_CHECK(vkCreateDevice(pVulkan->physicalDevice, &createInfo, NULL, &pVulkan->device));
//    vkGetDeviceQueue(pVulkan->device, pVulkan->graphicsQueueFamilyIndex, 0, &pVulkan->queue);
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats, uint32_t formatCount) {
    // Favor sRGB if it's available
    for (int i = 0; i < formatCount; ++i) {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB || availableFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
            return availableFormats[i];
        }
    }

    // Default to the first one if no sRGB
    return availableFormats[0];
}

static VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes, uint32_t presentModeCount) {
    // https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-2.html?language=en#_Toc445674479

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

    // So in VR you probably want to use VK_PRESENT_MODE_IMMEDIATE_KHR and rely on the OVR/OXR synchronization
    // but not in VR we probably want to use FIFO to go in hz of monitor
    swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    FBR_LOG_DEBUG("Selected Present Mode", swapChainPresentMode);

    return swapChainPresentMode;
}

static VkExtent2D chooseSwapExtent(FbrVulkan *pVulkan, const VkSurfaceCapabilitiesKHR capabilities) {
    // Logic from OVR Vulkan sample. Logic little different from vulkan tutorial
    // Don't know why I can't just use screenwdith/height??
    VkExtent2D extents;
    if (capabilities.currentExtent.width == -1) {
        // If the surface size is undefined, the size is set to the size of the images requested.
        extents.width = pVulkan->screenWidth;
        extents.height = pVulkan->screenHeight;
    } else {
        // If the surface size is defined, the swap chain size must match
        extents = capabilities.currentExtent;
    }

    FBR_LOG_DEBUG("SwapChain Extents", extents.width, extents.height);

    return extents;
}

static void createSwapChain(FbrVulkan *pVulkan) {
    // Logic from OVR Vulkan example
    VkSurfaceCapabilitiesKHR capabilities;
    FBR_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pVulkan->physicalDevice, pVulkan->surface, &capabilities));

    uint32_t formatCount;
    FBR_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(pVulkan->physicalDevice, pVulkan->surface, &formatCount, NULL));
    VkSurfaceFormatKHR formats[formatCount];
    FBR_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(pVulkan->physicalDevice, pVulkan->surface, &formatCount, (VkSurfaceFormatKHR *) &formats));

    uint32_t presentModeCount;
    FBR_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(pVulkan->physicalDevice, pVulkan->surface, &presentModeCount, NULL));
    VkPresentModeKHR presentModes[presentModeCount];
    FBR_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(pVulkan->physicalDevice, pVulkan->surface, &presentModeCount, (VkPresentModeKHR *) &presentModes));

    VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes, presentModeCount);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats, formatCount);
    pVulkan->swapImageFormat = surfaceFormat.format;
    pVulkan->swapExtent = chooseSwapExtent(pVulkan, capabilities);
    pVulkan->swapUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // I am setting this to 2 on the premise you get the least latency in VR.
    pVulkan->swapImageCount = 2;
    if (pVulkan->swapImageCount < capabilities.minImageCount) {
        FBR_LOG_DEBUG("swapImageCount is less than minImageCount", pVulkan->swapImageCount, capabilities.minImageCount);
    }

    VkSurfaceTransformFlagsKHR preTransform;
    if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = capabilities.currentTransform;
    }

    // just going to assume we can blit to swap
    // no, better to render to swap?
//    VkImageUsageFlags nImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
//    if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
//        nImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
//    } else {
//        printf("Vulkan swapchain does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT. Some operations may not be supported.\n");
//    }
//    if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == false) {
//        FBR_LOG_ERROR("Vulkan swapchain does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT");
//    }

    VkSwapchainCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = pVulkan->surface,
            .minImageCount = pVulkan->swapImageCount,
            .imageFormat = pVulkan->swapImageFormat,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = pVulkan->swapExtent,
            .imageUsage = pVulkan->swapUsage,
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
        printf("Unexpected value for VkSurfaceCapabilitiesKHR.compositeAlpha: %x\n", capabilities.supportedCompositeAlpha);
    }

    FBR_VK_CHECK(vkCreateSwapchainKHR(pVulkan->device, &createInfo, NULL, &pVulkan->swapChain));

    FBR_VK_CHECK(vkGetSwapchainImagesKHR(pVulkan->device, pVulkan->swapChain, &pVulkan->swapImageCount, NULL));
    pVulkan->pSwapImages = calloc(pVulkan->swapImageCount, sizeof(VkImage));
    FBR_VK_CHECK(vkGetSwapchainImagesKHR(pVulkan->device, pVulkan->swapChain, &pVulkan->swapImageCount, pVulkan->pSwapImages));

    if (pVulkan->swapImageCount != 2) {
        FBR_LOG_ERROR("Resulting swapchain count is not 2! Was planning on this always being 2. What device disallows 2!?");
    }

    FBR_LOG_DEBUG("swapchain created", pVulkan->swapImageCount, surfaceFormat.format, pVulkan->swapExtent.width, pVulkan->swapExtent.height);
}

static void createImageViews(FbrVulkan *pVulkan) {
    pVulkan->pSwapImageViews = calloc(pVulkan->swapImageCount, sizeof(VkImageView));

    for (size_t i = 0; i < pVulkan->swapImageCount; i++) {
        VkImageViewCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = pVulkan->pSwapImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = pVulkan->swapImageFormat,
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

        FBR_VK_CHECK(vkCreateImageView(pVulkan->device, &createInfo, NULL, &pVulkan->pSwapImageViews[i]));
    }
}

static void createRenderPass(FbrVulkan *pVulkan) {
    // supposedly most correct https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present
    VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
    };
    VkSubpassDependency dependencies[2] = {
            {
                    // https://gist.github.com/chrisvarns/b4a5dbd1a09545948261d8c650070383
                    // In subpass zero...
                    .dstSubpass = 0,
                    // ... at this pipeline stage ...
                    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    // ... wait before performing these operations ...
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    // ... until all operations of that type stop ...
                    .srcAccessMask = VK_ACCESS_NONE_KHR,
                    // ... at that same stages ...
                    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    // ... occuring in submission order prior to vkCmdBeginRenderPass ...
                    .srcSubpass = VK_SUBPASS_EXTERNAL,
                    // ... have completed execution.
                    .dependencyFlags = 0,
            },
            {
                    // ... In the external scope after the subpass ...
                    .dstSubpass = VK_SUBPASS_EXTERNAL,
                    // ... before anything can occur with this pipeline stage ...
                    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    // ... wait for all operations to stop ...
                    .dstAccessMask = VK_ACCESS_NONE_KHR,
                    // ... of this type ...
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    // ... at this stage ...
                    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    // ... in subpass 0 ...
                    .srcSubpass = 0,
                    // ... before it can execute and signal the semaphore rendering complete semaphore
                    // set to VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR on vkQueueSubmit2KHR  .
                    .dependencyFlags = 0,
            },
    };
    VkAttachmentDescription attachmentDescription = {
            .format = pVulkan->swapImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &attachmentDescription,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 2,
            .pDependencies = dependencies,
    };

    FBR_VK_CHECK(vkCreateRenderPass(pVulkan->device, &renderPassInfo, NULL, &pVulkan->renderPass));
}

static VkResult createFramebuffer(FbrVulkan *pVulkan) {
    const VkFramebufferAttachmentImageInfo framebufferAttachmentImageInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
            .width = pVulkan->swapExtent.width,
            .height = pVulkan->swapExtent.height,
            .layerCount = 1,
            .usage = pVulkan->swapUsage,
            .pViewFormats = &pVulkan->swapImageFormat,
            .viewFormatCount = 1,
    };
    const VkFramebufferAttachmentsCreateInfo framebufferAttachmentsCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO,
            .attachmentImageInfoCount = 1,
            .pAttachmentImageInfos = &framebufferAttachmentImageInfo,
    };
    const VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = &framebufferAttachmentsCreateInfo,
            .renderPass = pVulkan->renderPass,
            .flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
            .width = pVulkan->swapExtent.width,
            .height = pVulkan->swapExtent.height,
            .layers = 1,
            .attachmentCount = 1,
    };
    VK_CHECK(vkCreateFramebuffer(pVulkan->device, &framebufferCreateInfo, NULL, &pVulkan->swapFramebuffer));
}

static void createCommandPool(FbrVulkan *pVulkan) {
    VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
    };
    FBR_VK_CHECK(vkCreateCommandPool(pVulkan->device, &poolInfo, NULL, &pVulkan->commandPool));
}

static void createDescriptorPool(FbrVulkan *pVulkan) {
    const VkDescriptorPoolSize poolSizes[2] = {
            {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 3,
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 3,
            }
    };

    VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = 2,
            .pPoolSizes = poolSizes,
            .maxSets = 3,
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

    if (vkAllocateCommandBuffers(pVulkan->device, &allocInfo, &pVulkan->commandBuffer) != VK_SUCCESS)
        FBR_LOG_DEBUG("failed to allocate command buffers!");
}

static VkResult createSyncObjects(FbrVulkan *pVulkan) { // todo move to swap sync objects?
    const VkSemaphoreCreateInfo swapchainSemaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VK_CHECK(vkCreateSemaphore(pVulkan->device, &swapchainSemaphoreCreateInfo, NULL, &pVulkan->swapAcquireComplete));
    VK_CHECK(vkCreateSemaphore(pVulkan->device, &swapchainSemaphoreCreateInfo, NULL, &pVulkan->renderCompleteSemaphore));
}

static void createSurface(const FbrApp *pApp, FbrVulkan *pVulkan) {
    if (glfwCreateWindowSurface(pVulkan->instance, pApp->pWindow, NULL, &pVulkan->surface) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create window surface!");
    }
}

static void createTextureSampler(FbrVulkan *pVulkan){
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(pVulkan->physicalDevice, &properties);
    FBR_LOG_DEBUG("Max Anisotropy!", properties.limits.maxSamplerAnisotropy);

    VkSamplerCreateInfo samplerInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .mipLodBias = 0.0f,
            .minLod = 0.0f,
            .maxLod = 0.0f,
    };

    FBR_VK_CHECK(vkCreateSampler(pVulkan->device, &samplerInfo, NULL, &pVulkan->sampler));
}

static void initVulkan(const FbrApp *pApp, FbrVulkan *pVulkan) {
    FBR_LOG_DEBUG("initializing vulkan!");

    // app
    createInstance(pVulkan);
    createSurface(pApp, pVulkan);

    // device
    setupDebugMessenger(pVulkan);
    pickPhysicalDevice(pVulkan);
    createLogicalDevice(pVulkan);
    vkGetDeviceQueue(pVulkan->device, pVulkan->graphicsQueueFamilyIndex, pVulkan->isChild ? 1 : 0, &pVulkan->queue);

    // render
    createSwapChain(pVulkan);
    createImageViews(pVulkan);
    createRenderPass(pVulkan);
    createFramebuffer(pVulkan);
    createCommandPool(pVulkan);
    createCommandBuffer(pVulkan);
    createSyncObjects(pVulkan);
    createDescriptorPool(pVulkan);

    createTextureSampler(pVulkan);

    if (!pApp->isChild) {
        fbrCreateTimelineSemaphore(pVulkan, true, true, &pVulkan->pMainSemaphore);
    }
}

void fbrCreateVulkan(const FbrApp *pApp, FbrVulkan **ppAllocVulkan, int screenWidth, int screenHeight, bool enableValidationLayers) {
    *ppAllocVulkan = calloc(1, sizeof(FbrVulkan));
    FbrVulkan *pVulkan = *ppAllocVulkan;

    // tf need better place for child flag
    pVulkan->isChild = pApp->isChild;
    isChid = pApp->isChild;

    pVulkan->enableValidationLayers = enableValidationLayers;
    pVulkan->screenWidth = screenWidth;
    pVulkan->screenHeight = screenHeight;
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

void fbrCleanupVulkan(FbrVulkan *pVulkan) {
    if (pVulkan->pMainSemaphore != NULL)
        fbrDestroyTimelineSemaphore(pVulkan, pVulkan->pMainSemaphore);

    vkDestroyFramebuffer(pVulkan->device, pVulkan->swapFramebuffer, NULL);

    for (int i = 0; i < pVulkan->swapImageCount; ++i) {
        vkDestroyImageView(pVulkan->device, pVulkan->pSwapImageViews[i], NULL);
    }

    vkDestroySwapchainKHR(pVulkan->device, pVulkan->swapChain, NULL);

    vkDestroyDescriptorPool(pVulkan->device, pVulkan->descriptorPool, NULL);

    vkDestroyRenderPass(pVulkan->device, pVulkan->renderPass, NULL);

    free(pVulkan->pSwapImages);
    free(pVulkan->pSwapImageViews);

    vkDestroySemaphore(pVulkan->device, pVulkan->renderCompleteSemaphore, NULL);
    vkDestroySemaphore(pVulkan->device, pVulkan->swapAcquireComplete, NULL);

    vkDestroyCommandPool(pVulkan->device, pVulkan->commandPool, NULL);

    vkDestroySampler(pVulkan->device, pVulkan->sampler, NULL);

    vkDestroyDevice(pVulkan->device, NULL);

    if (pVulkan->enableValidationLayers) {
        destroyDebugUtilsMessengerEXT(pVulkan->instance, pVulkan->debugMessenger, NULL);
    }

    vkDestroySurfaceKHR(pVulkan->instance, pVulkan->surface, NULL);
    vkDestroyInstance(pVulkan->instance, NULL);

    free(pVulkan);

    glfwTerminate();
}


void fbrIPCTargetImportMainSemaphore(FbrApp *pApp, FbrIPCParamImportTimelineSemaphore *pParam){
    FBR_LOG_DEBUG("ImportTimelineSemaphore", pParam->handle);
    fbrImportTimelineSemaphore(pApp->pVulkan, true, pParam->handle, &pApp->pVulkan->pMainSemaphore);
}
