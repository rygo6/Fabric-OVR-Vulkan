#ifndef FABRIC_BUFFER_H
#define FABRIC_BUFFER_H

#include "fbr_app.h"

typedef struct UniformBufferObject {
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void *pUniformBufferMapped;
} UniformBufferObject;

VkCommandBuffer fbrBeginBufferCommands(const FbrVulkan *pVulkan);

VkCommandBuffer fbrEndBufferCommands(const FbrVulkan *pVulkan, VkCommandBuffer commandBuffer);

uint32_t fbrFindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void fbrCopyBuffer(const FbrVulkan *pVulkan, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void fbrCreateBuffer(const FbrVulkan *pVulkan,
                     VkDeviceSize size,
                     VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkBuffer *buffer,
                     VkDeviceMemory *bufferMemory);

void fbrCreateStagingBuffer(const FbrVulkan *pVulkan,
                            const void *srcData,
                            VkBuffer *stagingBuffer,
                            VkDeviceMemory *stagingBufferMemory,
                            VkDeviceSize bufferSize);

void fbrCreatePopulateBufferViaStaging(const FbrVulkan *pVulkan,
                                       const void *srcData,
                                       VkBufferUsageFlagBits usage,
                                       VkBuffer *buffer,
                                       VkDeviceMemory *bufferMemory,
                                       VkDeviceSize bufferSize);

void fbrCreateUniformBuffers(const FbrVulkan *pVulkan, UniformBufferObject *pUniformBufferObject, VkDeviceSize bufferSize);

void fbrCleanupUniformBuffers(const FbrVulkan *pVulkan, UniformBufferObject *pUniformBufferObject);

#endif //FABRIC_BUFFER_H
