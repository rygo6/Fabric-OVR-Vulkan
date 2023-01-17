#include "fbr_buffer.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"

#if WIN32
#include <vulkan/vulkan_win32.h>
#endif

// From OVR Vulkan example. Is this better/same as vulkan tutorial!?
static VkResult memoryTypeFromProperties(const VkPhysicalDeviceMemoryProperties memoryProperties,
                                         uint32_t memoryTypeBits,
                                         VkMemoryPropertyFlags properties,
                                         uint32_t *pMemoryTypeBits) {
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((memoryTypeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                *pMemoryTypeBits = i;
                return VK_SUCCESS;
            }
        }
        memoryTypeBits >>= 1;
    }

    FBR_LOG_DEBUG("Can't find memory type.", properties, properties);
    return VK_ERROR_FORMAT_NOT_SUPPORTED;
}

VkResult fbrImageMemoryTypeFromProperties(const FbrVulkan *pVulkan,
                                          VkImage image,
                                          VkMemoryPropertyFlags properties,
                                          VkMemoryRequirements *pMemRequirements,
                                          uint32_t *pMemoryTypeBits) {
    vkGetImageMemoryRequirements(pVulkan->device, image, pMemRequirements);
    return memoryTypeFromProperties(pVulkan->memProperties,
                                    pMemRequirements->memoryTypeBits,
                                    properties,
                                    pMemoryTypeBits);
}

VkResult fbrBufferMemoryTypeFromProperties(const FbrVulkan *pVulkan,
                                           VkBuffer buffer,
                                           VkMemoryPropertyFlags properties,
                                           VkMemoryRequirements *pMemRequirements,
                                           uint32_t *pMemoryTypeBits) {
    vkGetBufferMemoryRequirements(pVulkan->device, buffer, pMemRequirements);
    return memoryTypeFromProperties(pVulkan->memProperties,
                                    pMemRequirements->memoryTypeBits,
                                    properties,
                                    pMemoryTypeBits);
}

void fbrImportBuffer(const FbrVulkan *pVulkan,
                     VkDeviceSize size,
                     VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     HANDLE externalMemory,
                     VkBuffer *pBuffer,
                     VkDeviceMemory *pBufferMemory) {

//    VkExternalMemoryHandleTypeFlagsKHR externalHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;
    VkExternalMemoryHandleTypeFlagsKHR externalHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
    VkExternalMemoryBufferCreateInfoKHR externalMemoryBufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR,
            .pNext = VK_NULL_HANDLE,
            .handleTypes = externalHandleType
    };
    VkBufferCreateInfo bufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = &externalMemoryBufferCreateInfo,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    FBR_VK_CHECK(vkCreateBuffer(pVulkan->device, &bufferCreateInfo, NULL, pBuffer));

    VkMemoryRequirements memRequirements = {};
    uint32_t memTypeIndex;
    FBR_VK_CHECK(fbrBufferMemoryTypeFromProperties(pVulkan,
                                                   *pBuffer,
                                                   properties,
                                                   &memRequirements,
                                                   &memTypeIndex));

    // dedicated?
//    VkMemoryDedicatedAllocateInfoKHR dedicatedAllocInfo = {
//            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
//            .pNext = VK_NULL_HANDLE,
//            .image = pTexture->image,
//            .buffer = VK_NULL_HANDLE
//    };
    VkImportMemoryWin32HandleInfoKHR importMemoryInfo = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
//            .pNext = &dedicatedAllocInfo,
            .handleType = externalHandleType,
            .handle = externalMemory,
    };
    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memTypeIndex,
            .pNext = &importMemoryInfo
    };
    FBR_VK_CHECK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, pBufferMemory));

    FBR_VK_CHECK(vkBindBufferMemory(pVulkan->device, *pBuffer, *pBufferMemory, 0));
}

