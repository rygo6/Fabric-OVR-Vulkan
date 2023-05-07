#include "fbr_framebuffer.h"
#include "fbr_vulkan.h"
#include "fbr_buffer.h"
#include "fbr_log.h"



static VkFormat findSupportedDepthFormat(const FbrVulkan *pVulkan)
{
    //https://vulkan-tutorial.com/Depth_buffering
    VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    for (int i = 0; i < 3; ++i) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(pVulkan->physicalDevice, candidates[i], &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            depthFormat = candidates[i];
            break;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            depthFormat = candidates[i];
            break;
        }
    }

    if (depthFormat == VK_FORMAT_UNDEFINED)
        FBR_LOG_DEBUG("DepthFormat couldn't be foudn?!", depthFormat, (depthFormat == VK_FORMAT_D32_SFLOAT));

    if (depthFormat != VK_FORMAT_D32_SFLOAT)
        FBR_LOG_DEBUG("Depth colorFormat should be VK_FORMAT_D32_SFLOAT accord to ovr example", depthFormat, (depthFormat == VK_FORMAT_D32_SFLOAT));

    return depthFormat;
}

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

static VkImageAspectFlagBits getDepthAspectMask()
{
    VkImageAspectFlagBits depthAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    // todo is this needed? a
//    bool stencilComponent = hasStencilComponent(depthFormat);
//    if (stencilComponent) {
//        FBR_LOG_DEBUG("Depth has stencil component don't know what it is, from vulkan tutorial crossref with ovrexample", stencilComponent);
//        depthAspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
//    }
    return depthAspectMask;
}

static VkResult createFramebuffer(const FbrVulkan *pVulkan,
                                  FbrFramebuffer *pFrameBuffer,
                                  VkFormat colorFormat,
                                  VkFormat normalFormat,
                                  VkFormat depthFormat,
                                  VkExtent2D extent,
                                  VkImageUsageFlags colorUsage,
                                  VkImageUsageFlags normalUsage,
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
                    .usage = normalUsage,
                    .pViewFormats = &normalFormat,
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
            .attachmentImageInfoCount = 3,
            .pAttachmentImageInfos = pFramebufferAttachmentImageInfos,
    };
    const VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = &framebufferAttachmentsCreateInfo,
            .renderPass = pVulkan->renderPass,
            .flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
            .attachmentCount = 3,
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

//void fbrCreateFrameBufferFromImage(const FbrVulkan *pVulkan,
//                                   VkFormat colorFormat,
//                                   VkExtent2D extent,
//                                   VkImage image,
//                                   FbrFramebuffer **ppAllocFramebuffer) {
//    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
//    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
//    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;
//    // color
//    fbrCreateTextureFromImage(pVulkan,
//                              FBR_COLOR_BUFFER_FORMAT,
//                              extent,
//                              image,
//                              &pFramebuffer->pColorTexture);
//    // normal
//    fbrCreateTexture(pVulkan,
//                     FBR_NORMAL_BUFFER_FORMAT,
//                     extent,
//                     FBR_NORMAL_BUFFER_USAGE,
//                     VK_IMAGE_ASPECT_COLOR_BIT,
//                     false,
//                     &pFramebuffer->pNormalTexture);
//    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pNormalTexture->image,
//                                      VK_IMAGE_LAYOUT_UNDEFINED,  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//                                      VK_ACCESS_NONE,  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
//                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//                                      VK_IMAGE_ASPECT_COLOR_BIT);
//    // depth
//    VkFormat depthFormat = findSupportedDepthFormat(pVulkan);
//    fbrCreateTexture(pVulkan,
//                     depthFormat,
//                     extent,
//                     FBR_DEPTH_BUFFER_USAGE,
//                     VK_IMAGE_ASPECT_DEPTH_BIT,
//                     false,
//                     &pFramebuffer->pDepthTexture);
//    VkImageAspectFlagBits depthAspectMask = getDepthAspectMask();
//    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pDepthTexture->image,
//                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
//                                      VK_ACCESS_NONE, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
//                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT , VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
//                                      depthAspectMask);
//    createFramebuffer(pVulkan,
//                      pFramebuffer,
//                      FBR_COLOR_BUFFER_FORMAT,
//                      FBR_NORMAL_BUFFER_FORMAT,
//                      depthFormat,
//                      extent,
//                      FBR_COLOR_BUFFER_USAGE,
//                      FBR_NORMAL_BUFFER_USAGE,
//                      FBR_DEPTH_BUFFER_USAGE);
//}

