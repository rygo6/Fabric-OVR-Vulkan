#include "fbr_framebuffer.h"
#include "fbr_vulkan.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

static VkFormat findSupportedDepthFormat(const FbrVulkan *pVulkan)
{
    //https://vulkan-tutorial.com/Depth_buffering
    VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (int i = 0; i < 3; ++i) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(pVulkan->physicalDevice, candidates[i], &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return candidates[i];
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return candidates[i];
        }
    }
}

static VkResult createFramebuffer(const FbrVulkan *pVulkan,
                                  FbrFramebuffer *pFrameBuffer,
                                  VkFormat colorFormat,
                                  VkFormat depthFormat,
                                  VkExtent2D extent,
                                  VkImageUsageFlags colorUsage,
                                  VkImageUsageFlags depthUsage) {
    const VkFramebufferAttachmentImageInfo pFramebufferAttachmentImageInfos[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
                    .width = extent.width,
                    .height = extent.height,
                    .layerCount = 1,
                    .usage = colorUsage,
                    .pViewFormats = &colorFormat,
                    .viewFormatCount = 1,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
                    .width = extent.width,
                    .height = extent.height,
                    .layerCount = 1,
                    .usage = depthUsage,
                    .pViewFormats = &depthFormat,
                    .viewFormatCount = 1,
            }
    };
    const VkFramebufferAttachmentsCreateInfo framebufferAttachmentsCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO,
            .attachmentImageInfoCount = 2,
            .pAttachmentImageInfos = pFramebufferAttachmentImageInfos,
    };
    const VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = &framebufferAttachmentsCreateInfo,
            .renderPass = pVulkan->renderPass,
            .flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
            .attachmentCount = 2,
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
    };
    FBR_ACK(vkCreateFramebuffer(pVulkan->device,
                                &framebufferCreateInfo,
                                NULL,
                                &pFrameBuffer->framebuffer));
}

//void fbrTransitionForRender(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer) {
//    VkImageMemoryBarrier barrier = {
//            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//            .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//            .srcQueueFamilyIndex = 0,
//            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
//            .image = pFramebuffer->pColorTexture->image,
//            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//            .subresourceRange.baseMipLevel = 0,
//            .subresourceRange.levelCount = 1,
//            .subresourceRange.baseArrayLayer = 0,
//            .subresourceRange.layerCount = 1,
//            .srcAccessMask = 0,
//            .dstAccessMask = 0,
//    };
//    vkCmdPipelineBarrier(
//            commandBuffer,
//            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
//            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//            0,
//            0, NULL,
//            0, NULL,
//            1, &barrier
//    );
//}
//
//void fbrTransitionForDisplay(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer) {
//    VkImageMemoryBarrier barrier = {
//            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//            .srcQueueFamilyIndex = 0,
//            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
//            .image = pFramebuffer->pColorTexture->image,
//            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//            .subresourceRange.baseMipLevel = 0,
//            .subresourceRange.levelCount = 1,
//            .subresourceRange.baseArrayLayer = 0,
//            .subresourceRange.layerCount = 1,
//            .srcAccessMask = 0,
//            .dstAccessMask = 0,
//    };
//    vkCmdPipelineBarrier(
//            commandBuffer,
//            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
//            0,
//            0, NULL,
//            0, NULL,
//            1, &barrier
//    );
//}

void fbrCreateFrameBufferFromImage(const FbrVulkan *pVulkan,
                                   VkFormat colorFormat,
                                   VkExtent2D extent,
                                   VkImage image,
                                   FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;
    fbrCreateTextureFromImage(pVulkan,
                              colorFormat,
                              extent,
                              image,
                              &pFramebuffer->pColorTexture);
    VkFormat depthFormat = findSupportedDepthFormat(pVulkan);
    if (depthFormat != VK_FORMAT_D32_SFLOAT)
        FBR_LOG_DEBUG("Depth colorFormat should be VK_FORMAT_D32_SFLOAT accord to ovr example", depthFormat, (depthFormat == VK_FORMAT_D32_SFLOAT));
    fbrCreateTexture(pVulkan,
                     depthFormat,
                     extent,
                     FBR_DEPTH_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_DEPTH_BIT,
                     false,
                     &pFramebuffer->pDepthTexture);
    VkImageAspectFlagBits depthAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    bool stencilComponent = hasStencilComponent(depthFormat);
    if (stencilComponent) {
        FBR_LOG_DEBUG("Depth has stencil component don't know what it is, from vulkan tutorial crossref with ovrexample", stencilComponent);
        depthAspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pDepthTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_2_NONE_KHR, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR,
                                      VK_PIPELINE_STAGE_2_NONE_KHR , VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR,
                                      depthAspectMask);
    createFramebuffer(pVulkan,
                      pFramebuffer,
                      colorFormat,
                      depthFormat,
                      extent,
                      FBR_COLOR_BUFFER_USAGE,
                      FBR_DEPTH_BUFFER_USAGE);
}

