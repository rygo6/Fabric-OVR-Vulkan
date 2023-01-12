#include "fbr_texture.h"
#include "fbr_buffer.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if WIN32
#include <vulkan/vulkan_win32.h>
#endif

#if X11
#include <vulkan/vulkan_xlib.h>
#endif

static void copyBufferToImage(const FbrVulkan *pVulkan,
                       VkBuffer buffer,
                       VkImage image,
                       uint32_t width,
                       uint32_t height) {
    VkCommandBuffer commandBuffer = fbrBeginBufferCommands(pVulkan);

    VkBufferImageCopy region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .imageSubresource.mipLevel = 0,
            .imageSubresource.baseArrayLayer = 0,
            .imageSubresource.layerCount = 1,
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1},
    };

    vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
    );

    fbrEndBufferCommands(pVulkan, commandBuffer);
}

static void createTextureFromExternal(const FbrVulkan *pVulkan,
                          FbrTexture *pTexture,
                          uint32_t width,
                          uint32_t height,
                          VkFormat format,
                          VkImageTiling tiling,
                          VkImageUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          HANDLE sharedHandle) {

    VkExternalMemoryHandleTypeFlags sharedHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;
    VkExternalMemoryImageCreateInfoKHR externalImageInfo = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR,
            .pNext = VK_NULL_HANDLE,
            .handleTypes = sharedHandleType
    };

    VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = format,
            .tiling = tiling,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = usage,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .pNext = &externalImageInfo,
    };

    if (vkCreateImage(pVulkan->device, &imageCreateInfo, NULL, &pTexture->image) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to create image!");
    }

    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(pVulkan->device, pTexture->image, &memRequirements);

//    VkMemoryDedicatedAllocateInfoKHR dedicatedAllocInfo = {
//            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
//            .pNext = VK_NULL_HANDLE,
//            .image = pTexture->image,
//            .buffer = VK_NULL_HANDLE
//    };
    VkImportMemoryWin32HandleInfoKHR importMemoryInfo = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
//            .pNext = &dedicatedAllocInfo,
            .handleType = sharedHandleType,
            .handle = sharedHandle,
    };
    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = fbrFindMemoryType(pVulkan->physicalDevice, memRequirements.memoryTypeBits, properties),
            .pNext = &importMemoryInfo
    };

    VkResult result = vkAllocateMemory(pVulkan->device, &allocInfo, NULL, &pTexture->deviceMemory);
    if (result != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to allocate image memory!", result);
        return;
    }

    vkBindImageMemory(pVulkan->device, pTexture->image, pTexture->deviceMemory, 0);

    fbrTransitionImageLayout(pVulkan, pTexture->image,
                             VK_FORMAT_R8G8B8A8_SRGB,
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

static void createExternalTexture(const FbrVulkan *pVulkan,
                          FbrTexture *pTexture,
                          uint32_t width,
                          uint32_t height,
                          VkFormat format,
                          VkImageTiling tiling,
                          VkImageUsageFlags usage,
                          VkMemoryPropertyFlags properties) {

    VkExternalMemoryHandleTypeFlagBitsKHR externalHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;
    VkExternalMemoryImageCreateInfoKHR externalImageInfo = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .handleTypes = externalHandleType,
    };
    VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = &externalImageInfo,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = format,
            .tiling = tiling,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = usage,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    FBR_VK_CHECK(vkCreateImage(pVulkan->device, &imageCreateInfo, NULL, &pTexture->image));

    VkImageMemoryRequirementsInfo2KHR memoryRequirementsInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR,
            .image = pTexture->image,
    };
    VkMemoryDedicatedRequirementsKHR memoryDedicatedRequirements = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR,
    };
    VkMemoryRequirements2KHR memRequirements2 = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR,
            .pNext = &memoryDedicatedRequirements,
    };
    PFN_vkGetImageMemoryRequirements2KHR getImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2KHR) vkGetInstanceProcAddr(pVulkan->instance, "vkGetImageMemoryRequirements2KHR");
    if (getImageMemoryRequirements2 == NULL) {
        FBR_LOG_DEBUG("Failed to get PFN_vkGetImageMemoryRequirements2KHR!");
    }
    getImageMemoryRequirements2(pVulkan->device, &memoryRequirementsInfo, &memRequirements2);
    FBR_LOG_DEBUG("prefersDedicatedAllocation", memoryDedicatedRequirements.prefersDedicatedAllocation);
    FBR_LOG_DEBUG("requiresDedicatedAllocation", memoryDedicatedRequirements.requiresDedicatedAllocation);

    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(pVulkan->device, pTexture->image, &memRequirements);

    // not sure if dedicated is needed???
