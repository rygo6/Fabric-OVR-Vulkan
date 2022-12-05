//
// Created by rygo6 on 11/29/2022.
//

#ifndef MOXAIC_MXC_BUFFER_H
#define MOXAIC_MXC_BUFFER_H

#include "mxc_app.h"

typedef struct UniformBufferObject {
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* pUniformBufferMapped;
} UniformBufferObject;

void createBuffer(MxcAppState* pState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory);

void createUniformBuffers(MxcAppState *pAppState, UniformBufferObject *pUniformBufferObject, VkDeviceSize bufferSize);

void mxcCleanupBuffers(MxcAppState *pAppState, UniformBufferObject *pUniformBufferObject);

#endif //MOXAIC_MXC_BUFFER_H
