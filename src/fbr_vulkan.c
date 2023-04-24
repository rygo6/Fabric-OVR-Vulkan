#include "fbr_vulkan.h"
#include "fbr_log.h"
#include "fbr_swap.h"

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
static bool isChild;
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void *pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        FBR_LOG_DEBUG(isChild ? "Child Validation Layer" : "Parent Validation Layer", pCallbackData->pMessage);
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
        FBR_ACK(vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL));
        VkLayerProperties availableLayers[availableLayerCount];
        FBR_ACK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers));

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
    FBR_ACK(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL));
    VkExtensionProperties availableExtensions[availableExtensionCount];
    FBR_ACK(vkEnumerateInstanceExtensionProperties( NULL, &availableExtensionCount, availableExtensions));
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

    FBR_ACK(vkCreateInstance(&createInfo, NULL, &pVulkan->instance));

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

    vkGetPhysicalDeviceMemoryProperties(pVulkan->physicalDevice, &pVulkan->physicalDeviceMemoryProperties);
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
    VkPhysicalDeviceVulkan13Features enabledFeatures13 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
//            .synchronization2 = true,
            .robustImageAccess = true,
            .shaderTerminateInvocation = true,
            .shaderDemoteToHelperInvocation = true,
            .pNext = &physicalDeviceRobustness2Features,
    };
    // TODO enable swapFramebuffer ?
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
                    .fillModeNonSolid = true,
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
    FBR_ACK(vkCreateDevice(pVulkan->physicalDevice, &createInfo, NULL, &pVulkan->device));
}

static void createRenderPass(FbrVulkan *pVulkan, VkFormat format) {
    // supposedly most correct https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present
    const VkAttachmentReference pColorAttachments[] = {
            {
                    .attachment = 0,
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
            {
                    .attachment = 1,
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
    };
    const VkAttachmentReference depthAttachmentReference = {
            .attachment = 2,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpassDescription = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 2,
            .pColorAttachments = pColorAttachments,
            .pDepthStencilAttachment = &depthAttachmentReference,
    };
    const VkSubpassDependency dependencies[] = {
            {
                    .srcSubpass = VK_SUBPASS_EXTERNAL,
                    .dstSubpass = 0,
                    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask = VK_ACCESS_NONE_KHR,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dependencyFlags = 0,
            },
            {
                    .srcSubpass = VK_SUBPASS_EXTERNAL,
                    .dstSubpass = 0,
                    .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    .srcAccessMask = VK_ACCESS_NONE_KHR,
                    .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .dependencyFlags = 0,
            },
    };
    const VkAttachmentDescription pAttachments[] = {
            {
                    .format = format,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .finalLayout = pVulkan->isChild ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .flags = 0
            },
            {
                    .format = FBR_NORMAL_BUFFER_FORMAT,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .flags = 0
            },
            {
                    .format = FBR_DEPTH_BUFFER_FORMAT,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .flags = 0
            },
    };
    VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 3,
            .pAttachments = pAttachments,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 1,
            .pDependencies = dependencies,
    };

    FBR_VK_CHECK(vkCreateRenderPass(pVulkan->device, &renderPassInfo, NULL, &pVulkan->renderPass));
}

static void createCommandPool(FbrVulkan *pVulkan) {
    VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
    };
    FBR_VK_CHECK(vkCreateCommandPool(pVulkan->device,
                                     &poolInfo,
                                     NULL,
                                     &pVulkan->commandPool));
}

static FBR_RESULT createDescriptorPool(FbrVulkan *pVulkan) {
    const VkDescriptorPoolSize poolSizes[] = {
            {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 4,
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                    .descriptorCount = 4,
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 4,
            }
    };
    const VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = 3,
            .pPoolSizes = poolSizes,
            .maxSets = 12,
    };
    FBR_ACK(vkCreateDescriptorPool(pVulkan->device,
                                   &poolInfo,
                                   NULL,
                                   &pVulkan->descriptorPool));
}

static FBR_RESULT createCommandBuffer(FbrVulkan *pVulkan) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pVulkan->commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    FBR_ACK(vkAllocateCommandBuffers(pVulkan->device,
                                     &allocInfo,
                                     &pVulkan->commandBuffer));
}

static void createSurface(const FbrApp *pApp, FbrVulkan *pVulkan) {
    if (glfwCreateWindowSurface(pVulkan->instance, pApp->pWindow, NULL, &pVulkan->surface) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create window surface!");
    }
}

static void createTextureSampler(FbrVulkan *pVulkan){
    FBR_LOG_DEBUG("Max Anisotropy!", pVulkan->physicalDeviceProperties.limits.maxSamplerAnisotropy);

    VkSamplerCreateInfo samplerInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = pVulkan->physicalDeviceProperties.limits.maxSamplerAnisotropy,
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
    vkGetPhysicalDeviceProperties(pVulkan->physicalDevice, &pVulkan->physicalDeviceProperties);

    // render
    VkFormat swapFormat = chooseSwapSurfaceFormat(pVulkan).format;
    createRenderPass(pVulkan, swapFormat); // todo shouldn't be here?
    createCommandPool(pVulkan);
    createCommandBuffer(pVulkan);
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
    isChild = pApp->isChild;

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

    vkDestroyDescriptorPool(pVulkan->device, pVulkan->descriptorPool, NULL);

    vkDestroyRenderPass(pVulkan->device, pVulkan->renderPass, NULL);

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
