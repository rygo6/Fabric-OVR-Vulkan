#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

#include "fbr_texture.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

void copyBufferToImage(const FbrApp *pApp,
                       VkBuffer buffer,
                       VkImage image,
                       uint32_t width,
                       uint32_t height) {
    VkCommandBuffer commandBuffer = fbrBeginBufferCommands(pApp);

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

    fbrEndBufferCommands(pApp, commandBuffer);
}

void transitionImageLayout(const FbrApp *pApp,
                           VkImage image,
                           VkFormat format,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = fbrBeginBufferCommands(pApp);

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
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
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

    fbrEndBufferCommands(pApp, commandBuffer);
}

void createTexture(const FbrApp *pApp,
                   uint32_t width,
                   uint32_t height,
                   VkFormat format,
                   VkImageTiling tiling,
                   VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties,
                   VkImage *image,
                   VkDeviceMemory *imageMemory) {

    VkImageCreateInfo imageInfo = {
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

    if (vkCreateImage(pApp->device, &imageInfo, NULL, image) != VK_SUCCESS) {
        FBR_LOG_DEBUG("%s - failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(pApp->device, *image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = fbrFindMemoryType(pApp->physicalDevice, memRequirements.memoryTypeBits, properties),
    };

    if (vkAllocateMemory(pApp->device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
        FBR_LOG_DEBUG("%s - failed to allocate image memory!");
    }

    vkBindImageMemory(pApp->device, *image, *imageMemory, 0);
}

void createTextureView(const FbrApp *pApp, FbrTexture *pTexture, VkFormat format) {
    VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = pTexture->texture,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
    };

    if (vkCreateImageView(pApp->device, &viewInfo, NULL, &pTexture->textureView) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create texture image view!");
    }
}

void createTextureSampler(const FbrApp *pApp, FbrTexture *pTexture){
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(pApp->physicalDevice, &properties);
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

    if (vkCreateSampler(pApp->device, &samplerInfo, NULL, &pTexture->textureSampler) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to create texture sampler!");
    }
}

void createPopulateTexture(const FbrApp *pApp, FbrTexture *pTexture) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load("textures/test.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageBufferSize = texWidth * texHeight * 4;

    if (!pixels) {
        FBR_LOG_DEBUG("Failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    fbrCreateStagingBuffer(pApp,
                           pixels,
                           &stagingBuffer,
                           &stagingBufferMemory,
                           imageBufferSize);

    stbi_image_free(pixels);

    createTexture(pApp,
                  texWidth,
                  texHeight,
                  VK_FORMAT_R8G8B8A8_SRGB,
                  VK_IMAGE_TILING_OPTIMAL,
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  &pTexture->texture,
                  &pTexture->textureMemory);

    transitionImageLayout(pApp,
                          pTexture->texture,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(pApp, stagingBuffer, pTexture->texture, texWidth, texHeight);
    transitionImageLayout(pApp, pTexture->texture,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(pApp->device, stagingBuffer, NULL);
    vkFreeMemory(pApp->device, stagingBufferMemory, NULL);
}

void fbrCreateTexture(const FbrApp *pApp, FbrTexture **ppAllocTexture) {
    *ppAllocTexture = calloc(1, sizeof(FbrTexture));
    FbrTexture *pTexture = *ppAllocTexture;
    createPopulateTexture(pApp, pTexture);
    createTextureView(pApp, pTexture, VK_FORMAT_R8G8B8A8_SRGB);
    createTextureSampler(pApp, pTexture);
}

void fbrCleanupTexture(const FbrApp *pApp, FbrTexture *pTexture) {
    vkDestroyImage(pApp->device, pTexture->texture, NULL);
    vkFreeMemory(pApp->device, pTexture->textureMemory, NULL);
    vkDestroySampler(pApp->device, pTexture->textureSampler, NULL);
    vkDestroyImageView(pApp->device, pTexture->textureView, NULL);
    free(pTexture);
}