void fbrCreateExternalBuffer(const FbrVulkan *pVulkan,
                             VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags properties,
                             VkBuffer *pBuffer,
                             VkDeviceMemory *pBufferMemory,
                             HANDLE *pSharedMemory) {

//    VkExternalMemoryHandleTypeFlagsKHR externalHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;
    VkExternalMemoryHandleTypeFlagsKHR externalHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
    VkExternalMemoryBufferCreateInfoKHR externalMemoryBufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR,
            .pNext = VK_NULL_HANDLE,
            .handleTypes = externalHandleType
    };
    VkBufferCreateInfo bufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = &externalMemoryBufferCreateInfo,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    FBR_VK_CHECK(vkCreateBuffer(pVulkan->device, &bufferCreateInfo, NULL, pBuffer));

    VkMemoryRequirements memRequirements = {};
    uint32_t memTypeIndex;
    FBR_VK_CHECK(fbrBufferMemoryTypeFromProperties(pVulkan,
                                                   *pBuffer,
                                                   properties,
                                                   &memRequirements,
                                                   &memTypeIndex));

    // dedicated needed??
//    VkMemoryDedicatedAllocateInfoKHR dedicatedAllocInfo = {
//            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
//            .image = VK_NULL_HANDLE,
//            .buffer = *pRingBuffer,
//    };
    VkExportMemoryAllocateInfo exportAllocInfo = {
            .sType =VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
//            .pNext = &dedicatedAllocInfo,
            .handleTypes = externalHandleType
    };
    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memTypeIndex,
            .pNext = &exportAllocInfo
    };
    FBR_VK_CHECK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, pBufferMemory));

    FBR_VK_CHECK(vkBindBufferMemory(pVulkan->device, *pBuffer, *pBufferMemory, 0));

    VkMemoryGetWin32HandleInfoKHR memoryInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
            .pNext = NULL,
            .memory = *pBufferMemory,
            .handleType = externalHandleType
    };
    PFN_vkGetMemoryWin32HandleKHR getMemoryWin32HandleFunc = (PFN_vkGetMemoryWin32HandleKHR) vkGetInstanceProcAddr(pVulkan->instance, "vkGetMemoryWin32HandleKHR");
    if (getMemoryWin32HandleFunc == NULL) {
        FBR_LOG_DEBUG("Failed to get PFN_vkGetMemoryWin32HandleKHR!");
    }
    FBR_VK_CHECK(getMemoryWin32HandleFunc(pVulkan->device, &memoryInfo, pSharedMemory));
}

void fbrCreateBuffer(const FbrVulkan *pVulkan,
                     VkDeviceSize size,
                     VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkBuffer *pBuffer,
                     VkDeviceMemory *bufferMemory) {
    VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(pVulkan->device, &bufferInfo, NULL, pBuffer) != VK_SUCCESS) {
        printf("%s - failed to create buffer!\n", __FUNCTION__);
    }

    VkMemoryRequirements memRequirements = {};
    uint32_t memTypeIndex;
    FBR_VK_CHECK(fbrBufferMemoryTypeFromProperties(pVulkan,
                                                   *pBuffer,
                                                   properties,
                                                   &memRequirements,
                                                   &memTypeIndex));

    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memTypeIndex
    };
    FBR_VK_CHECK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, bufferMemory));

    vkBindBufferMemory(pVulkan->device, *pBuffer, *bufferMemory, 0);
}

