#include "mxc_buffer.h"
#include <stdio.h>
#include <memory.h>

static uint32_t findMemoryType(const MxcAppState* pState, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(pState->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    printf("%s - failed to find suitable memory type!\n", __FUNCTION__);
}

void createBuffer(const MxcAppState* pState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
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

    if (vkAllocateMemory(pState->device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
        printf("%s - failed to allocate buffer memory!\n", __FUNCTION__);
    }

    vkBindBufferMemory(pState->device, *buffer, *bufferMemory, 0);
}

void createUniformBuffers(const MxcAppState *pAppState, UniformBufferObject *pUniformBufferObject, VkDeviceSize bufferSize) {
    createBuffer(pAppState, bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &pUniformBufferObject->uniformBuffer,
                 &pUniformBufferObject->uniformBufferMemory);
    vkMapMemory(pAppState->device, pUniformBufferObject->uniformBufferMemory, 0, bufferSize, 0,&pUniformBufferObject->pUniformBufferMapped);
}

void mxcCleanupBuffers(const MxcAppState *pAppState, UniformBufferObject *pUniformBufferObject) {
    vkUnmapMemory(pAppState->device, pUniformBufferObject->uniformBufferMemory);
    vkDestroyBuffer(pAppState->device, pUniformBufferObject->uniformBuffer, NULL);
    vkFreeMemory(pAppState->device, pUniformBufferObject->uniformBufferMemory, NULL);
}

void updateUniformBuffer(const MxcAppState *pAppState, UniformBufferObject *pUniformBufferObject, void *data, int dataSize) {
    memcpy(pUniformBufferObject->pUniformBufferMapped, data, dataSize);
}