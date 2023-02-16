#include "fbr_core.h"
#include "fbr_mesh.h"
#include "fbr_pipeline.h"
#include "fbr_texture.h"
#include "fbr_camera.h"
#include "fbr_log.h"
#include "fbr_vulkan.h"
#include "fbr_input.h"
#include "fbr_node.h"

#include "cglm/cglm.h"

static void waitForQueueFence(FbrVulkan *pVulkan) {
    vkWaitForFences(pVulkan->device, 1, &pVulkan->queueFence, VK_TRUE, UINT64_MAX);
    vkResetFences(pVulkan->device, 1, &pVulkan->queueFence);
}

static void waitForTimeLine(FbrVulkan *pVulkan, VkSemaphore *pTimelineSemaphore) {
    const uint64_t waitValue = pVulkan->timelineValue;
    const VkSemaphoreWaitInfo semaphoreWaitInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = NULL,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = pTimelineSemaphore,
            .pValues = &waitValue,
    };
    vkWaitSemaphores(pVulkan->device, &semaphoreWaitInfo, UINT64_MAX);
}

static void processInputFrame(FbrApp *pApp) {
    fbrProcessInput();
    for (int i = 0; i < fbrInputEventCount(); ++i) {
        const FbrInputEvent *pInputEvent = fbrGetKeyEvent(i);

        switch (pInputEvent->type) {
            case FBR_NO_INPUT:
                break;
            case FBR_KEY_INPUT: {
                if (pInputEvent->keyInput.action != GLFW_RELEASE && pInputEvent->keyInput.key == GLFW_KEY_ESCAPE){
                    pApp->exiting = true;
                }
                break;
            }
            case FBR_MOUSE_POS_INPUT: {
                break;
            }
            case FBR_MOUSE_BUTTON_INPUT: {
                break;
            }
        }

        fbrUpdateCamera(pApp->pCamera, pInputEvent, pApp->pTime);
    }
}

static void beginRenderPass(const FbrVulkan *pVulkan, VkRenderPass renderPass, VkFramebuffer framebuffer) {
    VkClearValue clearColor = {{{0.1f, 0.2f, 0.05f, 1.0f}}};
    VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = framebuffer,
            .renderArea.extent = pVulkan->swapExtent,
            .clearValueCount = 1,
            .pClearValues = &clearColor,
    };
    vkCmdBeginRenderPass(pVulkan->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

static void beginRenderPassImageless(const FbrVulkan *pVulkan, VkRenderPass renderPass, VkFramebuffer framebuffer, VkImageView imageView) {
    VkClearValue clearColor = {{{0.1f, 0.2f, 0.05f, 1.0f}}};
    VkRenderPassAttachmentBeginInfo renderPassAttachmentBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO,
            .attachmentCount = 1,
            .pAttachments = &imageView
    };
    VkRenderPassBeginInfo vkRenderPassBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = &renderPassAttachmentBeginInfo,
            .renderPass = renderPass,
            .framebuffer = framebuffer,
            .renderArea.extent = pVulkan->swapExtent,
            .clearValueCount = 1,
            .pClearValues = &clearColor,
    };
    vkCmdBeginRenderPass(pVulkan->commandBuffer, &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

static void recordRenderPass(const FbrVulkan *pVulkan, const FbrPipeline *pPipeline, const FbrMesh *pMesh, VkDescriptorSet descriptorSet) {
    vkCmdBindPipeline(pVulkan->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->graphicsPipeline);

    vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pPipeline->pipelineLayout,
                            0,
                            1,
                            &descriptorSet,
                            0,
                            NULL);

    VkBuffer vertexBuffers[] = {pMesh->vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(pVulkan->commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(pVulkan->commandBuffer, pMesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    //upload the matrix to the GPU via push constants
    vkCmdPushConstants(pVulkan->commandBuffer, pPipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), pMesh->transform.matrix);

    vkCmdDrawIndexed(pVulkan->commandBuffer, pMesh->indexCount, 1, 0, 0, 0);
}

static void submitQueue(FbrVulkan *pVulkan) {
    const uint64_t waitValue = pVulkan->timelineValue++;
    const uint64_t signalValue = pVulkan->timelineValue;
    const VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                    .semaphore = pVulkan->timelineSemaphore,
                    .value = waitValue,
                    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    const VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                    .semaphore = pVulkan->timelineSemaphore,
                    .value = signalValue,
                    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    const VkCommandBufferSubmitInfo commandBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
            .commandBuffer = pVulkan->commandBuffer,
    };
    const VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
            .pNext = VK_NULL_HANDLE,
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &waitSemaphoreSubmitInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalSemaphoreSubmitInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
    };
    vkQueueSubmit2(pVulkan->queue,
                   1,
                   &submitInfo,
//                   pVulkan->queueFence);
                   VK_NULL_HANDLE);
}

