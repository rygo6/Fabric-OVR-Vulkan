#include "fbr_core.h"
#include "fbr_mesh.h"
#include "fbr_camera.h"
#include "fbr_log.h"
#include "fbr_vulkan.h"
#include "fbr_input.h"
#include "fbr_node.h"
#include "fbr_framebuffer.h"
#include "fbr_node_parent.h"
#include "fbr_pipelines.h"
#include "fbr_descriptors.h"
#include "fbr_swap.h"

#include "cglm/cglm.h"

#include <stdlib.h>
#include <stdio.h>

static void waitForTimeLine(FbrVulkan *pVulkan, FbrTimelineSemaphore *pTimelineSemaphore) {
    const VkSemaphoreWaitInfo semaphoreWaitInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = NULL,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &pTimelineSemaphore->semaphore,
            .pValues = &pTimelineSemaphore->waitValue,
    };
    FBR_VK_CHECK_COMMAND(vkWaitSemaphores(pVulkan->device, &semaphoreWaitInfo, UINT64_MAX));
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

static void beginRenderPassImageless(const FbrVulkan *pVulkan, const FbrFramebuffer *pFramebuffer, VkRenderPass renderPass, VkClearValue clearColor) {
    const VkRenderPassAttachmentBeginInfo renderPassAttachmentBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO,
            .attachmentCount = 1,
            .pAttachments = &pFramebuffer->pTexture->imageView
    };
    const VkRenderPassBeginInfo vkRenderPassBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = &renderPassAttachmentBeginInfo,
            .renderPass = renderPass,
            .framebuffer = pFramebuffer->framebuffer,
            .renderArea.extent = pFramebuffer->pTexture->extent,
            .clearValueCount = 1,
            .pClearValues = &clearColor,
    };
    vkCmdBeginRenderPass(pVulkan->commandBuffer, &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

static void recordRenderMesh(const FbrVulkan *pVulkan, const FbrMesh *pMesh) {
    VkBuffer vertexBuffers[] = {pMesh->vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(pVulkan->commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(pVulkan->commandBuffer, pMesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(pVulkan->commandBuffer, pMesh->indexCount, 1, 0, 0, 0);
}

static void recordNodeRenderPass(const FbrVulkan *pVulkan, const FbrNode *pNode, int timelineSwitch) {
    VkBuffer vertexBuffers[] = {pNode->pVertexUBOs[timelineSwitch]->uniformBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(pVulkan->commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(pVulkan->commandBuffer, pNode->pIndexUBO->uniformBuffer, 0, VK_INDEX_TYPE_UINT16);
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
            .pNext = NULL,
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &waitSemaphoreSubmitInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalSemaphoreSubmitInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
    };
    FBR_VK_CHECK_COMMAND(vkQueueSubmit2(pVulkan->queue,
                                        1,
                                        &submitInfo,
                                        VK_NULL_HANDLE));
}

static void submitQueueAndPresent(FbrVulkan *pVulkan, const FbrSwap *pSwap, FbrTimelineSemaphore *pSemaphore, uint32_t swapIndex) {
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
                    .semaphore = pSwap->acquireComplete,
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
                    .semaphore = pSwap->renderCompleteSemaphore,
                    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
            }
    };
    const VkCommandBufferSubmitInfo commandBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
            .commandBuffer = pVulkan->commandBuffer,
    };
    const VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
            .pNext = NULL,
            .waitSemaphoreInfoCount = 2,
            .pWaitSemaphoreInfos = waitSemaphoreSubmitInfos,
            .signalSemaphoreInfoCount = 2,
            .pSignalSemaphoreInfos = signalSemaphoreSubmitInfos,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
    };
    FBR_VK_CHECK_COMMAND(vkQueueSubmit2(pVulkan->queue,
                   1,
                   &submitInfo,
                   VK_NULL_HANDLE));

    // TODO want to use this?? https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_present_id.html
    const VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &pSwap->renderCompleteSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &pSwap->swapChain,
            .pImageIndices = &swapIndex,
    };
    FBR_VK_CHECK_COMMAND(vkQueuePresentKHR(pVulkan->queue, &presentInfo));
}

