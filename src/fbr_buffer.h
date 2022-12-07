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

void createBuffer(const FbrAppState* pState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory);

void createUniformBuffers(const FbrAppState *pAppState, UniformBufferObject *pUniformBufferObject, VkDeviceSize bufferSize);

void fbrCleanupBuffers(const FbrAppState *pAppState, UniformBufferObject *pUniformBufferObject);

#endif //FABRIC_BUFFER_H
