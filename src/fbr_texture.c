#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "fbr_texture.h"
#include "fbr_buffer.h"

void copyBufferToImage(FBR_APP_PARAM,
                       VkBuffer buffer,
                       VkImage image,
                       uint32_t width,
                       uint32_t height) {
    VkCommandBuffer commandBuffer = fbrBeginBufferCommands(pAppState);

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

    fbrEndBufferCommands(pAppState, commandBuffer);
}

void transitionImageLayout(FBR_APP_PARAM,
                           VkImage image,
                           VkFormat format,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = fbrBeginBufferCommands(pAppState);

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
    } else {
        printf( "%s - unsupported layout transition!\n", __FUNCTION__ );
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,NULL,
            0,NULL,
            1,&barrier
    );

    fbrEndBufferCommands(pAppState, commandBuffer);
}

void createImage(FBR_APP_PARAM,
                 uint32_t width,
                 uint32_t height,
                 VkFormat format,
                 VkImageTiling tiling,
                 VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 VkImage *restrict image,
                 VkDeviceMemory *restrict imageMemory) {

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

    if (vkCreateImage(pAppState->device, &imageInfo, NULL, image) != VK_SUCCESS) {
        printf( "%s - failed to create image!\n", __FUNCTION__ );
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(pAppState->device, *image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = fbrFindMemoryType(pAppState->physicalDevice, memRequirements.memoryTypeBits, properties),
    };

    if (vkAllocateMemory(pAppState->device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
        printf( "%s - failed to allocate image memory!\n", __FUNCTION__ );
    }

    vkBindImageMemory(pAppState->device, *image, *imageMemory, 0);
}

void fbrCreateTexture(FBR_APP_PARAM, FbrTexture** ppAllocTexture) {
    *ppAllocTexture = calloc(1, sizeof(FbrTexture));
    FbrTexture *pTexture = *ppAllocTexture;

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/test.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageBufferSize = texWidth * texHeight * 4;

    if (!pixels) {
        printf( "%s - failed to load texture image!\n", __FUNCTION__ );
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    fbrCreateStagingBuffer(pAppState,
                           pixels,
                           &stagingBuffer,
                           &stagingBufferMemory,
                           imageBufferSize);

    stbi_image_free(pixels);

    createImage(pAppState,
            texWidth,
                texHeight,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &pTexture->image,
                &pTexture->imageMemory);


    transitionImageLayout(pAppState, pTexture->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(pAppState, stagingBuffer, pTexture->image, texWidth, texHeight);
    transitionImageLayout(pAppState, pTexture->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(pAppState->device, stagingBuffer, NULL);
    vkFreeMemory(pAppState->device, stagingBufferMemory, NULL);
}

void fbrCleanupTexture(FBR_APP_PARAM, FbrTexture *restrict pTexture) {
    vkDestroyImage(pAppState->device, pTexture->image, NULL);
    vkFreeMemory(pAppState->device, pTexture->imageMemory, NULL);
    free(pTexture);
}