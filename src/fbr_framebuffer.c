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
                                  VkExtent2D extent)
{
    VkFormat colorFormat = FBR_COLOR_BUFFER_FORMAT;
    VkFormat normalFormat = FBR_NORMAL_BUFFER_FORMAT;
    VkFormat gBufferFormat = FBR_G_BUFFER_FORMAT;
    VkFormat depthFormat = FBR_DEPTH_BUFFER_FORMAT;
    const VkFramebufferAttachmentImageInfo pFramebufferAttachmentImageInfos[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
                    .width = extent.width,
                    .height = extent.height,
                    .layerCount = 1,
                    .usage = FBR_COLOR_BUFFER_USAGE,
                    .pViewFormats = &colorFormat,
                    .viewFormatCount = 1,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
                    .width = extent.width,
                    .height = extent.height,
                    .layerCount = 1,
                    .usage = FBR_NORMAL_BUFFER_USAGE,
                    .pViewFormats = &normalFormat,
                    .viewFormatCount = 1,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
                    .width = extent.width,
                    .height = extent.height,
                    .layerCount = 1,
                    .usage = FBR_G_BUFFER_USAGE,
                    .pViewFormats = &gBufferFormat,
                    .viewFormatCount = 1,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
                    .width = extent.width,
                    .height = extent.height,
                    .layerCount = 1,
                    .usage = FBR_DEPTH_BUFFER_USAGE,
                    .pViewFormats = &depthFormat,
                    .viewFormatCount = 1,
            }
    };
    const VkFramebufferAttachmentsCreateInfo framebufferAttachmentsCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO,
            .attachmentImageInfoCount = COUNT(pFramebufferAttachmentImageInfos),
            .pAttachmentImageInfos = pFramebufferAttachmentImageInfos,
    };
    const VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = &framebufferAttachmentsCreateInfo,
            .renderPass = pVulkan->renderPass,
            .flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
            .attachmentCount = COUNT(pFramebufferAttachmentImageInfos),
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
    };
    FBR_ACK(vkCreateFramebuffer(pVulkan->device,
                                &framebufferCreateInfo,
                                NULL,
                                &pFrameBuffer->framebuffer));
}

static FBR_RESULT createSyncObjects(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer)
{
    const VkSemaphoreCreateInfo swapchainSemaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    FBR_ACK(vkCreateSemaphore(pVulkan->device, &swapchainSemaphoreCreateInfo, NULL, &pFramebuffer->renderCompleteSemaphore));
}

//void fbrTransitionForRender(VkCommandBuffer graphicsCommandBuffer, FbrFramebuffer *pFramebuffer) {
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
//            graphicsCommandBuffer,
//            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
//            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//            0,
//            0, NULL,
//            0, NULL,
//            1, &barrier
//    );
//}
//
//void fbrTransitionForDisplay(VkCommandBuffer graphicsCommandBuffer, FbrFramebuffer *pFramebuffer) {
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
//            graphicsCommandBuffer,
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

//static void createColorFramebufferTexture(const FbrVulkan *pVulkan,
//                                    VkExtent2D extent,
//                                    VkImageUsageFlags usage,
//                                    bool external,
//                                    FbrTexture *pTexture)
//{
//    fbrCreateTexture(pVulkan,
//                     FBR_COLOR_BUFFER_FORMAT,
//                     extent,
//                     usage,
//                     VK_IMAGE_ASPECT_COLOR_BIT,
//                     external,
//                     &pTexture);
//    fbrTransitionImageLayoutImmediate(pVulkan, pTexture->image,
//                                      VK_IMAGE_LAYOUT_UNDEFINED,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//                                      0, 0,
//                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT , VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
//                                      VK_IMAGE_ASPECT_COLOR_BIT);
//}

static void initialLayoutTransition(const FbrVulkan *pVulkan, FbrTexture *pTexture, VkImageAspectFlags aspectMask){
    fbrTransitionImageLayoutImmediate(pVulkan, pTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      0, 0,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT , VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                      aspectMask);
}

static void initialImportColorLayoutTransition(const FbrVulkan *pVulkan, FbrTexture *pTexture){
    fbrTransitionImageLayoutImmediate(pVulkan,
                                      pTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
}

void fbrReleaseFramebufferFromGraphicsAttachToExternalRead(const FbrVulkan *pVulkan, const FbrFramebuffer *pFramebuffer)
{
    const VkImageMemoryBarrier pReleaseColorImageMemoryBarriers[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .image = pFramebuffer->pColorTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .image = pFramebuffer->pNormalTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .image = pFramebuffer->pGBufferTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            }
    };
    vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         COUNT(pReleaseColorImageMemoryBarriers), pReleaseColorImageMemoryBarriers);
    const VkImageMemoryBarrier pReleaseDepthImageMemoryBarriers[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .image = pFramebuffer->pDepthTexture->image,
                    FBR_DEFAULT_DEPTH_SUBRESOURCE_RANGE
            }
    };
    vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         COUNT(pReleaseDepthImageMemoryBarriers), pReleaseDepthImageMemoryBarriers);
}

