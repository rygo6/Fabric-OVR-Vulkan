#include "fbr_buffer.h"
#include <stdio.h>
#include <memory.h>

static uint32_t findMemoryType(const FbrAppState* pState, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(pState->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    printf("%s - failed to find suitable memory type!\n", __FUNCTION__);
}

void fbrCreateBuffer(const FbrAppState* pState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
    VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(pState->device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        printf("%s - failed to create buffer!\n", __FUNCTION__);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(pState->device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType(pState, memRequirements.memoryTypeBits, properties),
    };

    // this need to be made to vulkan memory allocator, read conclusion https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
    if (vkAllocateMemory(pState->device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
        printf("%s - failed to allocate buffer memory!\n", __FUNCTION__);
    }

    vkBindBufferMemory(pState->device, *buffer, *bufferMemory, 0);
}

void fbrCopyBuffer(const FbrAppState *pAppState, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool = pAppState->commandPool,
            .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(pAppState->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion= {
            .srcOffset = 0, // Optional
            .dstOffset = 0, // Optional
            .size = size,
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
    };

    vkQueueSubmit(pAppState->queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(pAppState->queue); // could be more optimized with vkWaitForFences https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

    vkFreeCommandBuffers(pAppState->device, pAppState->commandPool, 1, &commandBuffer);
}

void fbrCreateUniformBuffers(const FbrAppState *pAppState, UniformBufferObject *pUniformBufferObject, VkDeviceSize bufferSize) {
    fbrCreateBuffer(pAppState, bufferSize,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &pUniformBufferObject->uniformBuffer,
                    &pUniformBufferObject->uniformBufferMemory);
    vkMapMemory(pAppState->device, pUniformBufferObject->uniformBufferMemory, 0, bufferSize, 0,&pUniformBufferObject->pUniformBufferMapped);
}

void fbrCleanupBuffers(const FbrAppState *pAppState, UniformBufferObject *pUniformBufferObject) {
    vkUnmapMemory(pAppState->device, pUniformBufferObject->uniformBufferMemory);
    vkDestroyBuffer(pAppState->device, pUniformBufferObject->uniformBuffer, NULL);
    vkFreeMemory(pAppState->device, pUniformBufferObject->uniformBufferMemory, NULL);
}

void updateUniformBuffer(const FbrAppState *pAppState, UniformBufferObject *pUniformBufferObject, void *data, int dataSize) {
    memcpy(pUniformBufferObject->pUniformBufferMapped, data, dataSize);
}