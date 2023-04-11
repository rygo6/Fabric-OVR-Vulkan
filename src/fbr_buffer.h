#ifndef FABRIC_BUFFER_H
#define FABRIC_BUFFER_H

#include "fbr_app.h"

#if WIN32
#include <windows.h>
#endif

#define FBR_NO_DYNAMIC_BUFFER 0

#define FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE\
.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,\
.subresourceRange.baseMipLevel = 0,\
.subresourceRange.levelCount = 1,\
.subresourceRange.baseArrayLayer = 0,\
.subresourceRange.layerCount = 1,\

#define FBR_DEFAULT_DEPTH_SUBRESOURCE_RANGE\
.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,\
.subresourceRange.baseMipLevel = 0,\
.subresourceRange.levelCount = 1,\
.subresourceRange.baseArrayLayer = 0,\
.subresourceRange.layerCount = 1,\


typedef struct FbrUniformBufferObject {
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void *pUniformBufferMapped;
    uint32_t bufferSize;
    uint32_t dynamicAlignment;
    uint32_t dynamicCount;
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

VkResult fbrBeginImmediateCommandBuffer(const FbrVulkan *pVulkan, VkCommandBuffer *pCommandBuffer);

VkResult fbrEndImmediateCommandBuffer(const FbrVulkan *pVulkan, VkCommandBuffer *pCommandBuffer);

void fbrTransitionImageLayoutImmediate(const FbrVulkan *pVulkan,
                                       VkImage image,
                                       VkImageLayout oldLayout,
                                       VkImageLayout newLayout,
                                       VkAccessFlags srcAccessMask,
                                       VkAccessFlags dstAccessMask,
                                       VkPipelineStageFlags srcStageMask,
                                       VkPipelineStageFlags dstStageMask,
                                       VkImageAspectFlags aspectMask);

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

VkResult fbrCreateUBO(const FbrVulkan *pVulkan,
                      VkMemoryPropertyFlags properties,
                      VkBufferUsageFlags usage,
                      VkDeviceSize bufferSize,
                      uint32_t dynamicCount,
                      bool external,
                      FbrUniformBufferObject **ppAllocUBO);

void fbrImportUBO(const FbrVulkan *pVulkan,
                  VkMemoryPropertyFlags properties,
                  VkBufferUsageFlags usage,
                  VkDeviceSize bufferSize,
                  uint32_t dynamicCount,
                  HANDLE externalMemory,
                  FbrUniformBufferObject **ppAllocUBO);

void fbrDestroyUBO(const FbrVulkan *pVulkan, FbrUniformBufferObject *pUBO);

#endif //FABRIC_BUFFER_H
