#include "fbr_texture.h"
#include "fbr_buffer.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if WIN32
#include <vulkan/vulkan_win32.h>
#endif

#if X11
#include <vulkan/vulkan_xlib.h>
#endif

static VkResult copyBufferToImage(const FbrVulkan *pVulkan,
                       VkBuffer buffer,
                       VkImage image,
                       uint32_t width,
                       uint32_t height) {
    VkCommandBuffer commandBuffer;
    FBR_VK_CHECK_RETURN(fbrBeginImmediateCommandBuffer(pVulkan, &commandBuffer));

    VkBufferImageCopy region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .imageSubresource.mipLevel = 0,
            .imageSubresource.baseArrayLayer = 0,
            .imageSubresource.layerCount = 1,
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1},
    };

    vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
    );

    FBR_VK_CHECK_RETURN(fbrEndImmediateCommandBuffer(pVulkan, &commandBuffer));
}

static void createTextureFromExternal(const FbrVulkan *pVulkan,
                          int width,
                          int height,
                          VkFormat format,
                          VkImageTiling tiling,
                          VkImageUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          HANDLE externalMemory,
                          FbrTexture *pTexture) {

    FBR_LOG_DEBUG("Loading pTexture from external handle.", pTexture->externalMemory, pTexture->width, pTexture->height);

    VkExternalMemoryHandleTypeFlags externalHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
    VkExternalMemoryImageCreateInfoKHR externalImageInfo = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR,
            .pNext = VK_NULL_HANDLE,
            .handleTypes = externalHandleType
    };

    VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = format,
            .tiling = tiling,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = usage,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .pNext = &externalImageInfo,
    };

    FBR_VK_CHECK(vkCreateImage(pVulkan->device, &imageCreateInfo, NULL, &pTexture->image));

    VkMemoryRequirements memRequirements = {};
    uint32_t memTypeIndex;
    FBR_VK_CHECK(fbrImageMemoryTypeFromProperties(pVulkan,
                                                  pTexture->image,
                                                  properties,
                                                  &memRequirements,
                                                  &memTypeIndex));

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
    FBR_VK_CHECK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, &pTexture->deviceMemory));

    FBR_VK_CHECK(vkBindImageMemory(pVulkan->device, pTexture->image, pTexture->deviceMemory, 0));

    pTexture->externalMemory = externalMemory;
    pTexture->width = width;
    pTexture->height = height;
}

static void checkPreferredDedicated(const FbrVulkan *pVulkan, const FbrTexture *pTexture){
    VkImageMemoryRequirementsInfo2KHR memoryRequirementsInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR,
            .image = pTexture->image,
    };
    VkMemoryDedicatedRequirementsKHR memoryDedicatedRequirements = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR,
    };
    VkMemoryRequirements2KHR memRequirements2 = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR,
            .pNext = &memoryDedicatedRequirements,
    };
    PFN_vkGetImageMemoryRequirements2KHR getImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2KHR) vkGetInstanceProcAddr(pVulkan->instance, "vkGetImageMemoryRequirements2KHR");
    if (getImageMemoryRequirements2 == NULL) {
        FBR_LOG_DEBUG("Failed to get PFN_vkGetImageMemoryRequirements2KHR!");
    }
    getImageMemoryRequirements2(pVulkan->device, &memoryRequirementsInfo, &memRequirements2);
    FBR_LOG_DEBUG("prefersDedicatedAllocation", memoryDedicatedRequirements.prefersDedicatedAllocation);
    FBR_LOG_DEBUG("requiresDedicatedAllocation", memoryDedicatedRequirements.requiresDedicatedAllocation);
}

