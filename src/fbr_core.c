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
    FBR_ACK_EXIT(vkWaitSemaphores(pVulkan->device, &semaphoreWaitInfo, UINT64_MAX));
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

static void beginRenderPassImageless(const FbrVulkan *pVulkan, const FbrFramebuffer *pFramebuffer, VkRenderPass renderPass, VkClearColorValue clearColorValue)
{
    VkClearValue pClearValues[3] = { };
    pClearValues[0].color = clearColorValue;
    pClearValues[1].color = clearColorValue;
    pClearValues[2].depthStencil = (VkClearDepthStencilValue) {1.0f, 0 };

    const VkImageView pAttachments[] = {
            pFramebuffer->pColorTexture->imageView,
            pFramebuffer->pNormalTexture->imageView,
            pFramebuffer->pDepthTexture->imageView
    };
    const VkRenderPassAttachmentBeginInfo renderPassAttachmentBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO,
            .attachmentCount = 3,
            .pAttachments = pAttachments
    };
    const VkRenderPassBeginInfo vkRenderPassBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = &renderPassAttachmentBeginInfo,
            .renderPass = renderPass,
            .framebuffer = pFramebuffer->framebuffer,
            .renderArea.extent = pFramebuffer->pColorTexture->extent,
            .clearValueCount = 3,
            .pClearValues = pClearValues,
    };
    vkCmdBeginRenderPass(pVulkan->graphicsCommandBuffer, &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

static void recordRenderMesh(const FbrVulkan *pVulkan, const FbrMesh *pMesh) {
    VkBuffer vertexBuffers[] = {pMesh->vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(pVulkan->graphicsCommandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(pVulkan->graphicsCommandBuffer, pMesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(pVulkan->graphicsCommandBuffer, pMesh->indexCount, 1, 0, 0, 0);
}

static void recordNodeRenderPass(const FbrVulkan *pVulkan, const FbrNode *pNode, int timelineSwitch) {
    VkBuffer vertexBuffers[] = {pNode->pVertexUBOs[timelineSwitch]->uniformBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(pVulkan->graphicsCommandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(pVulkan->graphicsCommandBuffer, pNode->pIndexUBO->uniformBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(pVulkan->graphicsCommandBuffer, FBR_NODE_INDEX_COUNT, 1, 0, 0, 0);
}

static void submitQueue(const FbrVulkan *pVulkan, FbrTimelineSemaphore *pSemaphore) {
    const uint64_t waitValue = pSemaphore->waitValue;
    pSemaphore->waitValue++;
    const uint64_t signalValue = pSemaphore->waitValue;
    const VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreValueCount = 1,
        .pWaitSemaphoreValues = &waitValue,
        .signalSemaphoreValueCount = 1,
        .pSignalSemaphoreValues = &signalValue,
    };
    const VkPipelineStageFlags pWaitDstStageMask[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    const VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = &timelineSemaphoreSubmitInfo,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &pSemaphore->semaphore,
            .pWaitDstStageMask = pWaitDstStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &pVulkan->graphicsCommandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &pSemaphore->semaphore
    };
    FBR_ACK_EXIT(vkQueueSubmit(pVulkan->graphicsQueue,
                                1,
                                &submitInfo,
                                VK_NULL_HANDLE));
}

static void submitQueueAndPresent(FbrVulkan *pVulkan, const FbrSwap *pSwap, FbrTimelineSemaphore *pSemaphore, uint32_t swapIndex) {
    // https://www.khronos.org/blog/vulkan-timeline-semaphores
    const uint64_t waitValue = pSemaphore->waitValue;
    pSemaphore->waitValue++;
    const uint64_t signalValue = pSemaphore->waitValue;
    const uint64_t pWaitSemaphoreValues[] = {
            waitValue,
            0
    };
    const uint64_t pSignalSemaphoreValues[] = {
            signalValue,
            0
    };
    const VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo = {
            .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
            .pNext = NULL,
            .waitSemaphoreValueCount = 2,
            .pWaitSemaphoreValues =  pWaitSemaphoreValues,
            .signalSemaphoreValueCount = 2,
            .pSignalSemaphoreValues = pSignalSemaphoreValues,
    };
    const VkSemaphore pWaitSemaphores[] = {
            pSemaphore->semaphore,
            pSwap->acquireCompleteSemaphore
    };
    const VkPipelineStageFlags pWaitDstStageMask[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    const VkSemaphore pSignalSemaphores[] = {
            pSemaphore->semaphore,
            pSwap->renderCompleteSemaphore
    };
    const VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = &timelineSemaphoreSubmitInfo,
            .waitSemaphoreCount = 2,
            .pWaitSemaphores = pWaitSemaphores,
            .pWaitDstStageMask = pWaitDstStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &pVulkan->graphicsCommandBuffer,
            .signalSemaphoreCount = 2,
            .pSignalSemaphores = pSignalSemaphores
    };
    FBR_ACK_EXIT(vkQueueSubmit(pVulkan->graphicsQueue,
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
    FBR_ACK_EXIT(vkQueuePresentKHR(pVulkan->computeQueue, &presentInfo));
}

static void beginFrameCommandBuffer(FbrVulkan *pVulkan, VkExtent2D extent) {
    FBR_ACK_EXIT(vkResetCommandBuffer(pVulkan->graphicsCommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
    const VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    FBR_ACK_EXIT(vkBeginCommandBuffer(pVulkan->graphicsCommandBuffer, &beginInfo));

    const VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) extent.width,
            .height = (float) extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    vkCmdSetViewport(pVulkan->graphicsCommandBuffer, 0, 1, &viewport);
    const VkRect2D scissor = {
            .offset = {0, 0},
            .extent = extent,
    };
    vkCmdSetScissor(pVulkan->graphicsCommandBuffer, 0, 1, &scissor);
}

static void childMainLoop(FbrApp *pApp) {
    double lastFrameTime = glfwGetTime();

    // does the pointer indirection here actually cause issue!?
    FbrVulkan *pVulkan = pApp->pVulkan;
    FbrTimelineSemaphore *pParentSemaphore = pApp->pNodeParent->pParentSemaphore;
    FbrTimelineSemaphore *pChildSemaphore = pApp->pNodeParent->pChildSemaphore;
    FbrCamera *pCamera = pApp->pNodeParent->pCamera;
    FbrTime *pTime = pApp->pTime;
    FbrPipelines *pPipelines = pApp->pPipelines;
    FbrDescriptors *pDescriptors = pApp->pDescriptors;

    const uint64_t parentTimelineStep = 48;
    vkGetSemaphoreCounterValue(pVulkan->device, pParentSemaphore->semaphore, &pParentSemaphore->waitValue);
    pParentSemaphore->waitValue += parentTimelineStep;
    FBR_LOG_DEBUG("starting parent timeline value", pParentSemaphore->waitValue);

    uint8_t timelineSwitch = 0;

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {
//        FBR_LOG_DEBUG("Child FPS", 1.0 / pApp->pTime->deltaTime);

        // Update Time
        pTime->currentTime = glfwGetTime();
        pTime->deltaTime = pTime->currentTime - lastFrameTime;
        lastFrameTime = pTime->currentTime;

        // Update to current parent time, don't let it go faster than parent allows.
        vkGetSemaphoreCounterValue(pVulkan->device, pParentSemaphore->semaphore, &pParentSemaphore->waitValue);
//        uint8_t dynamicCameraIndex = (uint8_t) pParentSemaphore->waitValue % FBR_DYNAMIC_MAIN_CAMERA_COUNT;
        uint8_t dynamicCameraIndex = 1;
        uint32_t dynamicGlobalOffset = dynamicCameraIndex * pCamera->pUBO->dynamicAlignment;
        memcpy(pCamera->pUBO->pUniformBufferMapped + dynamicGlobalOffset, pCamera->pUBO->pUniformBufferMapped, sizeof(FbrCameraUBO));

        beginFrameCommandBuffer(pVulkan, pApp->pFramebuffers[timelineSwitch]->pColorTexture->extent);

        // Acquire Framebuffer Ownership
        const VkImageMemoryBarrier pAcquireColorImageMemoryBarriers[] = {
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = 0,
                        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                        .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .image = pApp->pFramebuffers[timelineSwitch]->pColorTexture->image,
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
                        .image = pApp->pFramebuffers[timelineSwitch]->pNormalTexture->image,
                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                }
        };
        vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0,
                             0, NULL,
                             0, NULL,
                             2, pAcquireColorImageMemoryBarriers);
        const VkImageMemoryBarrier pAcquireDepthImageMemoryBarriers[] = {
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = 0,
                        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                        .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .image = pApp->pFramebuffers[timelineSwitch]->pDepthTexture->image,
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



        beginRenderPassImageless(pVulkan,
                                 pApp->pFramebuffers[timelineSwitch],
                                 pVulkan->renderPass,
                                 (VkClearColorValue) {{0.2f, 0.2f, 0.2f, 0.0f}});
        vkCmdBindPipeline(pVulkan->graphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelines->pipeStandard);

        // Global
        uint32_t zeroOffset = 0;
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->pipeLayoutStandard,
                                FBR_GLOBAL_SET_INDEX,
                                1,
                                &pDescriptors->setGlobal,
                                1,
                                &zeroOffset);
//        // Pass
//        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                pPipelines->pipeLayoutStandard,
//                                FBR_PASS_SET_INDEX,
//                                1,
//                                &pDescriptors->setPass,
//                                0,
//                                NULL);
        // Material
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->pipeLayoutStandard,
                                FBR_MATERIAL_SET_INDEX,
                                1,
                                &pApp->testQuadMaterialSet,
                                0,
                                NULL);
        //cube 1
        fbrUpdateTransformMatrix(pApp->pTestQuadTransform);
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
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
        vkCmdEndRenderPass(pVulkan->graphicsCommandBuffer);

        fbrUpdateNodeParentMesh(pVulkan, pCamera, dynamicCameraIndex, timelineSwitch, pApp->pNodeParent);


        // Release Framebuffer Ownership
        const VkImageMemoryBarrier pReleaseColorImageMemoryBarriers[] = {
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        .dstAccessMask = 0,
                        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                        .image = pApp->pFramebuffers[timelineSwitch]->pColorTexture->image,
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
                        .image = pApp->pFramebuffers[timelineSwitch]->pNormalTexture->image,
                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                }
        };
        vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0,
                             0, NULL,
                             0, NULL,
                             2, pReleaseColorImageMemoryBarriers);
        const VkImageMemoryBarrier pReleaseDepthImageMemoryBarriers[] = {
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        .dstAccessMask = 0,
                        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                        .image = pApp->pFramebuffers[timelineSwitch]->pDepthTexture->image,
                        FBR_DEFAULT_DEPTH_SUBRESOURCE_RANGE
                }
        };
        vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0,
                             0, NULL,
                             0, NULL,
                             1, pReleaseDepthImageMemoryBarriers);


        FBR_ACK_EXIT(vkEndCommandBuffer(pVulkan->graphicsCommandBuffer));

//        FBR_LOG_DEBUG("Rendering... ", timelineSwitch, pChildSemaphore->waitValue);
        submitQueue(pVulkan, pChildSemaphore);

        // Add step to parent and wait on both child and parent
        pParentSemaphore->waitValue += parentTimelineStep;
        const VkSemaphoreWaitInfo releaseSemaphoreWaitInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
                .semaphoreCount = 2,
                .pSemaphores = (const VkSemaphore[]) {pParentSemaphore->semaphore,
                                                      pChildSemaphore->semaphore},
                .pValues = (const uint64_t[]) {pParentSemaphore->waitValue,
                                               pChildSemaphore->waitValue}
        };
        FBR_ACK_EXIT(vkWaitSemaphores(pVulkan->device, &releaseSemaphoreWaitInfo, UINT64_MAX));
        // for some reason this fixes a bug with validation layers thinking the graphicsQueue hasn't finished wait on timeline should be enough!!!
        if (pVulkan->enableValidationLayers) {
            FBR_ACK_EXIT(vkQueueWaitIdle(pVulkan->graphicsQueue));
        }

        timelineSwitch = (timelineSwitch + 1) % 2;

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
    FbrTimelineSemaphore *pMainTimelineSemaphore = pVulkan->pMainTimelineSemaphore;
    FbrTime *pTime = pApp->pTime;
    FbrCamera *pCamera = pApp->pCamera;
    FbrPipelines *pPipelines = pApp->pPipelines;
    FbrDescriptors *pDescriptors = pApp->pDescriptors;
    FbrNode *pTestNode = pApp->pTestNode;

    uint64_t priorChildTimeline = 0;
    uint8_t timelineSwitch = 1;
    uint8_t framebufferIndex = 0;

    VkExtent2D extents = pSwap->extent;

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {
//        FBR_LOG_DEBUG("Parent FPS", 1.0f / pTime->deltaTime);

        // Timing !
        // todo switch to vulkan timing primitives
        pTime->currentTime = glfwGetTime();
        pTime->deltaTime = pApp->pTime->currentTime - lastFrameTime;
        lastFrameTime = pTime->currentTime;

        // Input !
        processInputFrame(pApp);

        // Begin Graphics Commands
        FBR_ACK_EXIT(vkResetCommandBuffer(pVulkan->graphicsCommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
        const VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        FBR_ACK_EXIT(vkBeginCommandBuffer(pVulkan->graphicsCommandBuffer, &beginInfo));

        // Acquire Parent Framebuffers
        const VkImageMemoryBarrier pAcquireFrameBufferBarrier[] = {
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = 0,
                        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .image = pApp->pFramebuffers[framebufferIndex]->pColorTexture->image,
                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                },
        };
        vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ,
                             0,
                             0, NULL,
                             0, NULL,
                             1, pAcquireFrameBufferBarrier);


        //TODO is reading the semaphore slower than just sharing CPU memory?
        vkGetSemaphoreCounterValue(pVulkan->device,
                                   pTestNode->pChildSemaphore->semaphore,
                                   &pTestNode->pChildSemaphore->waitValue);
        if (priorChildTimeline != pTestNode->pChildSemaphore->waitValue) {
            priorChildTimeline = pTestNode->pChildSemaphore->waitValue;
            timelineSwitch = (timelineSwitch + 1) % 2;

            // Acquire Child Framebuffer Ownership
            const VkImageMemoryBarrier pAcquireChildColorFrameBufferBarrier[] = {
                    {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                            .srcAccessMask = 0,
                            .dstAccessMask = 0,
                            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                            .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                            .image = pTestNode->pFramebuffers[timelineSwitch]->pColorTexture->image,
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
                            .image = pTestNode->pFramebuffers[timelineSwitch]->pNormalTexture->image,
                            FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                    },
            };
            vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0, NULL,
                                 0, NULL,
                                 2, pAcquireChildColorFrameBufferBarrier);
            const VkImageMemoryBarrier pAcquireChildDepthFrameBufferBarrier[] = {
                    {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                            .srcAccessMask = 0,
                            .dstAccessMask = 0,
                            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
                            .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                            .image = pTestNode->pFramebuffers[timelineSwitch]->pDepthTexture->image,
                            FBR_DEFAULT_DEPTH_SUBRESOURCE_RANGE
                    }
            };
            vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0, NULL,
                                 0, NULL,
                                 1, pAcquireChildDepthFrameBufferBarrier);
//            FBR_LOG_DEBUG("parent switch", timelineSwitch, pTestNode->pChildSemaphore->waitValue);
        }

        // Being Parent Render Pass
        const VkViewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = (float) extents.width,
                .height = (float) extents.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
        };
        vkCmdSetViewport(pVulkan->graphicsCommandBuffer, 0, 1, &viewport);
        const VkRect2D scissor = {
                .offset = {0, 0},
                .extent = extents,
        };
        vkCmdSetScissor(pVulkan->graphicsCommandBuffer, 0, 1, &scissor);
        beginRenderPassImageless(pVulkan,
                                 pApp->pFramebuffers[framebufferIndex],
                                 pVulkan->renderPass,
                                 (VkClearColorValue ){{0.1f, 0.2f, 0.3f, 0.0f}});

        // Begin Render Commands
        vkCmdBindPipeline(pVulkan->graphicsCommandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pPipelines->pipeStandard);
        // Global
//        uint8_t dynamicCameraIndex = (int) (pMainTimelineSemaphore->waitValue % FBR_DYNAMIC_MAIN_CAMERA_COUNT);
        uint8_t dynamicCameraIndex = 0;
        fbrUpdateCameraUBO(pCamera, dynamicCameraIndex);
        uint32_t dynamicGlobalOffset = dynamicCameraIndex * pCamera->pUBO->dynamicAlignment;
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->pipeLayoutStandard,
                                FBR_GLOBAL_SET_INDEX,
                                1,
                                &pDescriptors->setGlobal,
                                1,
                                &dynamicGlobalOffset);
