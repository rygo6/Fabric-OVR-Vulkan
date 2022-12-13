//
// Created by rygo6 on 11/29/2022.
//

#ifndef FABRIC_BUFFER_H
#define FABRIC_BUFFER_H

#include "fbr_app.h"

typedef struct UniformBufferObject {
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void *pUniformBufferMapped;
} UniformBufferObject;

VkCommandBuffer fbrBeginBufferCommands(const FbrApp *pApp);

VkCommandBuffer fbrEndBufferCommands(const FbrApp *pApp, VkCommandBuffer commandBuffer);

uint32_t fbrFindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void fbrCopyBuffer(const FbrApp *pApp, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void fbrCreateBuffer(const FbrApp *pState, VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory);

void fbrCreateStagingBuffer(const FbrApp *pApp,
                            const void *srcData,
                            VkBuffer *stagingBuffer,
                            VkDeviceMemory *stagingBufferMemory,
                            VkDeviceSize bufferSize);

void fbrCreatePopulateBufferViaStaging(const FbrApp *pApp,
                                       const void *srcData,
                                       VkBufferUsageFlagBits usage,
                                       VkBuffer *buffer,
                                       VkDeviceMemory *bufferMemory,
                                       VkDeviceSize bufferSize);

void
fbrCreateUniformBuffers(const FbrApp *pApp, UniformBufferObject *pUniformBufferObject, VkDeviceSize bufferSize);

void fbrCleanupBuffers(const FbrApp *pApp, UniformBufferObject *pUniformBufferObject);

#endif //FABRIC_BUFFER_H
