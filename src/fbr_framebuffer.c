#include "fbr_framebuffer.h"
#include "fbr_vulkan.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

// From OVR Vulkan example. Is this better/same as vulkan tutorial!?
static bool memoryTypeFromProperties(const VkPhysicalDeviceMemoryProperties memoryProperties,
                                     uint32_t nMemoryTypeBits,
                                     VkMemoryPropertyFlags nMemoryProperties,
                                     uint32_t *pTypeIndexOut) {
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((nMemoryTypeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memoryProperties.memoryTypes[i].propertyFlags & nMemoryProperties) == nMemoryProperties) {
                *pTypeIndexOut = i;
                return VK_SUCCESS;
            }
        }
        nMemoryTypeBits >>= 1;
    }

    return VK_ERROR_FORMAT_NOT_SUPPORTED;
}

static void createFramebuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFrameBuffer) {
    VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = pFrameBuffer->extent.width,
            .extent.height = pFrameBuffer->extent.height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = pFrameBuffer->imageFormat,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
            .samples = pFrameBuffer->samples,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .flags = 0,
    };

    FBR_VK_CHECK(vkCreateImage(pVulkan->device, &imageCreateInfo, NULL, &pFrameBuffer->image));

    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(pVulkan->device, pFrameBuffer->image, &memRequirements);
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(pVulkan->physicalDevice, &memProperties);
    VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size
    };
    FBR_VK_CHECK(memoryTypeFromProperties(memProperties,
                                          memRequirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          &allocInfo.memoryTypeIndex ));

    FBR_VK_CHECK(vkAllocateMemory(pVulkan->device, &allocInfo, NULL, &pFrameBuffer->deviceMemory));
    FBR_VK_CHECK(vkBindImageMemory(pVulkan->device, pFrameBuffer->image, pFrameBuffer->deviceMemory, 0));

    fbrTransitionImageLayout(pVulkan, pFrameBuffer->image,
                             pFrameBuffer->imageFormat,
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageViewCreateInfo createInfo = {
            .flags = 0,
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = pFrameBuffer->image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = pFrameBuffer->imageFormat,
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

    FBR_VK_CHECK(vkCreateImageView(pVulkan->device, &createInfo, NULL, &pFrameBuffer->imageView));

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
            .pAttachments = &pFrameBuffer->imageView,
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

void fbrCreateFramebuffer(const FbrVulkan *pVulkan, FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->extent.width = 800;
    pFramebuffer->extent.height = 600;
    pFramebuffer->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;

    createFramebuffer(pVulkan, pFramebuffer);
//    createSyncObjects(pVulkan, pFramebuffer);
}

void fbrDestroyFramebuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer) {
    FBR_LOG_DEBUG("cleaning up!");
    vkDestroyImage(pVulkan->device, pFramebuffer->image, NULL);
    vkFreeMemory(pVulkan->device, pFramebuffer->deviceMemory, NULL);
    vkDestroyImageView(pVulkan->device, pFramebuffer->imageView, NULL);
    vkDestroyFramebuffer(pVulkan->device, pFramebuffer->framebuffer, NULL);
    vkDestroyRenderPass(pVulkan->device, pFramebuffer->renderPass, NULL);

    free(pFramebuffer->image);
    free(pFramebuffer->imageView);

    vkDestroySemaphore(pVulkan->device, pFramebuffer->renderFinishedSemaphore, NULL);
    vkDestroySemaphore(pVulkan->device, pFramebuffer->imageAvailableSemaphore, NULL);
    vkDestroyFence(pVulkan->device, pFramebuffer->inFlightFence, NULL);

    free(pFramebuffer);
}