#include "fbr_framebuffer.h"
#include "fbr_vulkan.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

static void createFramebuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFrameBuffer) {
//    VkImageCreateInfo imageCreateInfo = {
//            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
//            .imageType = VK_IMAGE_TYPE_2D,
//            .extent.width = pFrameBuffer->extent.width,
//            .extent.height = pFrameBuffer->extent.height,
//            .extent.depth = 1,
//            .mipLevels = 1,
//            .arrayLayers = 1,
//            .format = pFrameBuffer->imageFormat,
//            .tiling = VK_IMAGE_TILING_OPTIMAL,
//            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
//            .usage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
//            .samples = pFrameBuffer->samples,
//            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
//            .flags = 0,
//    };
//
//    FBR_VK_CHECK(vkCreateImage(pVulkan->device, &imageCreateInfo, NULL, &pFrameBuffer->pTexture->image));
//
//    VkMemoryRequirements memRequirements = {};
//    uint32_t memTypeIndex;
//    FBR_VK_CHECK(fbrImageMemoryTypeFromProperties(pVulkan,
//                                                  pFrameBuffer->pTexture->image,
//                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                                                  &memRequirements,
//                                                  &memTypeIndex));
//
//    VkMemoryAllocateInfo allocInfo = {
//            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
//            .allocationSize = memRequirements.size,
//            .memoryTypeIndex = memTypeIndex
//
//    };
//    FBR_VK_CHECK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, &pFrameBuffer->pTexture->deviceMemory));
//    FBR_VK_CHECK(vkBindImageMemory(pVulkan->device, pFrameBuffer->pTexture->image, pFrameBuffer->pTexture->deviceMemory, 0));
//
//    fbrTransitionImageLayout(pVulkan, pFrameBuffer->pTexture->image,
//                             pFrameBuffer->imageFormat,
//                             VK_IMAGE_LAYOUT_UNDEFINED,
//                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

//    VkImageViewCreateInfo createInfo = {
//            .flags = 0,
//            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
//            .image = pFrameBuffer->pTexture->image,
//            .viewType = VK_IMAGE_VIEW_TYPE_2D,
//            .format = pFrameBuffer->imageFormat,
//            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
//            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
//            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
//            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
//            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//            .subresourceRange.baseMipLevel = 0,
//            .subresourceRange.levelCount = 1,
//            .subresourceRange.baseArrayLayer = 0,
//            .subresourceRange.layerCount = 1,
//    };
//
//    FBR_VK_CHECK(vkCreateImageView(pVulkan->device, &createInfo, NULL, &pFrameBuffer->pTexture->imageView));

    VkAttachmentDescription colorAttachment = {
            .format = pFrameBuffer->imageFormat,
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

    VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .flags = 0,
            .inputAttachmentCount = 0,
            .pInputAttachments = NULL,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
            .pResolveAttachments = NULL,
            .pDepthStencilAttachment = NULL,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = NULL,
    };

    // OVR example doesn't have this
    VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .flags = 0,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 0,
            .pDependencies = NULL,
    };

    FBR_VK_CHECK(vkCreateRenderPass(pVulkan->device, &renderPassInfo, NULL, &pFrameBuffer->renderPass));

    VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = pFrameBuffer->renderPass,
            .attachmentCount = 1,
            .pAttachments = &pFrameBuffer->pTexture->imageView,
            .width = pFrameBuffer->extent.width,
            .height = pFrameBuffer->extent.height,
            .layers = 1,
    };

    FBR_VK_CHECK(vkCreateFramebuffer(pVulkan->device, &framebufferCreateInfo, NULL, &pFrameBuffer->framebuffer));
}

static void createSyncObjects(const FbrVulkan *pVulkan, FbrFramebuffer *pFrameBuffer) {
    VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    FBR_VK_CHECK(vkCreateSemaphore(pVulkan->device, &semaphoreInfo, NULL, &pFrameBuffer->imageAvailableSemaphore));
    FBR_VK_CHECK(vkCreateSemaphore(pVulkan->device, &semaphoreInfo, NULL, &pFrameBuffer->renderFinishedSemaphore));
    FBR_VK_CHECK(vkCreateFence(pVulkan->device, &fenceInfo, NULL, &pFrameBuffer->inFlightFence));
}

void fbrTransitionForRender(VkCommandBuffer commandBuffer, FbrFramebuffer *pFramebuffer) {
    VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = pFramebuffer->pTexture->image,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
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
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = pFramebuffer->pTexture->image,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
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

void fbrCreateFramebuffer(const FbrVulkan *pVulkan, FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->extent.width = 800;
    pFramebuffer->extent.height = 600;
    pFramebuffer->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;

//    pFramebuffer->pTexture = calloc(1, sizeof(FbrFramebuffer));
    fbrCreateFramebufferTexture(pVulkan, &pFramebuffer->pTexture, 800, 600);
    createFramebuffer(pVulkan, pFramebuffer);
//    createSyncObjects(pVulkan, pFramebuffer);
}

void fbrCreateFramebufferFromExternalMemory(const FbrVulkan *pVulkan, FbrFramebuffer **ppAllocFramebuffer, HANDLE externalMemory, int width, int height) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->extent.width = width;
    pFramebuffer->extent.height = height;
    pFramebuffer->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;

//    pFramebuffer->pTexture = calloc(1, sizeof(FbrFramebuffer));
    fbrCreateFramebufferTextureFromExternalMemory(pVulkan, &pFramebuffer->pTexture, externalMemory, 800, 600);
    createFramebuffer(pVulkan, pFramebuffer);
//    createSyncObjects(pVulkan, pFramebuffer);
}

void fbrDestroyFramebuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer) {
    fbrDestroyTexture(pVulkan, pFramebuffer->pTexture);

    vkDestroyFramebuffer(pVulkan->device, pFramebuffer->framebuffer, NULL);
    vkDestroyRenderPass(pVulkan->device, pFramebuffer->renderPass, NULL);


    vkDestroySemaphore(pVulkan->device, pFramebuffer->renderFinishedSemaphore, NULL);
    vkDestroySemaphore(pVulkan->device, pFramebuffer->imageAvailableSemaphore, NULL);
    vkDestroyFence(pVulkan->device, pFramebuffer->inFlightFence, NULL);

    free(pFramebuffer);
}