static void submitQueueAndPresent(FbrApp *pApp, uint32_t swapIndex) {
    // https://www.khronos.org/blog/vulkan-timeline-semaphores
    const uint64_t waitValue = pApp->pVulkan->timelineValue++;
    const uint64_t signalValue = pApp->pVulkan->timelineValue;
    const VkSemaphoreSubmitInfo waitSemaphoreSubmitInfos[2] = {
            {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                    .semaphore = pApp->pVulkan->timelineSemaphore,
                    .value = waitValue,
                    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
            },
            {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                    .semaphore = pApp->pVulkan->swapAcquireComplete,
                    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
            }
    };
    const VkSemaphoreSubmitInfo signalSemaphoreSubmitInfos[2] = {
            {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                    .semaphore = pApp->pVulkan->timelineSemaphore,
                    .value = signalValue,
                    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .deviceIndex = 0 //todo use this?
            },
            {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                    .semaphore = pApp->pVulkan->renderCompleteSemaphore,
                    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
            }
    };
    const VkCommandBufferSubmitInfo commandBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
            .commandBuffer = pApp->pVulkan->commandBuffer,
    };
    const VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
            .pNext = VK_NULL_HANDLE,
            .waitSemaphoreInfoCount = 2,
            .pWaitSemaphoreInfos = waitSemaphoreSubmitInfos,
            .signalSemaphoreInfoCount = 2,
            .pSignalSemaphoreInfos = signalSemaphoreSubmitInfos,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
    };
    vkQueueSubmit2(pApp->pVulkan->queue,
                   1,
                   &submitInfo,
//                   pApp->pVulkan->queueFence);
                   VK_NULL_HANDLE);


    const VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &pApp->pVulkan->renderCompleteSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &pApp->pVulkan->swapChain,
            .pImageIndices = &swapIndex,
    };
    vkQueuePresentKHR(pApp->pVulkan->queue, &presentInfo);
}

static void beginFrameCommandBuffer(FbrApp *pApp) {
    vkResetCommandBuffer(pApp->pVulkan->commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo beginInfo = {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    vkBeginCommandBuffer(pApp->pVulkan->commandBuffer, &beginInfo);

    VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) pApp->pVulkan->swapExtent.width,
            .height = (float) pApp->pVulkan->swapExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    vkCmdSetViewport(pApp->pVulkan->commandBuffer, 0, 1, &viewport);
    VkRect2D scissor = {
            .offset = {0, 0},
            .extent = pApp->pVulkan->swapExtent,
    };
    vkCmdSetScissor(pApp->pVulkan->commandBuffer, 0, 1, &scissor);
}

static void childMainLoop(FbrApp *pApp) {
    double lastFrameTime = glfwGetTime();

    FbrVulkan *pVulkan = pApp->pVulkan;

    vkGetSemaphoreCounterValue(pApp->pVulkan->device, pApp->parentTimelineSemaphore, &pApp->parentTimelineValue);
    FBR_LOG_DEBUG("stating parent timeline value", pApp->parentTimelineValue);
    pApp->parentTimelineValue += 20;

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {
        pApp->pTime->currentTime = glfwGetTime();
        pApp->pTime->deltaTime = pApp->pTime->currentTime - lastFrameTime;
        lastFrameTime = pApp->pTime->currentTime;

        waitForTimeLine(pVulkan, &pApp->parentTimelineSemaphore);

        const uint64_t waitValues[2] = { pApp->parentTimelineValue, pVulkan->timelineValue };
        const VkSemaphore waitSemaphores[2] = { pApp->parentTimelineSemaphore, pVulkan->timelineSemaphore };
        const VkSemaphoreWaitInfo semaphoreWaitInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
                .pNext = NULL,
                .flags = 0,
                .semaphoreCount = 2,
                .pSemaphores = waitSemaphores,
                .pValues = waitValues,
        };
        vkWaitSemaphores(pVulkan->device, &semaphoreWaitInfo, UINT64_MAX);
        pApp->parentTimelineValue += 20;

        // for some reason this fixes a bug with validation layers thinking the queue hasnt finished
        // wait on timeline should be enough!!!
        vkQueueWaitIdle(pVulkan->queue);


        uint64_t value;
        vkGetSemaphoreCounterValue(pApp->pVulkan->device, pApp->parentTimelineSemaphore, &value);
        FBR_LOG_DEBUG("import semaphore", value);


        beginFrameCommandBuffer(pApp);

        beginRenderPassImageless(pApp->pVulkan,
                                 pApp->pParentProcessFramebuffer->renderPass,
                                 pApp->pParentProcessFramebuffer->framebuffer,
                                 pApp->pParentProcessFramebuffer->pTexture->imageView);
        //cube 1
        fbrUpdateTransformMatrix(&pApp->pMesh->transform);
        recordRenderPass(pApp->pVulkan, pApp->pPipeline, pApp->pMesh, pApp->parentFramebufferDescriptorSet);
        // end framebuffer pass
        vkCmdEndRenderPass(pApp->pVulkan->commandBuffer);

        //swap pass
//            beginRenderPass(pApp->pVulkan, pApp->pVulkan->swapRenderPass, pApp->pVulkan->pSwapChainFramebuffers[swapIndex]);
//            //cube 1
//            fbrUpdateTransformMatrix(&pApp->pMesh->transform);
//            recordRenderPass(pApp->pVulkan, pApp->pPipeline, pApp->pMesh);
//            // end swap pass
//            vkCmdEndRenderPass(pApp->pVulkan->commandBuffer);

        vkEndCommandBuffer(pApp->pVulkan->commandBuffer);
        submitQueue(pApp->pVulkan);
    }

}

