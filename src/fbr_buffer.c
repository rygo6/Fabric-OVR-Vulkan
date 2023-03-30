#include "fbr_buffer.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"

#if WIN32
#include <vulkan/vulkan_win32.h>
#define FBR_EXTERNAL_MEMORY_HANDLE_TYPE VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
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

void importBuffer(const FbrVulkan *pVulkan,
                  VkMemoryPropertyFlags properties,
                  VkBufferUsageFlags usage,
                  VkDeviceSize size,
                  HANDLE externalMemory,
                  VkBuffer *pBuffer,
                  VkDeviceMemory *pBufferMemory) {

    VkExternalMemoryBufferCreateInfoKHR externalMemoryBufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR,
            .pNext = NULL,
            .handleTypes = FBR_EXTERNAL_MEMORY_HANDLE_TYPE
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
//            .pNext = NULL,
//            .image = pTestTexture->image,
//            .buffer = VK_NULL_HANDLE
//    };
    VkImportMemoryWin32HandleInfoKHR importMemoryInfo = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
            .pNext = NULL,
            .handleType = FBR_EXTERNAL_MEMORY_HANDLE_TYPE,
            .handle = externalMemory,
            .name = NULL
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

VkResult createAllocBindBuffer(const FbrVulkan *pVulkan,
                               VkMemoryPropertyFlags properties,
                               VkBufferUsageFlags usage,
                               VkDeviceSize size,
                               bool external,
                               VkBuffer *pBuffer,
                               VkDeviceMemory *pBufferMemory)
                               {
    VkExternalMemoryBufferCreateInfoKHR externalMemoryBufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR,
            .pNext = NULL,
            .handleTypes = FBR_EXTERNAL_MEMORY_HANDLE_TYPE
    };
    VkBufferCreateInfo bufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = external ? &externalMemoryBufferCreateInfo : NULL,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    FBR_ACK(vkCreateBuffer(pVulkan->device, &bufferCreateInfo, NULL, pBuffer));

    VkMemoryRequirements memRequirements = {};
    uint32_t memTypeIndex;
    FBR_ACK(fbrBufferMemoryTypeFromProperties(pVulkan,
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
    VkExportMemoryWin32HandleInfoKHR exportMemoryWin32HandleInfo = {
            .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
            .pNext = NULL,
            .name = NULL,
            .pAttributes = NULL,
            // This seems to not make the actual UBO read only, only the NT handle I presume
            .dwAccess = GENERIC_READ
    };
    VkExportMemoryAllocateInfo exportAllocInfo = {
            .sType =VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
            .pNext = &exportMemoryWin32HandleInfo,
            .handleTypes = FBR_EXTERNAL_MEMORY_HANDLE_TYPE
    };
    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = external ? &exportAllocInfo : NULL,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memTypeIndex
    };
    FBR_ACK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, pBufferMemory));

    FBR_ACK(vkBindBufferMemory(pVulkan->device, *pBuffer, *pBufferMemory, 0));

    return FBR_SUCCESS;
}

FBR_RESULT getExternalHandle(const FbrVulkan *pVulkan,
                           VkDeviceMemory *pBufferMemory,
                           HANDLE *pExternalHandle) {
    VkMemoryGetWin32HandleInfoKHR memoryInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
            .pNext = NULL,
            .memory = *pBufferMemory,
            .handleType = FBR_EXTERNAL_MEMORY_HANDLE_TYPE
    };
    PFN_vkGetMemoryWin32HandleKHR getMemoryWin32HandleFunc = (PFN_vkGetMemoryWin32HandleKHR) vkGetInstanceProcAddr(pVulkan->instance, "vkGetMemoryWin32HandleKHR");
    if (getMemoryWin32HandleFunc == NULL) {
        FBR_LOG_DEBUG("Failed to get PFN_vkGetMemoryWin32HandleKHR!");
    }
    FBR_ACK(getMemoryWin32HandleFunc(pVulkan->device, &memoryInfo, pExternalHandle));

    return FBR_SUCCESS;
}

// TODO this immediate command buffer creating destroy a whole command buffer and waiting each frame is horribly inefficient
FBR_RESULT fbrBeginImmediateCommandBuffer(const FbrVulkan *pVulkan, VkCommandBuffer *pCommandBuffer) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool =  pVulkan->commandPool,
            .commandBufferCount = 1,
    };

    FBR_ACK(vkAllocateCommandBuffers(pVulkan->device, &allocInfo, pCommandBuffer));

    VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    FBR_ACK(vkBeginCommandBuffer(*pCommandBuffer, &beginInfo));

    return FBR_SUCCESS;
}

