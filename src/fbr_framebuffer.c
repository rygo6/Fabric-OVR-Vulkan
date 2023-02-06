#include "fbr_framebuffer.h"
#include "fbr_vulkan.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

static void createFramebuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFrameBuffer) {
    VkAttachmentDescription colorAttachment = {
            .format = FBR_DEFAULT_TEXTURE_FORMAT,
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
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
    };

    VkSubpassDependency dependencies[2] = {
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
    VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .flags = 0,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 2,
            .pDependencies = dependencies,
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

void fbrCreateFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer **ppAllocFramebuffer) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->extent.width = 800;
    pFramebuffer->extent.height = 600;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;

//    pFramebuffer->pTexture = calloc(1, sizeof(FbrFramebuffer));
    fbrCreateExternalFramebufferTexture(pVulkan, &pFramebuffer->pTexture, pFramebuffer->extent.width,
                                        pFramebuffer->extent.height);
    createFramebuffer(pVulkan, pFramebuffer);
//    createSyncObjects(pVulkan, pFramebuffer);

    FBR_LOG_DEBUG("Created Framebuffer.");
}

void fbrImportFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer **ppAllocFramebuffer, HANDLE externalMemory, int width, int height) {
    *ppAllocFramebuffer = calloc(1, sizeof(FbrFramebuffer));
    FbrFramebuffer *pFramebuffer = *ppAllocFramebuffer;
    pFramebuffer->extent.width = width;
    pFramebuffer->extent.height = height;
    pFramebuffer->samples = VK_SAMPLE_COUNT_1_BIT;

//    pFramebuffer->pTexture = calloc(1, sizeof(FbrFramebuffer));
    fbrImportFramebufferTexture(pVulkan, &pFramebuffer->pTexture, externalMemory, 800, 600);
    createFramebuffer(pVulkan, pFramebuffer);
//    createSyncObjects(pVulkan, pFramebuffer);
}

void fbrDestroyFrameBuffer(const FbrVulkan *pVulkan, FbrFramebuffer *pFramebuffer) {
    fbrDestroyTexture(pVulkan, pFramebuffer->pTexture);

    vkDestroyFramebuffer(pVulkan->device, pFramebuffer->framebuffer, NULL);
    vkDestroyRenderPass(pVulkan->device, pFramebuffer->renderPass, NULL);


    vkDestroySemaphore(pVulkan->device, pFramebuffer->renderFinishedSemaphore, NULL);
    vkDestroySemaphore(pVulkan->device, pFramebuffer->imageAvailableSemaphore, NULL);
    vkDestroyFence(pVulkan->device, pFramebuffer->inFlightFence, NULL);

    free(pFramebuffer);
}

void fbrIPCTargetImportFrameBuffer(FbrApp *pApp, FbrIPCParamImportFrameBuffer *pParam) {
    printf("external pTexture handle d %lld\n", pParam->handle);
    printf("external pTexture handle p %p\n", pParam->handle);

    printf("WIDTH %d\n", pParam->width);
    printf("HEIGHT %d\n", pParam->height);

//    fbrCreateMesh(pApp->pVulkan, &pApp->pMeshExternalTest);
//    fbrImportTexture(pApp->pVulkan, &pApp->pTextureExternalTest, pParam->handle, pParam->width, pParam->height);
//    glm_vec3_add(pApp->pMeshExternalTest->transform.pos, (vec3) {1,0,0}, pApp->pMeshExternalTest->transform.pos);
//    fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTextureExternalTest->imageView, pApp->pVulkan->swapRenderPass, &pApp->pPipelineExternalTest);

    //render to framebuffer
    fbrImportFrameBuffer(pApp->pVulkan, &pApp->pFramebuffer, pParam->handle, pParam->width, pParam->height);
//    fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTexture->imageView, pApp->pFramebuffer->swapRenderPass, &pApp->pFramebufferPipeline); // is this pipeline needed!?

//    fbrCreateMesh(pApp->pVulkan, &pApp->pMeshExternalTest);
//    glm_vec3_add(pApp->pMeshExternalTest->transform.pos, (vec3) {1,0,0}, pApp->pMeshExternalTest->transform.pos);
//    fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pFramebuffer->pTexture->imageView, pApp->pVulkan->swapRenderPass, &pApp->pPipelineExternalTest);
}