//        // Pass
//        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                pPipelines->pipeLayoutStandard,
//                                FBR_PASS_SET_INDEX,
//                                1,
//                                &pDescriptors->setPass,
//                                0,
//                                NULL);
        // Material
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->pipeLayoutStandard,
                                FBR_MATERIAL_SET_INDEX,
                                1,
                                &pApp->testQuadMaterialSet,
                                0,
                                NULL);

        //cube 1
        fbrUpdateTransformMatrix(pApp->pTestQuadTransform);
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->pipeLayoutStandard,
                                FBR_OBJECT_SET_INDEX,
                                1,
                                &pApp->testQuadObjectSet,
                                0,
                                NULL);
        recordRenderMesh(pVulkan,
                         pApp->pTestQuadMesh);


        if (pTestNode != NULL) {
            // Material
            fbrUpdateTransformMatrix(pTestNode->pTransform);
            vkCmdBindPipeline(pVulkan->graphicsCommandBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pPipelines->pipeNode);
            vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pPipelines->pipeLayoutNode,
                                    FBR_GLOBAL_SET_INDEX,
                                    1,
                                    &pDescriptors->setGlobal,
                                    1,
                                    &dynamicGlobalOffset);
            uint32_t childDynamicGlobalOffset = 1 * pCamera->pUBO->dynamicAlignment;
            vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pPipelines->pipeLayoutNode,
                                    FBR_NODE_SET_INDEX,
                                    1,
                                    &pApp->pCompMaterialSets[timelineSwitch],
                                    1,
                                    &childDynamicGlobalOffset);
            recordNodeRenderPass(pVulkan,
                                 pTestNode,
                                 timelineSwitch);