FBR_RESULT fbrEndImmediateCommandBuffer(const FbrVulkan *pVulkan, VkCommandBuffer *pCommandBuffer) {
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

    FBR_ACK(vkQueueSubmit2(pVulkan->queue, 1, &submitInfo, VK_NULL_HANDLE));
    FBR_ACK(vkQueueWaitIdle(pVulkan->queue)); // TODO could be more optimized with vkWaitForFences https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

    vkFreeCommandBuffers(pVulkan->device, pVulkan->commandPool, 1, pCommandBuffer);

    return FBR_SUCCESS;
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
                                       VkPipelineStageFlags dstStageMask,
                                       VkImageAspectFlags aspectMask) {

    VkCommandBuffer commandBuffer;
    fbrBeginImmediateCommandBuffer(pVulkan, &commandBuffer);

    // why is src accesss/stage not being applied!?
    VkImageMemoryBarrier2 barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = srcStageMask,
            .dstStageMask = dstStageMask,
            .srcAccessMask = srcAccessMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange.aspectMask = aspectMask,
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

void fbrMemCopyMappedUBO(const FbrUniformBufferObject *pDstUBO, const void* pSrcData, size_t size) {
    memcpy(pDstUBO->pUniformBufferMapped, pSrcData, size);
}

//TODO reuse staging buffers
void fbrCreateStagingBuffer(const FbrVulkan *pVulkan,
                            const void *srcData,
                            VkBuffer *stagingBuffer,
                            VkDeviceMemory *stagingBufferMemory,
                            VkDeviceSize bufferSize) {
    createAllocBindBuffer(pVulkan,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          bufferSize,
                          false,
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

    createAllocBindBuffer(pVulkan,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                          bufferSize,
                          false,
                          buffer,
                          bufferMemory);

    fbrCopyBuffer(pVulkan, stagingBuffer, *buffer, bufferSize);

    vkDestroyBuffer(pVulkan->device, stagingBuffer, NULL);
    vkFreeMemory(pVulkan->device, stagingBufferMemory, NULL);
}

FBR_RESULT fbrCreateUBO(const FbrVulkan *pVulkan,
                      VkMemoryPropertyFlags properties,
                      VkBufferUsageFlags usage,
                      VkDeviceSize bufferSize,
                      bool external,
                      FbrUniformBufferObject **ppAllocUBO) {
    *ppAllocUBO = calloc(1, sizeof(FbrUniformBufferObject));
    FbrUniformBufferObject *pUBO = *ppAllocUBO;
    FBR_ACK(createAllocBindBuffer(pVulkan,
                                  properties,
                                  usage,
                                  bufferSize,
                                  external,
                                  &pUBO->uniformBuffer,
                                  &pUBO->uniformBufferMemory));
    FBR_ACK(vkMapMemory(pVulkan->device,
                        pUBO->uniformBufferMemory,
                        0,
                        bufferSize,
                        0,
                        &pUBO->pUniformBufferMapped));
    if (external) {
        FBR_ACK(getExternalHandle(pVulkan,
                                  &pUBO->uniformBufferMemory,
                                  &pUBO->externalMemory));
    }

    return FBR_SUCCESS;
}

void fbrImportUBO(const FbrVulkan *pVulkan,
                  VkMemoryPropertyFlags properties,
                  VkBufferUsageFlags usage,
                  VkDeviceSize bufferSize,
                  HANDLE externalMemory,
                  FbrUniformBufferObject **ppAllocUBO) {
    *ppAllocUBO = calloc(1, sizeof(FbrUniformBufferObject));
    FbrUniformBufferObject *pUBO = *ppAllocUBO;
    importBuffer(pVulkan,
                 properties,
                 usage,
                 bufferSize,
                 externalMemory,
                 &pUBO->uniformBuffer,
                 &pUBO->uniformBufferMemory);
    vkMapMemory(pVulkan->device,
                pUBO->uniformBufferMemory,
                0,
                bufferSize,
                0,
                &pUBO->pUniformBufferMapped);
    pUBO->externalMemory = externalMemory;
    // don't need to set or map anything because parent does it!
}

void fbrDestroyUBO(const FbrVulkan *pVulkan, FbrUniformBufferObject *pUBO) {
    vkDestroyBuffer(pVulkan->device, pUBO->uniformBuffer, NULL);

    if (pUBO->pUniformBufferMapped != NULL)
        vkUnmapMemory(pVulkan->device, pUBO->uniformBufferMemory);

    vkFreeMemory(pVulkan->device, pUBO->uniformBufferMemory, NULL);

    if (pUBO->externalMemory != NULL)
        CloseHandle(pUBO->externalMemory);

    free(pUBO);
}