//    VkMemoryDedicatedAllocateInfoKHR dedicatedAllocInfo = {
//            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
//            .image = pTexture->image,
//            .buffer = VK_NULL_HANDLE,
//    };
    VkExportMemoryAllocateInfo exportAllocInfo = {
            .sType =VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
//            .pNext =&dedicatedAllocInfo,
            .handleTypes = externalHandleType
    };
    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements2.memoryRequirements.size,
            .memoryTypeIndex = fbrFindMemoryType(pVulkan->physicalDevice, memRequirements.memoryTypeBits, properties),
            .pNext = &exportAllocInfo
    };

    FBR_VK_CHECK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, &pTexture->deviceMemory));

    vkBindImageMemory(pVulkan->device, pTexture->image, pTexture->deviceMemory, 0);

#if WIN32
    VkMemoryGetWin32HandleInfoKHR memoryInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
            .pNext = NULL,
            .memory = pTexture->deviceMemory,
            .handleType = externalHandleType
    };

    PFN_vkGetMemoryWin32HandleKHR getMemoryWin32HandleFunc = (PFN_vkGetMemoryWin32HandleKHR) vkGetInstanceProcAddr(pVulkan->instance, "vkGetMemoryWin32HandleKHR");
    if (getMemoryWin32HandleFunc == NULL) {
        FBR_LOG_DEBUG("Failed to get PFN_vkGetMemoryWin32HandleKHR!");
    }
    if (getMemoryWin32HandleFunc(pVulkan->device, &memoryInfo, &pTexture->sharedMemory) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to get external handle!");
    }
#endif
}

static void createTexture(const FbrVulkan *pVulkan,
                   FbrTexture *pTexture,
                   uint32_t width,
                   uint32_t height,
                   VkFormat format,
                   VkImageTiling tiling,
                   VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties) {

    VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = format,
            .tiling = tiling,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = usage,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    FBR_VK_CHECK(vkCreateImage(pVulkan->device, &imageCreateInfo, NULL, &pTexture->image));

    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(pVulkan->device, pTexture->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = fbrFindMemoryType(pVulkan->physicalDevice, memRequirements.memoryTypeBits, properties),
    };

    FBR_VK_CHECK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, &pTexture->deviceMemory));

    vkBindImageMemory(pVulkan->device, pTexture->image, pTexture->deviceMemory, 0);
}

static void createTextureView(const FbrVulkan *pVulkan, FbrTexture *pTexture, VkFormat format) {
    VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = pTexture->image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
    };

    if (vkCreateImageView(pVulkan->device, &viewInfo, NULL, &pTexture->imageView) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create texture image view!");
    }
}

static void createPopulateTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture, char const *filename, const bool external) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageBufferSize = texWidth * texHeight * 4;

    FBR_LOG_DEBUG("Loading Texture", filename, texWidth, texHeight, texChannels);

    if (!pixels) {
        FBR_LOG_DEBUG("Failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    fbrCreateStagingBuffer(pVulkan,
                           pixels,
                           &stagingBuffer,
                           &stagingBufferMemory,
                           imageBufferSize);

    stbi_image_free(pixels);

    if (external) {
        createExternalTexture(pVulkan,
                              pTexture,
                              texWidth,
                              texHeight,
                              VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    } else {
        createTexture(pVulkan,
                      pTexture,
                      texWidth,
                      texHeight,
                      VK_FORMAT_R8G8B8A8_SRGB,
                      VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    fbrTransitionImageLayout(pVulkan,
                             pTexture->image,
                             VK_FORMAT_R8G8B8A8_SRGB,
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(pVulkan, stagingBuffer, pTexture->image, texWidth, texHeight);
    fbrTransitionImageLayout(pVulkan, pTexture->image,
                             VK_FORMAT_R8G8B8A8_SRGB,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(pVulkan->device, stagingBuffer, NULL);
    vkFreeMemory(pVulkan->device, stagingBufferMemory, NULL);
}

void fbrCreateTexture(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, char const *filename, const bool external) {
    *ppAllocTexture = calloc(1, sizeof(FbrTexture));
    FbrTexture *pTexture = *ppAllocTexture;
    createPopulateTexture(pVulkan, pTexture, filename, external);
    createTextureView(pVulkan, pTexture, VK_FORMAT_R8G8B8A8_SRGB);
//    createTextureSampler(pVulkan, pTexture);
}

void fbrImportTexture(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, HANDLE handle) {
    *ppAllocTexture = calloc(1, sizeof(FbrTexture));
    FbrTexture *pTexture = *ppAllocTexture;
    createTextureFromExternal(pVulkan,
                              pTexture,
                              512,
                              512,
                              VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_IMAGE_USAGE_SAMPLED_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              handle);
    createTextureView(pVulkan, pTexture, VK_FORMAT_R8G8B8A8_SRGB);
//    createTextureSampler(pVulkan, pTexture);
}

void fbrCleanupTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture) {
    vkDestroyImage(pVulkan->device, pTexture->image, NULL);
    vkFreeMemory(pVulkan->device, pTexture->deviceMemory, NULL);
//    vkDestroySampler(pVulkan->device, pTexture->sampler, NULL);
    vkDestroyImageView(pVulkan->device, pTexture->imageView, NULL);
    free(pTexture);
}