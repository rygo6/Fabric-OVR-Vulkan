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

static void transitionImageLayout(const FbrVulkan *pVulkan,
                           VkImage image,
                           VkFormat format,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = fbrBeginBufferCommands(pVulkan);

    VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
            .srcAccessMask = 0, // TODO
            .dstAccessMask = 0, // TODO
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        FBR_LOG_DEBUG("%s - unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0, NULL,
            0, NULL,
            1, &barrier
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
                          HANDLE handle) {

    VkExternalMemoryImageCreateInfo externalImageInfo = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
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

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(pVulkan->device, pTexture->image, &memRequirements);

    VkImportMemoryWin32HandleInfoKHR importInfo = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
            .handle = handle,
    };

    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = fbrFindMemoryType(pVulkan->physicalDevice, memRequirements.memoryTypeBits, properties),
            .pNext = &importInfo
    };

    if (vkAllocateMemory(pVulkan->device, &allocInfo, NULL, &pTexture->deviceMemory) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to allocate image memory!");
    }

    vkBindImageMemory(pVulkan->device, pTexture->image, pTexture->deviceMemory, 0);

    transitionImageLayout(pVulkan, pTexture->image,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

static void createTexture(const FbrVulkan *pVulkan,
                   FbrTexture *pTexture,
                   uint32_t width,
                   uint32_t height,
                   VkFormat format,
                   VkImageTiling tiling,
                   VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties,
                   bool external) {

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

    if (external) {
        VkExternalMemoryImageCreateInfo externalImageInfo = {
                .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
                .pNext = VK_NULL_HANDLE,
                .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
        };
        imageCreateInfo.pNext = &externalImageInfo;
    }

    if (vkCreateImage(pVulkan->device, &imageCreateInfo, NULL, &pTexture->image) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(pVulkan->device, pTexture->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = fbrFindMemoryType(pVulkan->physicalDevice, memRequirements.memoryTypeBits, properties),
    };

    if (external) {
        VkExportMemoryAllocateInfo exportAllocInfo = {
                VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
                NULL,
                VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
        };
        allocInfo.pNext = &exportAllocInfo;
    }

    if (vkAllocateMemory(pVulkan->device, &allocInfo, NULL, &pTexture->deviceMemory) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to allocate image memory!");
    }

    vkBindImageMemory(pVulkan->device, pTexture->image, pTexture->deviceMemory, 0);

    if (external) {
#if WIN32
        VkMemoryGetWin32HandleInfoKHR memoryInfo = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
                .pNext =   NULL,
                .memory = pTexture->deviceMemory,
                .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
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

static void createTextureSampler(const FbrVulkan *pVulkan, FbrTexture *pTexture){
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

    if (vkCreateSampler(pVulkan->device, &samplerInfo, NULL, &pTexture->sampler) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to create texture sampler!");
    }
}

static void createPopulateTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture, char const *filename, const bool external) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageBufferSize = texWidth * texHeight * 4;

    FBR_LOG_DEBUG("Loading Texture", filename, texWidth, texHeight, texChannels)

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

    createTexture(pVulkan,
                  pTexture,
                  texWidth,
                  texHeight,
                  VK_FORMAT_R8G8B8A8_SRGB,
                  VK_IMAGE_TILING_OPTIMAL,
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  external);

    transitionImageLayout(pVulkan,
                          pTexture->image,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(pVulkan, stagingBuffer, pTexture->image, texWidth, texHeight);
    transitionImageLayout(pVulkan, pTexture->image,
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
    createTextureSampler(pVulkan, pTexture);
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
    createTextureSampler(pVulkan, pTexture);
}

void fbrCleanupTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture) {
    vkDestroyImage(pVulkan->device, pTexture->image, NULL);
    vkFreeMemory(pVulkan->device, pTexture->deviceMemory, NULL);
    vkDestroySampler(pVulkan->device, pTexture->sampler, NULL);
    vkDestroyImageView(pVulkan->device, pTexture->imageView, NULL);
    free(pTexture);
}