void fbrCreateFrameBuffer(const FbrVulkan *pVulkan,
                          bool external,
                          VkFormat colorFormat,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageUsageFlags colorUsage = external ? FBR_EXTERNAL_COLOR_BUFFER_USAGE : FBR_COLOR_BUFFER_USAGE;
    fbrCreateTexture(pVulkan,
                     colorFormat,
                     extent,
                     colorUsage,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     external,
                     &pFramebuffer->pColorTexture);
    // You don't need to do this on nvidia ??
    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pColorTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED,  external ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_2_NONE_KHR, VK_ACCESS_2_SHADER_READ_BIT,
                                      VK_PIPELINE_STAGE_2_NONE_KHR , VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
    VkFormat depthFormat = findSupportedDepthFormat(pVulkan);
    if (depthFormat != VK_FORMAT_D32_SFLOAT)
        FBR_LOG_DEBUG("Depth colorFormat should be VK_FORMAT_D32_SFLOAT accord to ovr example", depthFormat, (depthFormat == VK_FORMAT_D32_SFLOAT));
    VkImageAspectFlagBits depthAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    VkImageUsageFlags a = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageUsageFlags b = VK_IMAGE_USAGE_SAMPLED_BIT;
    VkImageUsageFlags c = (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    VkImageUsageFlags depthUsage = external ? FBR_EXTERNAL_DEPTH_BUFFER_USAGE : FBR_DEPTH_BUFFER_USAGE;
    fbrCreateTexture(pVulkan,
                     depthFormat,
                     extent,
                     depthUsage,
                     depthAspectMask,
                     external,
                     &pFramebuffer->pDepthTexture);
    bool stencilComponent = hasStencilComponent(depthFormat);
    if (stencilComponent) {
        FBR_LOG_DEBUG("Depth has stencil component don't know what it is, from vulkan tutorial crossref with ovrexample", stencilComponent);
        depthAspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    fbrTransitionImageLayoutImmediate(pVulkan,
                                      pFramebuffer->pDepthTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, external ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_2_NONE_KHR, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR,
                                      VK_PIPELINE_STAGE_2_NONE_KHR , VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR,
                                      depthAspectMask);
    createFramebuffer(pVulkan,
                      pFramebuffer,
                      colorFormat,
                      depthFormat,
                      extent,
                      colorUsage,
                      depthUsage);
}

void fbrImportFrameBuffer(const FbrVulkan *pVulkan,
                          HANDLE colorExternalMemory,
                          HANDLE depthExternalMemory,
                          VkFormat colorFormat,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;
    fbrImportTexture(pVulkan,
                     colorFormat,
                     extent,
                     FBR_EXTERNAL_COLOR_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     colorExternalMemory,
                     &pFramebuffer->pColorTexture);
    fbrTransitionImageLayoutImmediate(pVulkan,
                                      pFramebuffer->pColorTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_2_NONE_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_2_NONE_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      VK_IMAGE_ASPECT_COLOR_BIT);

    VkFormat depthFormat = findSupportedDepthFormat(pVulkan);
    if (depthFormat != VK_FORMAT_D32_SFLOAT)
        FBR_LOG_DEBUG("Depth colorFormat should be VK_FORMAT_D32_SFLOAT accord to ovr example", depthFormat, (depthFormat == VK_FORMAT_D32_SFLOAT));
    VkImageAspectFlagBits depthAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    bool stencilComponent = hasStencilComponent(depthFormat);
    if (stencilComponent) {
        FBR_LOG_DEBUG("Depth has stencil component don't know what it is, from vulkan tutorial crossref with ovrexample", stencilComponent);
        depthAspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    fbrImportTexture(pVulkan,
                     depthFormat,
                     extent,
                     FBR_EXTERNAL_DEPTH_BUFFER_USAGE,
                     depthAspectMask,
                     depthExternalMemory,
                     &pFramebuffer->pDepthTexture);
    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pDepthTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_2_NONE_KHR, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR,
                                      VK_PIPELINE_STAGE_2_NONE_KHR , VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR,
                                      depthAspectMask);

    createFramebuffer(pVulkan,
                      pFramebuffer,
                      colorFormat,
                      depthFormat,
                      extent,
                      FBR_EXTERNAL_COLOR_BUFFER_USAGE,
                      FBR_EXTERNAL_DEPTH_BUFFER_USAGE);
}

void fbrDestroyFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer) {
    fbrDestroyTexture(pVulkan, pFramebuffer->pColorTexture);
    fbrDestroyTexture(pVulkan, pFramebuffer->pDepthTexture);
    vkDestroyFramebuffer(pVulkan->device, pFramebuffer->framebuffer, NULL);
    free(pFramebuffer);
}

void fbrIPCTargetImportFrameBuffer(FbrApp *pApp, FbrIPCParamImportFrameBuffer *pParam) {
    FBR_LOG_DEBUG("Importing Framebuffer.", pParam->handle, pParam->width, pParam->height);
//    fbrImportFrameBuffer(pApp->pVulkan,
//                         &pApp->pParentProcessFramebuffer,
//                         pParam->handle,
//                         (VkExtent2D) {pParam->width, pParam->height});
}