static void parentMainLoop(FbrApp *pApp) {
    double lastFrameTime = glfwGetTime();

    bool firstRun = true;

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {

        Sleep(33);
//        FBR_LOG_DEBUG("Loop!", pApp->pVulkan->timelineValue);

        pApp->pTime->currentTime = glfwGetTime();
        pApp->pTime->deltaTime = pApp->pTime->currentTime - lastFrameTime;
        lastFrameTime = pApp->pTime->currentTime;

        waitForTimeLine(pApp->pVulkan, &pApp->pVulkan->timelineSemaphore);

        // for some reason this fixes a bug with validation layers thinking the queue hasnt finished
        // wait on timeline should be enough!!
        vkQueueWaitIdle(pApp->pVulkan->queue);


        processInputFrame(pApp);
        fbrUpdateCameraUBO(pApp->pCamera);

        // somehow need to signal semaphore after UBO updating
//        VkSemaphoreSignalInfo signalInfo;
//        signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
//        signalInfo.pNext = NULL;
//        signalInfo.semaphore = timeline;
//        signalInfo.value = 7;
//        vkSignalSemaphore(dev, &signalInfo);

        // Get open swap image
        uint32_t swapIndex;
        vkAcquireNextImageKHR(pApp->pVulkan->device,
                              pApp->pVulkan->swapChain,
                              UINT64_MAX,
                              pApp->pVulkan->swapAcquireComplete,
                              VK_NULL_HANDLE,
                              &swapIndex);

        // TODO this should be possible
//        if (firstRun) {
//            firstRun = false;

            beginFrameCommandBuffer(pApp);

            //swap pass
            beginRenderPassImageless(pApp->pVulkan, pApp->pVulkan->swapRenderPass, pApp->pVulkan->swapFramebuffer, pApp->pVulkan->pSwapImageViews[swapIndex]);
            //cube 1
            fbrUpdateTransformMatrix(&pApp->pMesh->transform);
            recordRenderPass(pApp->pVulkan, pApp->pPipeline, pApp->pMesh, pApp->pVulkan->swapDescriptorSet);
            //cube2
            fbrUpdateTransformMatrix(&pApp->pTestNodeDisplayMesh->transform);
            recordRenderPass(pApp->pVulkan, pApp->pPipeline, pApp->pTestNodeDisplayMesh, pApp->testNodeDisplayDescriptorSet);
            // end swap pass
            vkCmdEndRenderPass(pApp->pVulkan->commandBuffer);

            vkEndCommandBuffer(pApp->pVulkan->commandBuffer);
//        }

        submitQueueAndPresent(pApp, swapIndex);
    }
}

void fbrMainLoop(FbrApp *pApp) {
    FBR_LOG_DEBUG("mainloop starting!");

    if (!pApp->isChild) {
        parentMainLoop(pApp);
    } else {
        childMainLoop(pApp);
    }

    vkQueueWaitIdle(pApp->pVulkan->queue);
    vkDeviceWaitIdle(pApp->pVulkan->device);
}