void fbrAcquireFramebufferFromExternalToGraphicsAttach(const FbrVulkan *pVulkan, const FbrFramebuffer *pFramebuffer)
{
    const VkImageMemoryBarrier pAcquireColorImageMemoryBarriers[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .image = pFramebuffer->pColorTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .image = pFramebuffer->pNormalTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .image = pFramebuffer->pGBufferTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            }
    };
    vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         COUNT(pAcquireColorImageMemoryBarriers), pAcquireColorImageMemoryBarriers);
    const VkImageMemoryBarrier pAcquireDepthImageMemoryBarriers[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .image = pFramebuffer->pDepthTexture->image,
                    FBR_DEFAULT_DEPTH_SUBRESOURCE_RANGE
            }
    };
    vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, pAcquireDepthImageMemoryBarriers);
}

void fbrReleaseFramebufferFromGraphicsAttachToComputeRead(const FbrVulkan *pVulkan, const FbrFramebuffer *pFramebuffer)
{
    const VkImageMemoryBarrier pReleaseFrameBufferBarrier[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = pVulkan->computeQueueFamilyIndex,
                    .image = pFramebuffer->pColorTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = pVulkan->computeQueueFamilyIndex,
                    .image = pFramebuffer->pNormalTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = pVulkan->computeQueueFamilyIndex,
                    .image = pFramebuffer->pGBufferTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
    };
    vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ,
                         0,
                         0, NULL,
                         0, NULL,
                         COUNT(pReleaseFrameBufferBarrier), pReleaseFrameBufferBarrier);
    const VkImageMemoryBarrier pReleaseDepthFrameBufferBarrier[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = pVulkan->computeQueueFamilyIndex,
                    .image = pFramebuffer->pDepthTexture->image,
                    FBR_DEFAULT_DEPTH_SUBRESOURCE_RANGE
            },
    };
    vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ,
                         0,
                         0, NULL,
                         0, NULL,
                         COUNT(pReleaseDepthFrameBufferBarrier), pReleaseDepthFrameBufferBarrier);
}

void fbrAcquireFramebufferFromGraphicsAttachToComputeRead(const FbrVulkan *pVulkan, const FbrFramebuffer *pFramebuffer)
{
    const VkImageMemoryBarrier pTransitionBlitBarrier[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = pVulkan->computeQueueFamilyIndex,
                    .image = pFramebuffer->pColorTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = pVulkan->computeQueueFamilyIndex,
                    .image = pFramebuffer->pNormalTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = pVulkan->computeQueueFamilyIndex,
                    .image = pFramebuffer->pGBufferTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .dstQueueFamilyIndex = pVulkan->computeQueueFamilyIndex,
                    .image = pFramebuffer->pDepthTexture->image,
                    FBR_DEFAULT_DEPTH_SUBRESOURCE_RANGE
            },
    };
    vkCmdPipelineBarrier(pVulkan->computeCommandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         COUNT(pTransitionBlitBarrier), pTransitionBlitBarrier);
}

void fbrAcquireFramebufferFromExternalAttachToGraphicsRead(const FbrVulkan *pVulkan, const FbrFramebuffer *pFramebuffer)
{
    const VkImageMemoryBarrier pAcquireChildColorFrameBufferBarrier[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .image = pFramebuffer->pColorTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .image = pFramebuffer->pNormalTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .image = pFramebuffer->pGBufferTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
    };
    vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         COUNT(pAcquireChildColorFrameBufferBarrier), pAcquireChildColorFrameBufferBarrier);
    const VkImageMemoryBarrier pAcquireChildDepthFrameBufferBarrier[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                    .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                    .image = pFramebuffer->pDepthTexture->image,
                    FBR_DEFAULT_DEPTH_SUBRESOURCE_RANGE
            }
    };
    vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         COUNT(pAcquireChildDepthFrameBufferBarrier), pAcquireChildDepthFrameBufferBarrier);
}