void fbrCreateFrameBuffer(const FbrVulkan *pVulkan,
                          bool external,
                          VkFormat colorFormat,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;
    // Color
    VkImageUsageFlags colorUsage = external ? FBR_EXTERNAL_COLOR_BUFFER_USAGE : FBR_COLOR_BUFFER_USAGE;
    fbrCreateTexture(pVulkan,
                     FBR_COLOR_BUFFER_FORMAT,
                     extent,
                     colorUsage,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     external,
                     &pFramebuffer->pColorTexture);
    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pColorTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED,  external ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE, external ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT , external ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
    // Normal
    VkImageUsageFlags normalUsage = external ? FBR_EXTERNAL_NORMAL_BUFFER_USAGE : FBR_NORMAL_BUFFER_USAGE;
    fbrCreateTexture(pVulkan,
                     FBR_NORMAL_BUFFER_FORMAT,
                     extent,
                     normalUsage,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     external,
                     &pFramebuffer->pNormalTexture);
    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pNormalTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED,  external ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE, external ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT , external ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
    //Depth
    VkFormat depthFormat = findSupportedDepthFormat(pVulkan);
    VkImageAspectFlagBits depthAspectMask = getDepthAspectMask();
    VkImageUsageFlags depthUsage = external ? FBR_EXTERNAL_DEPTH_BUFFER_USAGE : FBR_DEPTH_BUFFER_USAGE;
    fbrCreateTexture(pVulkan,
                     depthFormat,
                     extent,
                     depthUsage,
                     depthAspectMask,
                     external,
                     &pFramebuffer->pDepthTexture);
    fbrTransitionImageLayoutImmediate(pVulkan,
                                      pFramebuffer->pDepthTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, external ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE, external ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT , external ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                      depthAspectMask);
    createFramebuffer(pVulkan,
                      pFramebuffer,
                      FBR_COLOR_BUFFER_FORMAT,
                      FBR_NORMAL_BUFFER_FORMAT,
                      depthFormat,
                      extent,
                      colorUsage,
                      normalUsage,
                      depthUsage);
}

void fbrImportFrameBuffer(const FbrVulkan *pVulkan,
                          HANDLE colorExternalMemory,
                          HANDLE normalExternalMemory,
                          HANDLE depthExternalMemory,
                          VkFormat colorFormat,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;
    // color
    fbrImportTexture(pVulkan,
                     FBR_COLOR_BUFFER_FORMAT,
                     extent,
                     FBR_EXTERNAL_COLOR_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     colorExternalMemory,
                     &pFramebuffer->pColorTexture);
    fbrTransitionImageLayoutImmediate(pVulkan,
                                      pFramebuffer->pColorTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
    // normal
    fbrImportTexture(pVulkan,
                     FBR_NORMAL_BUFFER_FORMAT,
                     extent,
                     FBR_EXTERNAL_NORMAL_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     normalExternalMemory,
                     &pFramebuffer->pNormalTexture);
    fbrTransitionImageLayoutImmediate(pVulkan,
                                      pFramebuffer->pNormalTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
    // depth
    VkFormat depthFormat = findSupportedDepthFormat(pVulkan);
    VkImageAspectFlagBits depthAspectMask = getDepthAspectMask();
    fbrImportTexture(pVulkan,
                     depthFormat,
                     extent,
                     FBR_EXTERNAL_DEPTH_BUFFER_USAGE,
                     depthAspectMask,
                     depthExternalMemory,
                     &pFramebuffer->pDepthTexture);
    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pDepthTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT , VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                      depthAspectMask);
    createFramebuffer(pVulkan,
                      pFramebuffer,
                      FBR_COLOR_BUFFER_FORMAT,
                      FBR_NORMAL_BUFFER_FORMAT,
                      depthFormat,
                      extent,
                      FBR_EXTERNAL_COLOR_BUFFER_USAGE,
                      FBR_EXTERNAL_NORMAL_BUFFER_USAGE,
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