//            uint64_t childWaitValue = pTestNode->pChildSemaphore->waitValue;
//            FBR_LOG_DEBUG("Displaying: ", timelineSwitch, childWaitValue);
        }

        vkCmdEndRenderPass(pVulkan->graphicsCommandBuffer);
        // End of Graphics Commands

        // Release Framebuffer
        const VkImageMemoryBarrier pReleaseFrameBufferBarrier[] = {
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        .dstAccessMask = 0,
                        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                        .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .dstQueueFamilyIndex = pVulkan->computeQueueFamilyIndex,
                        .image = pApp->pFramebuffers[framebufferIndex]->pColorTexture->image,
                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                },
        };
        vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ,
                             0,
                             0, NULL,
                             0, NULL,
                             1, pReleaseFrameBufferBarrier);

        FBR_ACK_EXIT(vkEndCommandBuffer(pVulkan->graphicsCommandBuffer));
        // End Command Buffer



        // Submit Graphics
        const uint64_t waitValue = pMainTimelineSemaphore->waitValue;
        const uint64_t pRenderWaitSemaphoreValues[] = {
                waitValue,
        };
        const VkSemaphore pRenderWaitSemaphores[] = {
                pMainTimelineSemaphore->semaphore,
        };
        const VkSemaphore pRenderSignalSemaphores[] = {
                pApp->pFramebuffers[framebufferIndex]->renderCompleteSemaphore
        };
        const VkPipelineStageFlags pRenderWaitDstStageMask[] = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
        const VkTimelineSemaphoreSubmitInfo renderTimelineSemaphoreSubmitInfo = {
                .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
                .pNext = NULL,
                .waitSemaphoreValueCount = 1,
                .pWaitSemaphoreValues =  pRenderWaitSemaphoreValues,
                .signalSemaphoreValueCount = 0,
                .pSignalSemaphoreValues = NULL,
        };
        const VkSubmitInfo renderSubmitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = &renderTimelineSemaphoreSubmitInfo,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = pRenderWaitSemaphores,
                .pWaitDstStageMask = pRenderWaitDstStageMask,
                .commandBufferCount = 1,
                .pCommandBuffers = &pVulkan->graphicsCommandBuffer,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = pRenderSignalSemaphores
        };
        FBR_ACK_EXIT(vkQueueSubmit(pVulkan->graphicsQueue,
                                   1,
                                   &renderSubmitInfo,
                                   VK_NULL_HANDLE));
        // End Submit Graphics



        // Acquire Compute Swap
        uint32_t swapIndex;
        FBR_ACK_EXIT(vkAcquireNextImageKHR(pVulkan->device,
                                           pSwap->swapChain,
                                           UINT64_MAX,
                                           pSwap->acquireCompleteSemaphore,
                                           VK_NULL_HANDLE,
                                           &swapIndex));

        // Begin Compute Command Buffer
        FBR_ACK_EXIT(vkResetCommandBuffer(pVulkan->computeCommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
        const VkCommandBufferBeginInfo computeBeginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        FBR_ACK_EXIT(vkBeginCommandBuffer(pVulkan->computeCommandBuffer, &computeBeginInfo));


        const VkImageMemoryBarrier pTransitionBlitBarrier[] = {
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = 0,
                        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                        .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .dstQueueFamilyIndex = pVulkan->computeQueueFamilyIndex,
                        .image = pApp->pFramebuffers[framebufferIndex]->pColorTexture->image,
                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                },
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = 0,
                        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                        .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .image = pSwap->pSwapImages[swapIndex],
                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                },
        };
        vkCmdPipelineBarrier(pVulkan->computeCommandBuffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ,
                             0,
                             0, NULL,
                             0, NULL,
                             2, pTransitionBlitBarrier);



        FbrSetComposite setComposite;
        fbrCreateSetComposite(pApp->pVulkan,
                              pApp->pDescriptors->setLayoutComposite,
                              pApp->pFramebuffers[framebufferIndex]->pColorTexture->imageView,
                              pSwap->pSwapImageViews[swapIndex],
                              &setComposite);
        vkCmdBindPipeline(pVulkan->computeCommandBuffer,
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          pApp->pPipelines->computePipeComposite);
        vkCmdBindDescriptorSets(pVulkan->computeCommandBuffer,
                                VK_PIPELINE_BIND_POINT_COMPUTE,
                                pApp->pPipelines->computePipeLayoutComposite,
                                0,
                                1,
                                &setComposite,
                                0,
                                0);

        vkResetQueryPool(pVulkan->device, pVulkan->queryPool, 0, 2);

        vkCmdWriteTimestamp(pVulkan->computeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, pVulkan->queryPool, 0);
        const int localSize = 32;
        vkCmdDispatch(pVulkan->computeCommandBuffer, extents.width / localSize, extents.height / localSize, 1);
        vkCmdWriteTimestamp(pVulkan->computeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, pVulkan->queryPool, 1);


        const VkImageMemoryBarrier pTransitionEndBlitBarrier[] = {
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = pSwap->pSwapImages[swapIndex],
                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                },
        };
        vkCmdPipelineBarrier(pVulkan->computeCommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ,
                             0,
                             0, NULL,
                             0, NULL,
                             1, pTransitionEndBlitBarrier);

        FBR_ACK_EXIT(vkEndCommandBuffer(pVulkan->computeCommandBuffer));
        // End Compute Command Buffer

        
        // Submit Compute
        const VkSemaphore pComputeWaitSemaphores[] = {
                pApp->pFramebuffers[framebufferIndex]->renderCompleteSemaphore,
                pSwap->acquireCompleteSemaphore,
        };
        const VkPipelineStageFlags pComputeWaitDstStageMask[] = {
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
        };
        pMainTimelineSemaphore->waitValue++;
        const uint64_t signalValue = pMainTimelineSemaphore->waitValue;
        const uint64_t pComputeSignalSemaphoreValues[] = {
                signalValue,
                0,
        };
        const VkSemaphore pSignalSemaphores[] = {
                pMainTimelineSemaphore->semaphore,
                pSwap->renderCompleteSemaphore
        };
        const VkTimelineSemaphoreSubmitInfo computeTimelineSemaphoreSubmitInfo = {
                .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
                .pNext = NULL,
                .waitSemaphoreValueCount = 0,
                .pWaitSemaphoreValues =  NULL,
                .signalSemaphoreValueCount = 2,
                .pSignalSemaphoreValues = pComputeSignalSemaphoreValues,
        };
        const VkSubmitInfo computeSubmitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = &computeTimelineSemaphoreSubmitInfo,
                .waitSemaphoreCount = 2,
                .pWaitSemaphores = pComputeWaitSemaphores,
                .pWaitDstStageMask = pComputeWaitDstStageMask,
                .commandBufferCount = 1,
                .pCommandBuffers = &pVulkan->computeCommandBuffer,
                .signalSemaphoreCount = 2,
                .pSignalSemaphores = pSignalSemaphores
        };
        FBR_ACK_EXIT(vkQueueSubmit(pVulkan->computeQueue,
                                   1,
                                   &computeSubmitInfo,
                                   VK_NULL_HANDLE));
        // End Submit Compute


        // Submit Present
        // TODO want to use id+wait ? Doesn't work on Quest 2.
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_present_id.html
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_present_wait.html
        const VkPresentInfoKHR presentInfo = {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &pSwap->renderCompleteSemaphore,
                .swapchainCount = 1,
                .pSwapchains = &pSwap->swapChain,
                .pImageIndices = &swapIndex,
        };
        FBR_ACK_EXIT(vkQueuePresentKHR(pVulkan->computeQueue, &presentInfo));
        // End Submit Present


        // Wait!
        const VkSemaphoreWaitInfo semaphoreWaitInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
                .pNext = NULL,
                .flags = 0,
                .semaphoreCount = 1,
                .pSemaphores = &pMainTimelineSemaphore->semaphore,
                .pValues = &pMainTimelineSemaphore->waitValue,
        };
        FBR_ACK_EXIT(vkWaitSemaphores(pVulkan->device, &semaphoreWaitInfo, UINT64_MAX));

        // for some reason this fixes a bug with validation layers thinking the graphicsQueue hasnt finished
        // wait on timeline should be enough!!
        if (pVulkan->enableValidationLayers) {
            FBR_ACK_EXIT(vkQueueWaitIdle(pVulkan->graphicsQueue));
            FBR_ACK_EXIT(vkQueueWaitIdle(pVulkan->computeQueue));
        }

        uint64_t timestamps[2];
        vkGetQueryPoolResults(pVulkan->device, pVulkan->queryPool, 0, 2, sizeof(uint64_t) * 2, timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );
        float ms = (float)(timestamps[1] - timestamps[0]) / 1000000.0f;
        FBR_LOG_DEBUG("Compute: ", ms);


        vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &setComposite);

        framebufferIndex = !framebufferIndex;
    }
}

void fbrMainLoop(FbrApp *pApp) {
    FBR_LOG_DEBUG("mainloop starting!");

    if (!pApp->isChild) {
        parentMainLoop(pApp);
    } else {
        childMainLoop(pApp);
    }

    vkQueueWaitIdle(pApp->pVulkan->graphicsQueue);
    vkDeviceWaitIdle(pApp->pVulkan->device);
}