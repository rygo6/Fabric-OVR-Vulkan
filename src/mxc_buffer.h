//
// Created by rygo6 on 11/29/2022.
//

#ifndef MOXAIC_MXC_BUFFER_H
#define MOXAIC_MXC_BUFFER_H

#include "mxc_app.h"

void createBuffer(mxcAppState* pState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory);

#endif //MOXAIC_MXC_BUFFER_H