static void beginFrameCommandBuffer(FbrVulkan *pVulkan, VkExtent2D extent) {
    FBR_VK_CHECK_COMMAND(vkResetCommandBuffer(pVulkan->commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
    const VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    FBR_VK_CHECK_COMMAND(vkBeginCommandBuffer(pVulkan->commandBuffer, &beginInfo));

    const VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) extent.width,
            .height = (float) extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    vkCmdSetViewport(pVulkan->commandBuffer, 0, 1, &viewport);
    const VkRect2D scissor = {
            .offset = {0, 0},
            .extent = extent,
    };
    vkCmdSetScissor(pVulkan->commandBuffer, 0, 1, &scissor);
}

static void childMainLoop(FbrApp *pApp) {
    double lastFrameTime = glfwGetTime();

    // does the pointer indirection here actually cause issue!?
    FbrVulkan *pVulkan = pApp->pVulkan;
    FbrFramebuffer *pFrameBuffers[] = {pApp->pNodeParent->pFramebuffers[0],
                                       pApp->pNodeParent->pFramebuffers[1]};
    FbrTimelineSemaphore *pParentSemaphore = pApp->pNodeParent->pParentSemaphore;
    FbrTimelineSemaphore *pChildSemaphore = pApp->pNodeParent->pChildSemaphore;
    FbrCamera *pCamera = pApp->pNodeParent->pCamera;
    FbrTime *pTime = pApp->pTime;
    FbrPipelines *pPipelines = pApp->pPipelines;
    FbrDescriptors *pDescriptors = pApp->pDescriptors;

//    bool firstRun = true;

    const uint64_t parentTimelineStep = 16;
    vkGetSemaphoreCounterValue(pVulkan->device, pParentSemaphore->semaphore, &pParentSemaphore->waitValue);
    pParentSemaphore->waitValue += parentTimelineStep;
    FBR_LOG_DEBUG("starting parent timeline value", pParentSemaphore->waitValue);

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {
        pTime->currentTime = glfwGetTime();
        pTime->deltaTime = pTime->currentTime - lastFrameTime;
        lastFrameTime = pTime->currentTime;

//        FBR_LOG_DEBUG("Child FPS", 1.0 / pApp->pTime->deltaTime);

        // wait on both parent and child
        const VkSemaphoreWaitInfo semaphoreWaitInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
                .pNext = NULL,
                .flags = 0,
                .semaphoreCount = 2,
                .pSemaphores = (const VkSemaphore[]) {pParentSemaphore->semaphore,
                                                      pChildSemaphore->semaphore},
                .pValues = (const uint64_t[]) {pParentSemaphore->waitValue,
                                               pChildSemaphore->waitValue}
        };
        FBR_VK_CHECK_COMMAND(vkWaitSemaphores(pVulkan->device, &semaphoreWaitInfo, UINT64_MAX));

        // Update to current parent time, don't let it go faster that parent allows.
        // TODO can be be predictive to start just in time for parent frame flip?
        vkGetSemaphoreCounterValue(pVulkan->device, pParentSemaphore->semaphore, &pParentSemaphore->waitValue);
        pParentSemaphore->waitValue += parentTimelineStep;
        int timelineSwitch = (int) (pChildSemaphore->waitValue % 2);

        // for some reason this fixes a bug with validation layers thinking the queue hasnt finished wait on timeline should be enough!!!
        if (pVulkan->enableValidationLayers)
            FBR_VK_CHECK_COMMAND(vkQueueWaitIdle(pVulkan->queue));

        // not exactly order for this, but we need cam UBO data for each frame step
        fbrUpdateNodeParentMesh(pVulkan, pCamera, timelineSwitch, pApp->pNodeParent);

//        if (firstRun)
        {
//            firstRun = false;

            // Todo there is some trickery to de done here with multiple frames per child node framebuffer
            // and it swapping them, but this seems to be no issue on nvidia due to how nvidia implements this stuff...
            beginFrameCommandBuffer(pVulkan, pFrameBuffers[timelineSwitch]->pTexture->extent);

            beginRenderPassImageless(pVulkan,
                                     pFrameBuffers[timelineSwitch],
                                     pVulkan->renderPass,
                                     (VkClearValue) {{{0.0f, 0.0f, 0.00f, 0.0f}}});


            vkCmdBindPipeline(pVulkan->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelines->pipeStandard);
            // Global
            vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pPipelines->pipeLayoutStandard,
                                    FBR_GLOBAL_SET_INDEX,
                                    1,
                                    &pDescriptors->setGlobal,
                                    0,
                                    NULL);

            // Material
            vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pPipelines->pipeLayoutStandard,
                                    FBR_MATERIAL_SET_INDEX,
                                    1,
                                    &pApp->testQuadMaterialSet,
                                    0,
                                    NULL);

            //cube 1
            fbrUpdateTransformMatrix(pApp->pTestQuadTransform);
            vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pPipelines->pipeLayoutStandard,
                                    FBR_OBJECT_SET_INDEX,
                                    1,
                                    &pApp->testQuadObjectSet,
                                    0,
                                    NULL);
            recordRenderMesh(pVulkan,
                             pApp->pTestQuadMesh);


            // end framebuffer pass
            vkCmdEndRenderPass(pVulkan->commandBuffer);

            FBR_VK_CHECK_COMMAND(vkEndCommandBuffer(pVulkan->commandBuffer));

        }

        submitQueue(pVulkan, pChildSemaphore);

        // todo for light cleanup exit if time exceeds
//        _exit(0);
    }
}

static void setHighPriority(){
    // ovr example does this, is it good? https://github.com/ValveSoftware/virtual_display/blob/da13899ea6b4c0e4167ed97c77c6d433718489b1/virtual_display/virtual_display.cpp
#define THREAD_PRIORITY_MOST_URGENT 15
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_MOST_URGENT );
    SetPriorityClass(GetCurrentThread(), REALTIME_PRIORITY_CLASS);
}