static void createExternalTexture(const FbrVulkan *pVulkan,
                                  uint32_t width,
                                  uint32_t height,
                                  VkFormat format,
                                  VkImageTiling tiling,
                                  VkImageUsageFlags usage,
                                  VkMemoryPropertyFlags properties,
                                  FbrTexture *pTexture) {

    VkExternalMemoryHandleTypeFlagBitsKHR externalHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
    VkExternalMemoryImageCreateInfoKHR externalImageInfo = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .handleTypes = externalHandleType,
    };
    VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = &externalImageInfo,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = format,
            .tiling = tiling,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = usage,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    FBR_VK_CHECK(vkCreateImage(pVulkan->device, &imageCreateInfo, NULL, &pTexture->image));

    VkMemoryRequirements memRequirements = {};
    uint32_t memTypeIndex;
    FBR_VK_CHECK(fbrImageMemoryTypeFromProperties(pVulkan,
                                             pTexture->image,
                                             properties,
                                             &memRequirements,
                                             &memTypeIndex));

    // not sure if dedicated is needed???
//    VkMemoryDedicatedAllocateInfoKHR dedicatedAllocInfo = {
//            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
//            .image = pTexture->image,
//            .buffer = VK_NULL_HANDLE,
//    };
    VkExportMemoryAllocateInfo exportAllocInfo = {
            .sType =VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
//            .pNext =&dedicatedAllocInfo,
            .handleTypes = externalHandleType
    };
    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memTypeIndex,
            .pNext = &exportAllocInfo
    };
    FBR_VK_CHECK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, &pTexture->deviceMemory));

    FBR_LOG_DEBUG("Allocated Framebuffer of size: ", memRequirements.size);

    FBR_VK_CHECK(vkBindImageMemory(pVulkan->device, pTexture->image, pTexture->deviceMemory, 0));

#if WIN32
    VkMemoryGetWin32HandleInfoKHR memoryInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
            .pNext = NULL,
            .memory = pTexture->deviceMemory,
            .handleType = externalHandleType
    };

    PFN_vkGetMemoryWin32HandleKHR getMemoryWin32HandleFunc = (PFN_vkGetMemoryWin32HandleKHR) vkGetInstanceProcAddr(pVulkan->instance, "vkGetMemoryWin32HandleKHR");
    if (getMemoryWin32HandleFunc == NULL) {
        FBR_LOG_DEBUG("Failed to get PFN_vkGetMemoryWin32HandleKHR!");
    }
    if (getMemoryWin32HandleFunc(pVulkan->device, &memoryInfo, &pTexture->externalMemory) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to get external handle!");
    }
#endif
}

static void createTexture(const FbrVulkan *pVulkan,
                          uint32_t width,
                          uint32_t height,
                          VkFormat format,
                          VkImageTiling tiling,
                          VkImageUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          FbrTexture *pTexture) {

    VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = format,
            .tiling = tiling,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = usage,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    FBR_VK_CHECK(vkCreateImage(pVulkan->device, &imageCreateInfo, NULL, &pTexture->image));

    VkMemoryRequirements memRequirements = {};
    uint32_t memTypeIndex;
    FBR_VK_CHECK(fbrImageMemoryTypeFromProperties(pVulkan,
                                                  pTexture->image,
                                                  properties,
                                                  &memRequirements,
                                                  &memTypeIndex));

    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memTypeIndex,
    };
    FBR_VK_CHECK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, &pTexture->deviceMemory));

    vkBindImageMemory(pVulkan->device, pTexture->image, pTexture->deviceMemory, 0);
}

static void createTextureView(const FbrVulkan *pVulkan, FbrTexture *pTexture, VkFormat format) {
    VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = pTexture->image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
    };

    if (vkCreateImageView(pVulkan->device, &viewInfo, NULL, &pTexture->imageView) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to create pTexture image view!");
    }
}