VkCommandBuffer fbrBeginBufferCommands(const FbrVulkan *pVulkan) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool =  pVulkan->commandPool,
            .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(pVulkan->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

VkCommandBuffer fbrEndBufferCommands(const FbrVulkan *pVulkan, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
    };

    vkQueueSubmit(pVulkan->queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(pVulkan->queue); // could be more optimized with vkWaitForFences https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

    vkFreeCommandBuffers(pVulkan->device, pVulkan->commandPool, 1, &commandBuffer);
}

void fbrTransitionImageLayout(const FbrVulkan *pVulkan,
                              VkImage image,
                              VkFormat format,
                              VkImageLayout oldLayout,
                              VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = fbrBeginBufferCommands(pVulkan);

    VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
            .srcAccessMask = 0, // TODO
            .dstAccessMask = 0, // TODO
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        FBR_LOG_DEBUG("%s - unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0, NULL,
            0, NULL,
            1, &barrier
    );

    fbrEndBufferCommands(pVulkan, commandBuffer);
}

void fbrCopyBuffer(const FbrVulkan *pVulkan, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = fbrBeginBufferCommands(pVulkan);

    VkBufferCopy copyRegion = {
            .srcOffset = 0, // Optional
            .dstOffset = 0, // Optional
            .size = size,
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    fbrEndBufferCommands(pVulkan, commandBuffer);
}

void fbrCreateStagingBuffer(const FbrVulkan *pVulkan,
                            const void *srcData,
                            VkBuffer *stagingBuffer,
                            VkDeviceMemory *stagingBufferMemory,
                            VkDeviceSize bufferSize) {
    fbrCreateBuffer(pVulkan,
                    bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer,
                    stagingBufferMemory);

    void *dstData;
    vkMapMemory(pVulkan->device, *stagingBufferMemory, 0, bufferSize, 0, &dstData);
    memcpy(dstData, srcData, bufferSize);
    vkUnmapMemory(pVulkan->device, *stagingBufferMemory);
}

void fbrCreatePopulateBufferViaStaging(const FbrVulkan *pVulkan,
                                       const void *srcData,
                                       VkBufferUsageFlagBits usage,
                                       VkBuffer *buffer,
                                       VkDeviceMemory *bufferMemory,
                                       VkDeviceSize bufferSize) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    fbrCreateStagingBuffer(pVulkan,
                           srcData,
                           &stagingBuffer,
                           &stagingBufferMemory,
                           bufferSize);

    fbrCreateBuffer(pVulkan,
                    bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    buffer,
                    bufferMemory);

    fbrCopyBuffer(pVulkan, stagingBuffer, *buffer, bufferSize);

    vkDestroyBuffer(pVulkan->device, stagingBuffer, NULL);
    vkFreeMemory(pVulkan->device, stagingBufferMemory, NULL);
}

void fbrCreateUniformBuffer(const FbrVulkan *pVulkan,
                            UniformBufferObject *pUniformBufferObject,
                            VkDeviceSize bufferSize) {
    fbrCreateBuffer(pVulkan, bufferSize,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &pUniformBufferObject->uniformBuffer,
                    &pUniformBufferObject->uniformBufferMemory);
    vkMapMemory(pVulkan->device,
                pUniformBufferObject->uniformBufferMemory,
                0,
                bufferSize,
                0,
                &pUniformBufferObject->pUniformBufferMapped);
}

void fbrImportUniformBuffer(const FbrVulkan *pVulkan,
                                    UniformBufferObject *pUniformBufferObject,
                                    VkDeviceSize bufferSize,
                                    HANDLE externalMemory) {
    fbrImportBuffer(pVulkan,
                    bufferSize,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    externalMemory,
                    &pUniformBufferObject->uniformBuffer,
                    &pUniformBufferObject->uniformBufferMemory);
    vkMapMemory(pVulkan->device,
                pUniformBufferObject->uniformBufferMemory,
                0,
                bufferSize,
                0,
                &pUniformBufferObject->pUniformBufferMapped);
}

void fbrCreateExternalUniformBuffer(const FbrVulkan *pVulkan,
                                    UniformBufferObject *pUniformBufferObject,
                                    VkDeviceSize bufferSize) {
    fbrCreateExternalBuffer(pVulkan, bufferSize,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &pUniformBufferObject->uniformBuffer,
                            &pUniformBufferObject->uniformBufferMemory,
                            &pUniformBufferObject->externalMemory);
    vkMapMemory(pVulkan->device,
                pUniformBufferObject->uniformBufferMemory,
                0,
                bufferSize,
                0,
                &pUniformBufferObject->pUniformBufferMapped);
}

void fbrCleanupUniformBuffers(const FbrVulkan *pVulkan, UniformBufferObject *pUniformBufferObject) {
    vkUnmapMemory(pVulkan->device, pUniformBufferObject->uniformBufferMemory);
    vkDestroyBuffer(pVulkan->device, pUniformBufferObject->uniformBuffer, NULL);
    vkFreeMemory(pVulkan->device, pUniformBufferObject->uniformBufferMemory, NULL);
}