static void parentMainLoop(FbrApp *pApp) {
    setHighPriority();

    double lastFrameTime = glfwGetTime();

    FbrVulkan *pVulkan = pApp->pVulkan;
    FbrSwap *pSwap = pApp->pSwap;
    FbrTimelineSemaphore *pMainSemaphore = pVulkan->pMainSemaphore;
    FbrTime *pTime = pApp->pTime;
    FbrCamera *pCamera = pApp->pCamera;
    FbrPipelines *pPipelines = pApp->pPipelines;
    FbrDescriptors *pDescriptors = pApp->pDescriptors;
    FbrNode *pTestNode = pApp->pTestNode;

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {
        // todo switch to vulkan timing primitives
        pTime->currentTime = glfwGetTime();
        pTime->deltaTime = pApp->pTime->currentTime - lastFrameTime;
        lastFrameTime = pTime->currentTime;

//        FBR_LOG_DEBUG("Parent FPS", 1.0f / pTime->deltaTime);

        waitForTimeLine(pVulkan, pMainSemaphore);

        // for some reason this fixes a bug with validation layers thinking the queue hasnt finished
        // wait on timeline should be enough!!
        if (pVulkan->enableValidationLayers)
            FBR_VK_CHECK_COMMAND(vkQueueWaitIdle(pVulkan->queue));

        processInputFrame(pApp);
        fbrUpdateCameraUBO(pCamera);

        // Get open swap image
        uint32_t swapIndex;
        FBR_VK_CHECK_COMMAND(vkAcquireNextImageKHR(pVulkan->device,
                                                   pSwap->swapChain,
                              UINT64_MAX,
                                                   pSwap->acquireComplete,
                              VK_NULL_HANDLE,
                              &swapIndex));

        beginFrameCommandBuffer(pVulkan, pSwap->extent);

        //swap pass
        beginRenderPassImageless(pVulkan,
                                 pSwap->pFramebuffers[swapIndex],
                                 pVulkan->renderPass,
                                 (VkClearValue){{{0.1f, 0.2f, 0.3f, 1.0f}}});

        vkCmdBindPipeline(pVulkan->commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pPipelines->pipeStandard);
        // Global
        vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->pipeLayoutStandard,
                                FBR_GLOBAL_SET_INDEX,
                                1,
                                &pDescriptors->setGlobal,
                                0,
                                NULL);

        // Material
        vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->pipeLayoutStandard,
                                FBR_MATERIAL_SET_INDEX,
                                1,
                                &pApp->testQuadMaterialSet,
                                0,
                                NULL);

        //cube 1
        fbrUpdateTransformMatrix(pApp->pTestQuadTransform);
        vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->pipeLayoutStandard,
                                FBR_OBJECT_SET_INDEX,
                                1,
                                &pApp->testQuadObjectSet,
                                0,
                                NULL);
        recordRenderMesh(pVulkan,
                         pApp->pTestQuadMesh);


        //test node
        vkGetSemaphoreCounterValue(pVulkan->device, pTestNode->pChildSemaphore->semaphore, &pTestNode->pChildSemaphore->waitValue);
        int timelineSwitch = (int)(pTestNode->pChildSemaphore->waitValue % 2);
        // Material
//        vkCmdBindDescriptorSets(pVulkan->commandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                pPipelines->pipeLayoutNode,
//                                FBR_MATERIAL_SET_INDEX,
//                                1,
//                                &pApp->pCompMaterialSets[timelineSwitch],
//                                0,
//                                NULL);
        fbrUpdateTransformMatrix(pTestNode->pTransform);
//        vkCmdBindDescriptorSets(pVulkan->commandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                pPipelines->pipeLayoutStandard,
//                                FBR_OBJECT_SET_INDEX,
//                                1,
//                                &pApp->compObjectSet,
//                                0,
//                                NULL);
        vkCmdBindPipeline(pVulkan->commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pPipelines->pipeNode);
        vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->pipeLayoutNode,
                                FBR_GLOBAL_SET_INDEX,
                                1,
                                &pDescriptors->setGlobal,
                                0,
                                NULL);
        vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->pipeLayoutNode,
                                FBR_NODE_SET_INDEX,
                                1,
                                &pApp->pCompMaterialSets[timelineSwitch],
                                0,
                                NULL);
        recordNodeRenderPass(pVulkan,
                             pTestNode,
                             timelineSwitch);

        // end swap pass
        vkCmdEndRenderPass(pVulkan->commandBuffer);

        FBR_VK_CHECK_COMMAND(vkEndCommandBuffer(pVulkan->commandBuffer));

        submitQueueAndPresent(pVulkan, pSwap, pMainSemaphore, swapIndex);
    }


    uint64_t value2;
    vkGetSemaphoreCounterValue(pVulkan->device, pMainSemaphore->semaphore, &value2);
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