void fbrTransitionFramebufferFromIgnoredReadToGraphicsAttach(const FbrVulkan *pVulkan, const FbrFramebuffer *pFramebuffer)
{
    const VkImageMemoryBarrier pAcquireFrameBufferBarrier[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = pFramebuffer->pColorTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = pFramebuffer->pNormalTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = pFramebuffer->pGBufferTexture->image,
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
            },
    };
    vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ,
                         0,
                         0, NULL,
                         0, NULL,
                         COUNT(pAcquireFrameBufferBarrier), pAcquireFrameBufferBarrier);
    const VkImageMemoryBarrier pAcquireDepthFrameBufferBarrier[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
                    .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = pFramebuffer->pDepthTexture->image,
                    FBR_DEFAULT_DEPTH_SUBRESOURCE_RANGE
            },
    };
    vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT ,
                         0,
                         0, NULL,
                         0, NULL,
                         COUNT(pAcquireDepthFrameBufferBarrier), pAcquireDepthFrameBufferBarrier);
}

void fbrCreateFrameBuffer(const FbrVulkan *pVulkan,
                          bool external,
                          VkFormat colorFormat,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;

    // Color
    fbrCreateTexture(pVulkan,
                     FBR_COLOR_BUFFER_FORMAT,
                     extent,
                     FBR_COLOR_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     external,
                     &pFramebuffer->pColorTexture);
    initialLayoutTransition(pVulkan, pFramebuffer->pColorTexture, VK_IMAGE_ASPECT_COLOR_BIT);

    // Normal
    fbrCreateTexture(pVulkan,
                     FBR_NORMAL_BUFFER_FORMAT,
                     extent,
                     FBR_NORMAL_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     external,
                     &pFramebuffer->pNormalTexture);
    initialLayoutTransition(pVulkan, pFramebuffer->pNormalTexture, VK_IMAGE_ASPECT_COLOR_BIT);

    // GBuffer
    fbrCreateTexture(pVulkan,
                     FBR_G_BUFFER_FORMAT,
                     extent,
                     FBR_G_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     external,
                     &pFramebuffer->pGBufferTexture);
    initialLayoutTransition(pVulkan, pFramebuffer->pGBufferTexture, VK_IMAGE_ASPECT_COLOR_BIT);

    //Depth
    fbrCreateTexture(pVulkan,
                     FBR_DEPTH_BUFFER_FORMAT,
                     extent,
                     FBR_DEPTH_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_DEPTH_BIT,
                     external,
                     &pFramebuffer->pDepthTexture);
    initialLayoutTransition(pVulkan, pFramebuffer->pDepthTexture, VK_IMAGE_ASPECT_DEPTH_BIT);

    createFramebuffer(pVulkan,
                      pFramebuffer,
                      extent);

    createSyncObjects(pVulkan, pFramebuffer);
}

void fbrImportFrameBuffer(const FbrVulkan *pVulkan,
                          HANDLE colorExternalMemory,
                          HANDLE normalExternalMemory,
                          HANDLE gbufferExternalMemory,
                          HANDLE depthExternalMemory,
                          VkFormat colorFormat,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer)
{
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;

    // color
    fbrImportTexture(pVulkan,
                     FBR_COLOR_BUFFER_FORMAT,
                     extent,
                     FBR_COLOR_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     colorExternalMemory,
                     &pFramebuffer->pColorTexture);
    initialImportColorLayoutTransition(pVulkan, pFramebuffer->pColorTexture);

    // normal
    fbrImportTexture(pVulkan,
                     FBR_NORMAL_BUFFER_FORMAT,
                     extent,
                     FBR_NORMAL_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     normalExternalMemory,
                     &pFramebuffer->pNormalTexture);
    initialImportColorLayoutTransition(pVulkan, pFramebuffer->pNormalTexture);

    // GBuffer
    fbrImportTexture(pVulkan,
                     FBR_G_BUFFER_FORMAT,
                     extent,
                     FBR_G_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     gbufferExternalMemory,
                     &pFramebuffer->pGBufferTexture);
    initialImportColorLayoutTransition(pVulkan, pFramebuffer->pGBufferTexture);

    // depth
    fbrImportTexture(pVulkan,
                     FBR_DEPTH_BUFFER_FORMAT,
                     extent,
                     FBR_DEPTH_BUFFER_USAGE,
                     VK_IMAGE_ASPECT_DEPTH_BIT,
                     depthExternalMemory,
                     &pFramebuffer->pDepthTexture);
    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pDepthTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT , VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                      VK_IMAGE_ASPECT_DEPTH_BIT);

    createFramebuffer(pVulkan,
                      pFramebuffer,
                      extent);
}

void fbrDestroyFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer) {
    fbrDestroyTexture(pVulkan, pFramebuffer->pColorTexture);
    fbrDestroyTexture(pVulkan, pFramebuffer->pNormalTexture);
    fbrDestroyTexture(pVulkan, pFramebuffer->pGBufferTexture);
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