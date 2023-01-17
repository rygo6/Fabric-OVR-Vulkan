#ifndef FABRIC_BUFFER_H
#define FABRIC_BUFFER_H

#include "fbr_app.h"

#if WIN32
#include <windows.h>
#endif

typedef struct UniformBufferObject {
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void *pUniformBufferMapped;
#ifdef WIN32
    HANDLE externalMemory;
#endif
} UniformBufferObject;

bool fbrMemoryTypeFromProperties(const VkPhysicalDeviceMemoryProperties memoryProperties,
                                 uint32_t nMemoryTypeBits,
                                 VkMemoryPropertyFlags nMemoryProperties,
                                 uint32_t *pTypeIndexOut);

void fbrImportBuffer(const FbrVulkan *pVulkan,
                     VkDeviceSize size,
                     VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     HANDLE externalMemory,
                     VkBuffer *pBuffer,
                     VkDeviceMemory *pBufferMemory);

VkCommandBuffer fbrBeginBufferCommands(const FbrVulkan *pVulkan);

VkCommandBuffer fbrEndBufferCommands(const FbrVulkan *pVulkan, VkCommandBuffer commandBuffer);

uint32_t fbrFindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void fbrTransitionImageLayout(const FbrVulkan *pVulkan,
                              VkImage image,
                              VkFormat format,
                              VkImageLayout oldLayout,
                              VkImageLayout newLayout);

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

void fbrCreateUniformBuffer(const FbrVulkan *pVulkan, UniformBufferObject *pUniformBufferObject, VkDeviceSize bufferSize);

void fbrImportUniformBuffer(const FbrVulkan *pVulkan,
                            UniformBufferObject *pUniformBufferObject,
                            VkDeviceSize bufferSize,
                            HANDLE externalMemory);

void fbrCreateExternalUniformBuffer(const FbrVulkan *pVulkan, UniformBufferObject *pUniformBufferObject, VkDeviceSize bufferSize);

void fbrCleanupUniformBuffers(const FbrVulkan *pVulkan, UniformBufferObject *pUniformBufferObject);

#endif //FABRIC_BUFFER_H
