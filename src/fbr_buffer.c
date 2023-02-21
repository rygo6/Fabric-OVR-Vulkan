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
//            .image = pTestTexture->image,
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

// TODO this immediate command buffer creating destroy a whole command buffer and waiting each frame is horribly inefficient
VkResult fbrBeginImmediateCommandBuffer(const FbrVulkan *pVulkan, VkCommandBuffer *pCommandBuffer) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool =  pVulkan->commandPool,
            .commandBufferCount = 1,
    };

    FBR_VK_CHECK_RETURN(vkAllocateCommandBuffers(pVulkan->device, &allocInfo, pCommandBuffer));

    VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    FBR_VK_CHECK_RETURN(vkBeginCommandBuffer(*pCommandBuffer, &beginInfo));

    return VK_SUCCESS;
}

VkResult fbrEndImmediateCommandBuffer(const FbrVulkan *pVulkan, VkCommandBuffer *pCommandBuffer) {
    vkEndCommandBuffer(*pCommandBuffer);

    VkCommandBufferSubmitInfo commandBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
            .commandBuffer = *pCommandBuffer,
    };
    VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
    };

    FBR_VK_CHECK_RETURN(vkQueueSubmit2(pVulkan->queue, 1, &submitInfo, VK_NULL_HANDLE));
    FBR_VK_CHECK_RETURN(vkQueueWaitIdle(pVulkan->queue)); // TODO could be more optimized with vkWaitForFences https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

    vkFreeCommandBuffers(pVulkan->device, pVulkan->commandPool, 1, pCommandBuffer);

    return VK_SUCCESS;
}

// TODO implemented for unified graphics + transfer only right now
//  https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#transfer-dependencies
void fbrTransitionImageLayoutImmediate(const FbrVulkan *pVulkan,
                                       VkImage image,
                                       VkImageLayout oldLayout,
                                       VkImageLayout newLayout,
                                       VkAccessFlags srcAccessMask,
                                       VkAccessFlags dstAccessMask,
                                       VkPipelineStageFlags srcStageMask,
                                       VkPipelineStageFlags dstStageMask) {

    VkCommandBuffer commandBuffer;
    fbrBeginImmediateCommandBuffer(pVulkan, &commandBuffer);

    VkImageMemoryBarrier2 barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
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
    };
    VkDependencyInfo dependencyInfo = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier,
    };
    vkCmdPipelineBarrier2(commandBuffer,&dependencyInfo);

    fbrEndImmediateCommandBuffer(pVulkan, &commandBuffer);
}

void fbrTransitionBufferLayoutImmediate(const FbrVulkan *pVulkan,
                                       VkBuffer buffer,
                                       VkAccessFlags srcAccessMask,
                                       VkAccessFlags dstAccessMask,
                                       VkPipelineStageFlags srcStageMask,
                                       VkPipelineStageFlags dstStageMask) {

    VkCommandBuffer commandBuffer;
    fbrBeginImmediateCommandBuffer(pVulkan, &commandBuffer);

    VkBufferMemoryBarrier2 barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = srcAccessMask,
            .dstAccessMask = dstAccessMask,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = buffer,
            .offset = 0,
            .size = 0
    };
    VkDependencyInfo dependencyInfo = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &barrier,
    };
    vkCmdPipelineBarrier2(commandBuffer,&dependencyInfo);

    fbrEndImmediateCommandBuffer(pVulkan, &commandBuffer);
}

void fbrCopyBuffer(const FbrVulkan *pVulkan, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer;
    fbrBeginImmediateCommandBuffer(pVulkan, &commandBuffer);

    VkBufferCopy copyRegion = {
            .srcOffset = 0, // Optional
            .dstOffset = 0, // Optional
            .size = size,
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    fbrEndImmediateCommandBuffer(pVulkan, &commandBuffer);
}

//TODO reuse staging buffers
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

    // Flush the memory range
    // If the memory type of stagingMemory includes VK_MEMORY_PROPERTY_HOST_COHERENT, skip this step
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
    // don't need to set or map anything because parent does it!
//    vkMapMemory(pVulkan->device,
//                pUniformBufferObject->uniformBufferMemory,
//                0,
//                bufferSize,
//                0,
//                &pUniformBufferObject->pUniformBufferMapped);
//    fbrTransitionBufferLayoutImmediate(pVulkan, pUniformBufferObject->uniformBuffer,
//                                       VK_ACCESS_NONE_KHR, VK_ACCESS_2_HOST_READ_BIT_KHR,
//                                       VK_PIPELINE_STAGE_NONE , VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT );
}

void fbrCreateExternalUniformBuffer(const FbrVulkan *pVulkan,
                                    UniformBufferObject *pUniformBufferObject,
                                    VkDeviceSize bufferSize) {
    // todo this should alloc/create the UBO

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
//    fbrTransitionBufferLayoutImmediate(pVulkan, pUniformBufferObject->uniformBuffer,
//                                       VK_ACCESS_NONE_KHR, VK_ACCESS_HOST_WRITE_BIT,
//                                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT );
}

void fbrCleanupUniformBuffer(const FbrVulkan *pVulkan, UniformBufferObject *pUniformBufferObject) {
    vkDestroyBuffer(pVulkan->device, pUniformBufferObject->uniformBuffer, NULL);

    if (pUniformBufferObject->pUniformBufferMapped != NULL)
        vkUnmapMemory(pVulkan->device, pUniformBufferObject->uniformBufferMemory);

    vkFreeMemory(pVulkan->device, pUniformBufferObject->uniformBufferMemory, NULL);

    if (pUniformBufferObject->externalMemory != NULL)
        CloseHandle(pUniformBufferObject->externalMemory);

    free(pUniformBufferObject);
}