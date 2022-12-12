//
// Created by rygo6 on 11/29/2022.
//

#ifndef FABRIC_BUFFER_H
#define FABRIC_BUFFER_H

#include "fbr_app.h"

typedef struct UniformBufferObject {
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* pUniformBufferMapped;
} UniformBufferObject;

VkCommandBuffer fbrBeginBufferCommands(const FbrAppState *pAppState);

VkCommandBuffer fbrEndBufferCommands(const FbrAppState *pAppState, VkCommandBuffer commandBuffer);

uint32_t fbrFindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void fbrCopyBuffer(const FbrAppState *pAppState, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void fbrCreateBuffer(const FbrAppState* pState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory);

void fbrCreateStagingBuffer(const FbrAppState* pAppState,
                            const void* srcData,
                            VkBuffer* stagingBuffer,
                            VkDeviceMemory* stagingBufferMemory,
                            VkDeviceSize bufferSize);

void fbrCreatePopulateBufferViaStaging(const FbrAppState *restrict pAppState,
                                       const void *restrict srcData,
                                       VkBufferUsageFlagBits usage,
                                       VkBuffer *restrict buffer,
                                       VkDeviceMemory *restrict bufferMemory,
                                       VkDeviceSize bufferSize);

void fbrCreateUniformBuffers(const FbrAppState *pAppState, UniformBufferObject *pUniformBufferObject, VkDeviceSize bufferSize);

void fbrCleanupBuffers(const FbrAppState *pAppState, UniformBufferObject *pUniformBufferObject);

#endif //FABRIC_BUFFER_H
