#include "fbr_core.h"
#include "fbr_mesh.h"
#include "fbr_pipeline.h"
#include "fbr_texture.h"
#include "fbr_camera.h"
#include "fbr_log.h"
#include "fbr_vulkan.h"
#include "fbr_input.h"
#include "fbr_node.h"
#include "fbr_framebuffer.h"
#include "fbr_node_parent.h"

#include "cglm/cglm.h"

static void waitForQueueFence(FbrVulkan *pVulkan) {
    vkWaitForFences(pVulkan->device, 1, &pVulkan->queueFence, VK_TRUE, UINT64_MAX);
    vkResetFences(pVulkan->device, 1, &pVulkan->queueFence);
}

static void waitForTimeLine(const FbrVulkan *pVulkan, FbrTimelineSemaphore *pTimelineSemaphore) {
    const VkSemaphoreWaitInfo semaphoreWaitInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = NULL,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &pTimelineSemaphore->semaphore,
            .pValues = &pTimelineSemaphore->waitValue,
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

static void beginRenderPassImageless(const FbrVulkan *pVulkan, VkRenderPass renderPass, VkFramebuffer framebuffer, VkImageView imageView, VkClearValue clearColor) {
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

static void recordNodeRenderPass(const FbrVulkan *pVulkan, const FbrPipeline *pPipeline, const FbrNode *pNode, VkDescriptorSet descriptorSet) {
    vkCmdBindPipeline(pVulkan->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->graphicsPipeline);
    vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pPipeline->pipelineLayout,
                            0,
                            1,
                            &descriptorSet,
                            0,
                            NULL);

    VkBuffer vertexBuffers[] = {pNode->pVertexUBOs[0]->uniformBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(pVulkan->commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(pVulkan->commandBuffer, pNode->pIndexUBO->uniformBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdPushConstants(pVulkan->commandBuffer, pPipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), pNode->transform.matrix);

    vkCmdDrawIndexed(pVulkan->commandBuffer, FBR_NODE_INDEX_COUNT, 1, 0, 0, 0);
}

static void submitQueue(const FbrVulkan *pVulkan, FbrTimelineSemaphore *pSemaphore) {

    const uint64_t waitValue = pSemaphore->waitValue++;
    const uint64_t signalValue = pSemaphore->waitValue;
    const VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                    .semaphore = pSemaphore->semaphore,
                    .value = waitValue,
                    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    const VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                    .semaphore = pSemaphore->semaphore,
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

static void submitQueueAndPresent(FbrApp *pApp, FbrTimelineSemaphore *pSemaphore, uint32_t swapIndex) {
    // https://www.khronos.org/blog/vulkan-timeline-semaphores
    const uint64_t waitValue = pSemaphore->waitValue++;
    const uint64_t signalValue = pSemaphore->waitValue;
    const VkSemaphoreSubmitInfo waitSemaphoreSubmitInfos[2] = {
            {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                    .semaphore = pSemaphore->semaphore,
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
                    .semaphore = pSemaphore->semaphore,
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

static void beginFrameCommandBuffer(FbrVulkan *pVulkan) {
    vkResetCommandBuffer(pVulkan->commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo beginInfo = {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    vkBeginCommandBuffer(pVulkan->commandBuffer, &beginInfo);

    VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) pVulkan->swapExtent.width,
            .height = (float) pVulkan->swapExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    vkCmdSetViewport(pVulkan->commandBuffer, 0, 1, &viewport);
    VkRect2D scissor = {
            .offset = {0, 0},
            .extent.width = pVulkan->swapExtent.width,
            .extent.height = pVulkan->swapExtent.height,
    };
    vkCmdSetScissor(pVulkan->commandBuffer, 0, 1, &scissor);
}

static void childMainLoop(FbrApp *pApp) {
    double lastFrameTime = glfwGetTime();

    // does the pointer indirection here actually cause issue!?
    FbrVulkan *pVulkan = pApp->pVulkan;
    FbrFramebuffer *pFrameBuffer = pApp->pNodeParent->pFramebuffer;
    FbrTimelineSemaphore *pParentSemaphore = pApp->pNodeParent->pParentSemaphore;
//    FbrTimelineSemaphore *pChildSemaphore = pApp->pVulkan->pMainSemaphore;
    FbrTimelineSemaphore *pChildSemaphore = pApp->pNodeParent->pChildSemaphore;
    FbrCamera *pCamera = pApp->pNodeParent->pCamera;
    FbrTime *pTime = pApp->pTime;

    const uint64_t timelineStep = 4;
    vkGetSemaphoreCounterValue(pVulkan->device, pParentSemaphore->semaphore, &pParentSemaphore->waitValue);
    pParentSemaphore->waitValue += timelineStep;
    FBR_LOG_DEBUG("starting parent timeline value", pParentSemaphore->waitValue);

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {
        pTime->currentTime = glfwGetTime();
        pTime->deltaTime = pTime->currentTime - lastFrameTime;
        lastFrameTime = pTime->currentTime;

        FBR_LOG_DEBUG("Child FPS", 1.0 / pApp->pTime->deltaTime);

        // wait for child timeline sempahore to finish
        FBR_LOG_DEBUG("Waiting For Child Seamphore", pChildSemaphore->waitValue);
        waitForTimeLine(pVulkan, pChildSemaphore);

        // for some reason this fixes a bug with validation layers thinking the queue hasnt finished
        // wait on timeline should be enough!!!
        if (pVulkan->enableValidationLayers)
            vkQueueWaitIdle(pVulkan->queue);

        // check parent timeline semaphore, if parent timeline value past what it should be
        // it means child took too long to render
        uint64_t value;
        vkGetSemaphoreCounterValue(pVulkan->device, pParentSemaphore->semaphore, &value);
        if (value >= pParentSemaphore->waitValue) {
            FBR_LOG_DEBUG("Child took to long!", value, pParentSemaphore->waitValue);
            pParentSemaphore->waitValue = value;
        } else {
            waitForTimeLine(pVulkan, pParentSemaphore);
        }
        pParentSemaphore->waitValue += timelineStep;


//        uint64_t value;
//        vkGetSemaphoreCounterValue(pApp->pVulkan->device, pApp->parentTimelineSemaphore, &value);
//        FBR_LOG_DEBUG("child import semaphore", value);

        // Todo there is some trickery to de done here with multiple frames per child node framebuffer
        // and it swapping them, but this seems to be no issue on nvidia due to how nvidia implements this stuff...
        beginFrameCommandBuffer(pVulkan);

        beginRenderPassImageless(pVulkan,
                                 pFrameBuffer->renderPass,
                                 pFrameBuffer->framebuffer,
                                 pFrameBuffer->pTexture->imageView,
                                 (VkClearValue){{{0.0f, 0.0f, 0.00f, 0.0f}}});
        //cube 1
        fbrUpdateTransformMatrix(&pApp->pTestQuadMesh->transform);
        recordRenderPass(pVulkan, pApp->pSwapPipeline, pApp->pTestQuadMesh, pApp->pNodeParent->parentFramebufferDescriptorSet);
        // end framebuffer pass
        vkCmdEndRenderPass(pVulkan->commandBuffer);

        //swap pass
//            beginRenderPass(pApp->pVulkan, pApp->pVulkan->renderPass, pApp->pVulkan->pSwapChainFramebuffers[swapIndex]);
//            //cube 1
//            fbrUpdateTransformMatrix(&pApp->pTestQuadMesh->transform);
//            recordRenderPass(pApp->pVulkan, pApp->pSwapPipeline, pApp->pTestQuadMesh);
//            // end swap pass
//            vkCmdEndRenderPass(pApp->pVulkan->commandBuffer);

        vkEndCommandBuffer(pVulkan->commandBuffer);
        submitQueue(pVulkan, pChildSemaphore);
    }

}

static void parentMainLoop(FbrApp *pApp) {
    double lastFrameTime = glfwGetTime();

    FbrVulkan *pVulkan = pApp->pVulkan;
    FbrTimelineSemaphore *pSemaphore = pVulkan->pMainSemaphore;
    FbrTime *pTime = pApp->pTime;
    FbrCamera *pCamera = pApp->pCamera;

    bool firstRun = true;

//    Sleep(100);

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {

//        Sleep(33);
//        FBR_LOG_DEBUG("Loop!", pApp->pVulkan->waitValue);

        // todo switch to vulkan timing primitives
        pTime->currentTime = glfwGetTime();
        pTime->deltaTime = pApp->pTime->currentTime - lastFrameTime;
        lastFrameTime = pTime->currentTime;

//        FBR_LOG_DEBUG("Parent FPS", 1.0f / pTime->deltaTime);

        waitForTimeLine(pVulkan, pSemaphore);

        // for some reason this fixes a bug with validation layers thinking the queue hasnt finished
        // wait on timeline should be enough!!
        if (pVulkan->enableValidationLayers)
            vkQueueWaitIdle(pVulkan->queue);


//        uint64_t value;
//        vkGetSemaphoreCounterValue(pVulkan->device, pVulkan->semaphore, &value);
//        FBR_LOG_DEBUG("parent import semaphore", value % 16);


        processInputFrame(pApp);
        fbrUpdateCameraUBO(pCamera);

        // somehow need to signal semaphore after UBO updating
//        VkSemaphoreSignalInfo signalInfo;
//        signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
//        signalInfo.pNext = NULL;
//        signalInfo.semaphore = timeline;
//        signalInfo.value = 7;
//        vkSignalSemaphore(dev, &signalInfo);

        // Get open swap image
        uint32_t swapIndex;
        vkAcquireNextImageKHR(pVulkan->device,
                              pVulkan->swapChain,
                              UINT64_MAX,
                              pVulkan->swapAcquireComplete,
                              VK_NULL_HANDLE,
                              &swapIndex);

        // TODO this should be possible
//        if (firstRun) {
//            firstRun = false;

            beginFrameCommandBuffer(pVulkan);

            //swap pass
            beginRenderPassImageless(pVulkan,
                                     pVulkan->renderPass,
                                     pVulkan->swapFramebuffer,
                                     pVulkan->pSwapImageViews[swapIndex],
                                     (VkClearValue){{{0.1f, 0.2f, 0.3f, 1.0f}}});
            //cube 1
            fbrUpdateTransformMatrix(&pApp->pTestQuadMesh->transform);
            recordRenderPass(pVulkan, pApp->pSwapPipeline, pApp->pTestQuadMesh, pApp->pVulkan->swapDescriptorSet);
            //cube2


            fbrUpdateNodeMesh(pVulkan, pCamera, pApp->pTestNode);
            recordNodeRenderPass(pVulkan, pApp->pCompPipeline, pApp->pTestNode, pApp->compDescriptorSet);

            // end swap pass
            vkCmdEndRenderPass(pVulkan->commandBuffer);

            vkEndCommandBuffer(pVulkan->commandBuffer);
//        }

        submitQueueAndPresent(pApp, pSemaphore, swapIndex);
    }


    uint64_t value2;
    vkGetSemaphoreCounterValue(pVulkan->device, pSemaphore->semaphore, &value2);
    FBR_LOG_DEBUG("final parent import semaphore", value2);
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