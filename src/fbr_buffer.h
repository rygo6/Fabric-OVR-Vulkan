#ifndef FABRIC_BUFFER_H
#define FABRIC_BUFFER_H

#include "fbr_app.h"

#if WIN32
#include <windows.h>
#endif

typedef struct FbrUniformBufferObject {
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void *pUniformBufferMapped;
#ifdef WIN32
    HANDLE externalMemory;
#endif
} FbrUniformBufferObject;

VkResult fbrImageMemoryTypeFromProperties(const FbrVulkan *pVulkan,
                                          VkImage image,
                                          VkMemoryPropertyFlags properties,
                                          VkMemoryRequirements *pMemRequirements,
                                          uint32_t *pMemoryTypeBits);

VkResult fbrBufferMemoryTypeFromProperties(const FbrVulkan *pVulkan,
                                           VkBuffer image,
                                           VkMemoryPropertyFlags properties,
                                           VkMemoryRequirements *pMemRequirements,
                                           uint32_t *pMemoryTypeBits);

void importBuffer(const FbrVulkan *pVulkan,
                  VkDeviceSize size,
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  HANDLE externalMemory,
                  VkBuffer *pBuffer,
                  VkDeviceMemory *pBufferMemory);

VkResult fbrBeginImmediateCommandBuffer(const FbrVulkan *pVulkan, VkCommandBuffer *pCommandBuffer);

VkResult fbrEndImmediateCommandBuffer(const FbrVulkan *pVulkan, VkCommandBuffer *pCommandBuffer);

void fbrTransitionImageLayoutImmediate(const FbrVulkan *pVulkan,
                                       VkImage image,
                                       VkImageLayout oldLayout,
                                       VkImageLayout newLayout,
                                       VkAccessFlags srcAccessMask,
                                       VkAccessFlags dstAccessMask,
                                       VkPipelineStageFlags srcStageMask,
                                       VkPipelineStageFlags dstStageMask);

void fbrCopyBuffer(const FbrVulkan *pVulkan, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void fbrMemCopyMappedUBO(const FbrUniformBufferObject *pDstUBO, const void* pSrcData, size_t size);

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

void fbrCreateUBO(const FbrVulkan *pVulkan,
                  VkMemoryPropertyFlags properties,
                  VkBufferUsageFlags usage,
                  VkDeviceSize bufferSize,
                  FbrUniformBufferObject **ppAllocUBO);

void fbrImportUBO(const FbrVulkan *pVulkan,
                  VkDeviceSize bufferSize,
                  HANDLE externalMemory,
                  FbrUniformBufferObject **ppAllocUBO);

void fbrCreateExternalUBO(const FbrVulkan *pVulkan,
                          VkDeviceSize bufferSize,
                          FbrUniformBufferObject **ppAllocUBO);

void fbrDestroyUBO(const FbrVulkan *pVulkan, FbrUniformBufferObject *pUBO);

#endif //FABRIC_BUFFER_H
