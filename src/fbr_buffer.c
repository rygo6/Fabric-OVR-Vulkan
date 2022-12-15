#include "fbr_buffer.h"
#include <stdio.h>
#include <memory.h>

uint32_t fbrFindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    printf("%s - failed to find suitable memory type!\n", __FUNCTION__);
}

void fbrCreateBuffer(const FbrApp *pApp, VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
    VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(pApp->device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        printf("%s - failed to create buffer!\n", __FUNCTION__);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(pApp->device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = fbrFindMemoryType(pApp->physicalDevice, memRequirements.memoryTypeBits, properties),
    };

    // this need to be made to vulkan memory allocator, read conclusion https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
    if (vkAllocateMemory(pApp->device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
        printf("%s - failed to allocate buffer memory!\n", __FUNCTION__);
    }

    vkBindBufferMemory(pApp->device, *buffer, *bufferMemory, 0);
}

VkCommandBuffer fbrBeginBufferCommands(const FbrApp *pApp) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool =  pApp->commandPool,
            .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(pApp->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

VkCommandBuffer fbrEndBufferCommands(const FbrApp *pApp, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
    };

    vkQueueSubmit(pApp->queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(
            pApp->queue); // could be more optimized with vkWaitForFences https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

    vkFreeCommandBuffers(pApp->device, pApp->commandPool, 1, &commandBuffer);
}

void fbrCopyBuffer(const FbrApp *pApp, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = fbrBeginBufferCommands(pApp);

    VkBufferCopy copyRegion = {
            .srcOffset = 0, // Optional
            .dstOffset = 0, // Optional
            .size = size,
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    fbrEndBufferCommands(pApp, commandBuffer);
}

void fbrCreateStagingBuffer(const FbrApp *pApp,
                            const void *srcData,
                            VkBuffer *stagingBuffer,
                            VkDeviceMemory *stagingBufferMemory,
                            VkDeviceSize bufferSize) {
    fbrCreateBuffer(pApp,
                    bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer,
                    stagingBufferMemory);

    void *dstData;
    vkMapMemory(pApp->device, *stagingBufferMemory, 0, bufferSize, 0, &dstData);
    memcpy(dstData, srcData, bufferSize);
    vkUnmapMemory(pApp->device, *stagingBufferMemory);
}

void fbrCreatePopulateBufferViaStaging(const FbrApp *pApp,
                                       const void *srcData,
                                       VkBufferUsageFlagBits usage,
                                       VkBuffer *buffer,
                                       VkDeviceMemory *bufferMemory,
                                       VkDeviceSize bufferSize) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    fbrCreateStagingBuffer(pApp,
                           srcData,
                           &stagingBuffer,
                           &stagingBufferMemory,
                           bufferSize);

    fbrCreateBuffer(pApp,
                    bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    buffer,
                    bufferMemory);

    fbrCopyBuffer(pApp, stagingBuffer, *buffer, bufferSize);

    vkDestroyBuffer(pApp->device, stagingBuffer, NULL);
    vkFreeMemory(pApp->device, stagingBufferMemory, NULL);
}

void fbrCreateUniformBuffers(const FbrApp *pApp, UniformBufferObject *pUniformBufferObject, VkDeviceSize bufferSize) {
    fbrCreateBuffer(pApp, bufferSize,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &pUniformBufferObject->uniformBuffer,
                    &pUniformBufferObject->uniformBufferMemory);
    vkMapMemory(pApp->device,
                pUniformBufferObject->uniformBufferMemory,
                0,
                bufferSize,
                0,
                &pUniformBufferObject->pUniformBufferMapped);
}

void fbrCleanupUniformBuffers(const FbrApp *pApp, UniformBufferObject *pUniformBufferObject) {
    vkUnmapMemory(pApp->device, pUniformBufferObject->uniformBufferMemory);
    vkDestroyBuffer(pApp->device, pUniformBufferObject->uniformBuffer, NULL);
    vkFreeMemory(pApp->device, pUniformBufferObject->uniformBufferMemory, NULL);
}

void updateUniformBuffer(const FbrApp *pApp, UniformBufferObject *pUniformBufferObject, void *data, int dataSize) {
    memcpy(pUniformBufferObject->pUniformBufferMapped, data, dataSize);
}