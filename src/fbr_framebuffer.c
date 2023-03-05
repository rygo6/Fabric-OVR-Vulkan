#include "fbr_framebuffer.h"
#include "fbr_vulkan.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

static VkResult createFramebuffer(const FbrVulkan *pVulkan,
                                  FbrFramebuffer *pFrameBuffer,
                                  VkImageUsageFlags usage,
                                  VkFormat format,
                                  VkExtent2D extent) {
    const VkAttachmentDescription colorAttachment = {
            .format = format,
            .samples = pFrameBuffer->samples,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            // different in OVR example
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .flags = 0,
    };
    const VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
    };
    const VkSubpassDependency dependencies[2] = {
            {
                    // https://gist.github.com/chrisvarns/b4a5dbd1a09545948261d8c650070383
                    // In subpass zero...
                    .dstSubpass = 0,
                    // ... at this pipeline stage ...
                    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    // ... wait before performing these operations ...
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    // ... until all operations of that type stop ...
                    .srcAccessMask = VK_ACCESS_NONE_KHR,
                    // ... at that same stages ...
                    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    // ... occuring in submission order prior to vkCmdBeginRenderPass ...
                    .srcSubpass = VK_SUBPASS_EXTERNAL,
                    // ... have completed execution.
                    .dependencyFlags = 0,
            },
            {
                    // ... In the external scope after the subpass ...
                    .dstSubpass = VK_SUBPASS_EXTERNAL,
                    // ... before anything can occur with this pipeline stage ...
                    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    // ... wait for all operations to stop ...
                    .dstAccessMask = VK_ACCESS_NONE_KHR,
                    // ... of this type ...
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    // ... at this stage ...
                    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    // ... in subpass 0 ...
                    .srcSubpass = 0,
                    // ... before it can execute and signal the semaphore rendering complete semaphore
                    // set to VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR on vkQueueSubmit2KHR  .
                    .dependencyFlags = 0,
            },
    };
    const VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .flags = 0,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 2,
            .pDependencies = dependencies,
    };
    FBR_VK_CHECK_RETURN(vkCreateRenderPass(pVulkan->device, &renderPassInfo, NULL, &pFrameBuffer->renderPass));

    const VkFramebufferAttachmentImageInfo framebufferAttachmentImageInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
            .width = pVulkan->swapExtent.width,
            .height = pVulkan->swapExtent.height,
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
            .renderPass = pFrameBuffer->renderPass,
            .flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
            .attachmentCount = 1,
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
    };
    FBR_VK_CHECK_RETURN(vkCreateFramebuffer(pVulkan->device, &framebufferCreateInfo, NULL, &pFrameBuffer->framebuffer));
}

//static VkResult createSyncObjects(const FbrVulkan *pVulkan, FbrFramebuffer *pFrameBuffer) {
//    VkSemaphoreTypeCreateInfo timelineSemaphoreTypeCreateInfo = {
//            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
//            .pNext = NULL,
//            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
//            .initialValue = 0,
//    };
//    VkSemaphoreCreateInfo timelineSemaphoreCreateInfo = {
//            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
//            .pNext = &timelineSemaphoreTypeCreateInfo,
//            .flags = 0,
//    };
//    FBR_VK_CHECK_RETURN(vkCreateSemaphore(pVulkan->device, &timelineSemaphoreCreateInfo, NULL, &pFrameBuffer->semaphore));
//}

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

void fbrCreateExternalFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer **ppAllocFramebuffer, VkExtent2D extent) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;

    fbrCreateExternalTexture(pVulkan,
                             &pFramebuffer->pTexture,
                             extent,
                             FBR_EXTERNAL_FRAMEBUFFER_USAGE,
                             pVulkan->swapImageFormat);
    fbrTransitionImageLayoutImmediate(pVulkan, pFramebuffer->pTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      VK_ACCESS_NONE_KHR, VK_ACCESS_SHADER_READ_BIT,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    createFramebuffer(pVulkan,
                      pFramebuffer,
                      FBR_EXTERNAL_FRAMEBUFFER_USAGE,
                      pVulkan->swapImageFormat,
                      extent);
//    createSyncObjects(pVulkan, pFramebuffer);
}

void fbrImportFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer **ppAllocFramebuffer, HANDLE externalMemory, VkExtent2D extent) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;

    fbrImportTexture(pVulkan,
                     &pFramebuffer->pTexture,
                     externalMemory,
                     extent,
                     FBR_EXTERNAL_FRAMEBUFFER_USAGE,
                     pVulkan->swapImageFormat);
    fbrTransitionImageLayoutImmediate(pVulkan,
                                      pFramebuffer->pTexture->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_ACCESS_NONE_KHR, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    createFramebuffer(pVulkan,
                      pFramebuffer,
                      FBR_EXTERNAL_FRAMEBUFFER_USAGE,
                      pVulkan->swapImageFormat,
                      extent);
//    createSyncObjects(pVulkan, pFramebuffer);
}

void fbrDestroyFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer) {
    fbrDestroyTexture(pVulkan, pFramebuffer->pTexture);

    vkDestroyFramebuffer(pVulkan->device, pFramebuffer->framebuffer, NULL);
    vkDestroyRenderPass(pVulkan->device, pFramebuffer->renderPass, NULL);

//    vkDestroySemaphore(pVulkan->device, pFramebuffer->semaphore, NULL);

    free(pFramebuffer);
}

void fbrIPCTargetImportFrameBuffer(FbrApp *pApp, FbrIPCParamImportFrameBuffer *pParam) {
    FBR_LOG_DEBUG("Importing Framebuffer.", pParam->handle, pParam->width, pParam->height);
//    fbrImportFrameBuffer(pApp->pVulkan,
//                         &pApp->pParentProcessFramebuffer,
//                         pParam->handle,
//                         (VkExtent2D) {pParam->width, pParam->height});
}