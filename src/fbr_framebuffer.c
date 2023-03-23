#include "fbr_framebuffer.h"
#include "fbr_vulkan.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

static VkResult createFramebuffer(const FbrVulkan *pVulkan,
                                  FbrFramebuffer *pFrameBuffer,
                                  VkFormat format,
                                  VkExtent2D extent,
                                  VkImageUsageFlags usage) {
    const VkFramebufferAttachmentImageInfo framebufferAttachmentImageInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
            .width = extent.width,
            .height = extent.height,
            .layerCount = 1,
            .usage = usage,
            .pViewFormats = &format,
            .viewFormatCount = 1,
    };
    const VkFramebufferAttachmentsCreateInfo framebufferAttachmentsCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO,
            .attachmentImageInfoCount = 1,
            .pAttachmentImageInfos = &framebufferAttachmentImageInfo,
    };
    const VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = &framebufferAttachmentsCreateInfo,
            .renderPass = pVulkan->renderPass,
            .flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
            .attachmentCount = 1,
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
    };
    VK_CHECK(vkCreateFramebuffer(pVulkan->device,
                                 &framebufferCreateInfo,
                                 NULL,
                                 &pFrameBuffer->framebuffer));
}

void fbrTransitionForRender(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer) {
    VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
            .image = pFramebuffer->pTexture->image,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
    };
    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &barrier
    );
}

void fbrTransitionForDisplay(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer) {
    VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
            .image = pFramebuffer->pTexture->image,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
    };
    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &barrier
    );
}

void fbrCreateFrameBufferFromImage(const FbrVulkan *pVulkan,
                                   VkFormat format,
                                   VkExtent2D extent,
                                   VkImage image,
                                   FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;
    fbrCreateTextureFromImage(pVulkan,
                              format,
                              extent,
                              image,
                              &pFramebuffer->pTexture);
    createFramebuffer(pVulkan,
                      pFramebuffer,
                      format,
                      extent,
                      FBR_FRAMEBUFFER_USAGE);
}

void fbrCreateFrameBuffer(const FbrVulkan *pVulkan,
                          bool external,
                          VkFormat format,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;
    fbrCreateTexture(pVulkan,
                     format,
                     extent,
                     external ? FBR_EXTERNAL_FRAMEBUFFER_USAGE : FBR_FRAMEBUFFER_USAGE,
                     external,
                     &pFramebuffer->pTexture);
    // You don't need to do this on nvidia ??
    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      VK_ACCESS_NONE_KHR, VK_ACCESS_SHADER_READ_BIT,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    createFramebuffer(pVulkan,
                      pFramebuffer,
                      format,
                      extent,
                      external ? FBR_EXTERNAL_FRAMEBUFFER_USAGE : FBR_FRAMEBUFFER_USAGE);
}

void fbrImportFrameBuffer(const FbrVulkan *pVulkan,
                          HANDLE externalMemory,
                          VkFormat format,
                          VkExtent2D extent,
                          FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;
    fbrImportTexture(pVulkan,
                     format,
                     extent,
                     FBR_EXTERNAL_FRAMEBUFFER_USAGE,
                     externalMemory,
                     &pFramebuffer->pTexture);
    fbrTransitionImageLayoutImmediate(pVulkan,
                                      pFramebuffer->pTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE_KHR, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    createFramebuffer(pVulkan,
                      pFramebuffer,
                      format,
                      extent,
                      FBR_EXTERNAL_FRAMEBUFFER_USAGE);
}

void fbrDestroyFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer) {
    fbrDestroyTexture(pVulkan, pFramebuffer->pTexture);
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