static void createTextureFromFile(const FbrVulkan *pVulkan, FbrTexture *pTexture, char const *filename, const bool external) {
    int texChannels;
    stbi_uc *pixels = stbi_load(filename, &pTexture->width, &pTexture->height, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageBufferSize = pTexture->width * pTexture->height * 4;

    FBR_LOG_DEBUG("Loading pTexture from file.", filename, pTexture->width, pTexture->height, texChannels);

    if (!pixels) {
        FBR_LOG_DEBUG("Failed to load pTexture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    fbrCreateStagingBuffer(pVulkan,
                           pixels,
                           &stagingBuffer,
                           &stagingBufferMemory,
                           imageBufferSize);

    stbi_image_free(pixels);

    if (external) {
        createExternalTexture(pVulkan,
                              pTexture->width,
                              pTexture->height,
                              VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              pTexture);
    } else {
        createTexture(pVulkan,
                      pTexture->width,
                      pTexture->height,
                      VK_FORMAT_R8G8B8A8_SRGB,
                      VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      pTexture);
    }

    fbrTransitionImageLayoutImmediate(pVulkan,
                                      pTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      VK_ACCESS_2_NONE, VK_ACCESS_2_MEMORY_WRITE_BIT,
                                      VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    copyBufferToImage(pVulkan, stagingBuffer, pTexture->image, pTexture->width, pTexture->height);
    fbrTransitionImageLayoutImmediate(pVulkan,
                                      pTexture->image,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      VK_ACCESS_2_MEMORY_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT_KHR,
                                      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR);

    vkDestroyBuffer(pVulkan->device, stagingBuffer, NULL);
    vkFreeMemory(pVulkan->device, stagingBufferMemory, NULL);
}

void fbrCreateTextureFromFile(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, char const *filename, bool external) {
    *ppAllocTexture = calloc(1, sizeof(FbrTexture));
    FbrTexture *pTexture = *ppAllocTexture;
    createTextureFromFile(pVulkan, pTexture, filename, external);
    createTextureView(pVulkan, pTexture, VK_FORMAT_R8G8B8A8_SRGB);
}

void fbrImportTexture(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, HANDLE externalMemory, int width, int height) {
    *ppAllocTexture = calloc(1, sizeof(FbrTexture));
    FbrTexture *pTexture = *ppAllocTexture;
    pTexture->width = width;
    pTexture->height = height;
    createTextureFromExternal(pVulkan,
                              width,
                              height,
                              VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_IMAGE_USAGE_SAMPLED_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              externalMemory,
                              pTexture);
    fbrTransitionImageLayoutImmediate(pVulkan, pTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      VK_ACCESS_NONE_KHR, VK_ACCESS_SHADER_READ_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    createTextureView(pVulkan, pTexture, VK_FORMAT_R8G8B8A8_SRGB);
}

void fbrImportFramebufferTexture(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, HANDLE externalMemory, int width, int height) {
    *ppAllocTexture = calloc(1, sizeof(FbrTexture));
    FbrTexture *pTexture = *ppAllocTexture;
    pTexture->width = width;
    pTexture->height = height;
    createTextureFromExternal(pVulkan,
                              width,
                              height,
                              VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_TILING_OPTIMAL,
                              (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              externalMemory,
                              pTexture);
    fbrTransitionImageLayoutImmediate(pVulkan, pTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE_KHR, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    createTextureView(pVulkan, pTexture, VK_FORMAT_R8G8B8A8_SRGB);
}

// todo condense to one method
void fbrCreateExternalFramebufferTexture(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, int width, int height) {
    *ppAllocTexture = calloc(1, sizeof(FbrTexture));
    FbrTexture *pTexture = *ppAllocTexture;
    pTexture->width = width;
    pTexture->height = height;
    createExternalTexture(pVulkan,
                          width,
                          height,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_TILING_OPTIMAL,
                          (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          pTexture);
    fbrTransitionImageLayoutImmediate(pVulkan, pTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      VK_ACCESS_NONE_KHR, VK_ACCESS_SHADER_READ_BIT,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    createTextureView(pVulkan, pTexture, VK_FORMAT_R8G8B8A8_SRGB);
}

void fbrDestroyTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture) {
    vkDestroyImage(pVulkan->device, pTexture->image, NULL);
    vkFreeMemory(pVulkan->device, pTexture->deviceMemory, NULL);
    vkDestroyImageView(pVulkan->device, pTexture->imageView, NULL);

    if (pTexture->externalMemory != NULL)
        CloseHandle(pTexture->externalMemory);

    free(pTexture);
}