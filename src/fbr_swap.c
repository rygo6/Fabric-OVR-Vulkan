#include "fbr_swap.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const FbrVulkan *pVulkan)
{
    uint32_t formatCount;
    FBR_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(pVulkan->physicalDevice, pVulkan->surface, &formatCount, NULL));
    VkSurfaceFormatKHR formats[formatCount];
    FBR_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(pVulkan->physicalDevice, pVulkan->surface, &formatCount, (VkSurfaceFormatKHR *) &formats));

    // Favor sRGB if it's available
    for (int i = 0; i < formatCount; ++i) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB || formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
            return formats[i];
        }
    }

    // Default to the first one if no sRGB
    return formats[0];
}

static VkPresentModeKHR chooseSwapPresentMode(const FbrVulkan *pVulkan)
{
    uint32_t presentModeCount;
    FBR_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(pVulkan->physicalDevice, pVulkan->surface, &presentModeCount, NULL));
    VkPresentModeKHR presentModes[presentModeCount];
    FBR_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(pVulkan->physicalDevice, pVulkan->surface, &presentModeCount, (VkPresentModeKHR *) &presentModes));

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
        //                                  An internal single-entry graphicsQueue is used to hold pending presentation requests.
        // 3. VK_PRESENT_MODE_FIFO_RELAXED_KHR - equivalent of eglSwapInterval(-1).
        if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            // The presentation engine does not wait for a vertical blanking period to update the
            // current image, meaning this mode may result in visible tearing
            swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        } else if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapChainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        } else if ((swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
                   (presentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR)) {
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

static VkExtent2D chooseSwapExtent(const FbrVulkan *pVulkan, const VkSurfaceCapabilitiesKHR capabilities)
{
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

static FBR_RESULT createSwapChain(const FbrVulkan *pVulkan, FbrSwap *pSwap)
{
    // Logic from OVR Vulkan example
    VkSurfaceCapabilitiesKHR capabilities;
    FBR_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pVulkan->physicalDevice, pVulkan->surface, &capabilities));

    VkPresentModeKHR presentMode = chooseSwapPresentMode(pVulkan);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(pVulkan);
    pSwap->format = surfaceFormat.format;
    pSwap->extent = chooseSwapExtent(pVulkan, capabilities);

    // I am setting this to 2 on the premise you get the least latency in VR.
    if (FBR_SWAP_COUNT < capabilities.minImageCount) {
        FBR_LOG_DEBUG("FBR_SWAP_COUNT is less than minImageCount", FBR_SWAP_COUNT, capabilities.minImageCount);
    }

    pSwap->usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // OBS is adding VK_IMAGE_USAGE_TRANSFER_SRC_BIT is there a way to detect that!?
//    if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
//        pVulkan->swapUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
//    } else {
//        printf("Vulkan swapchain does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT. Some operations may not be supported.\n");
//    }

    VkSurfaceTransformFlagsKHR preTransform;
    if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = capabilities.currentTransform;
    }

    VkSwapchainCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = pVulkan->surface,
            .minImageCount = FBR_SWAP_COUNT,
            .imageFormat = pSwap->format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = pSwap->extent,
            .imageUsage = pSwap->usage,
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

    FBR_VK_CHECK(vkCreateSwapchainKHR(pVulkan->device, &createInfo, NULL, &pSwap->swapChain));

    uint32_t swapCount;
    FBR_VK_CHECK(vkGetSwapchainImagesKHR(pVulkan->device, pSwap->swapChain, &swapCount, NULL));
    if (swapCount != FBR_SWAP_COUNT) {
        FBR_LOG_ERROR("Resulting swap count is not 2! Was planning on this always being 2. What device disallows 2!?");
        return VK_ERROR_UNKNOWN;
    }

//    VkImage pSwapImages[FBR_SWAP_COUNT];
    FBR_VK_CHECK(vkGetSwapchainImagesKHR(pVulkan->device, pSwap->swapChain, &swapCount, pSwap->pSwapImages));

//    for (int i = 0; i < FBR_SWAP_COUNT; ++i) {
//        fbrCreateFrameBufferFromImage(pVulkan, pSwap->format, pSwap->extent, pSwapImages[i], &pSwap->pFramebuffers[i]);
//    }

    FBR_LOG_DEBUG("Swap created.", swapCount, surfaceFormat.format, pSwap->extent.width, pSwap->extent.height);

    return VK_SUCCESS;
}

static VkResult createSyncObjects(const FbrVulkan *pVulkan, FbrSwap *pSwap)
{ // todo move to swap sync objects?
    const VkSemaphoreCreateInfo swapchainSemaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    FBR_ACK(vkCreateSemaphore(pVulkan->device, &swapchainSemaphoreCreateInfo, NULL, &pSwap->acquireComplete));
    FBR_ACK(vkCreateSemaphore(pVulkan->device, &swapchainSemaphoreCreateInfo, NULL, &pSwap->renderCompleteSemaphore));
}

void fbrCreateSwap(const FbrVulkan *pVulkan,
                   VkExtent2D extent,
                   FbrSwap **ppAllocSwap)
{
    *ppAllocSwap = calloc(1, sizeof(FbrSwap));
    FbrSwap *pSwap = *ppAllocSwap;
    pSwap->extent = extent;

    createSwapChain(pVulkan, pSwap);
    createSyncObjects(pVulkan, pSwap);
}

void fbrDestroySwap(const FbrVulkan *pVulkan, FbrSwap *pSwap)
{
    vkDestroySemaphore(pVulkan->device, pSwap->renderCompleteSemaphore, NULL);
    vkDestroySemaphore(pVulkan->device, pSwap->acquireComplete, NULL);

    free(pSwap);
}
