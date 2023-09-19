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
#include "fbr_ipc.h"
#include "fbr_cglm.h"

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
    VkClearValue pClearValues[4] = { };
    pClearValues[0].color = clearColorValue;
    pClearValues[1].color = (VkClearColorValue ){{0.0f, 0.0f, 0.0f, 0.0f}};
    pClearValues[2].color = (VkClearColorValue ){{0.0f, 0.0f, 0.0f, 0.0f}};
    pClearValues[3].depthStencil = (VkClearDepthStencilValue) {1.0f, 0 };

    const VkImageView pAttachments[] = {
            pFramebuffer->pColorTexture->imageView,
            pFramebuffer->pNormalTexture->imageView,
            pFramebuffer->pGBufferTexture->imageView,
            pFramebuffer->pDepthTexture->imageView
    };
    const VkRenderPassAttachmentBeginInfo renderPassAttachmentBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO,
            .attachmentCount = COUNT(pAttachments),
            .pAttachments = pAttachments
    };
    const VkRenderPassBeginInfo vkRenderPassBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = &renderPassAttachmentBeginInfo,
            .renderPass = renderPass,
            .framebuffer = pFramebuffer->framebuffer,
            .renderArea.extent = pFramebuffer->pColorTexture->extent,
            .clearValueCount = COUNT(pClearValues),
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
    FBR_ACK_EXIT(vkQueuePresentKHR(pVulkan->graphicsQueue, &presentInfo));
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

static void updateTime(FbrTime *pTime)
{
    pTime->currentTime = glfwGetTime();
    pTime->deltaTime = pTime->currentTime - pTime->lastTime;
    pTime->lastTime = pTime->currentTime;
}

static void childMainLoop(FbrApp *pApp)
{
    int exitCounter = 0;

    // does the pointer indirection here actually cause issue!?
    FbrVulkan *pVulkan = pApp->pVulkan;
    FbrTimelineSemaphore *pParentSemaphore = pApp->pNodeParent->pParentSemaphore;
    FbrTimelineSemaphore *pChildSemaphore = pApp->pNodeParent->pChildSemaphore;
    FbrCamera *pCamera = pApp->pCamera;
    FbrTime *pTime = pApp->pTime;
    FbrPipelines *pPipelines = pApp->pPipelines;
    FbrDescriptors *pDescriptors = pApp->pDescriptors;

    const uint64_t parentTimelineStep = 120;
    vkGetSemaphoreCounterValue(pVulkan->device, pParentSemaphore->semaphore, &pParentSemaphore->waitValue);
    pParentSemaphore->waitValue += parentTimelineStep;
    FBR_LOG_DEBUG("starting parent timeline value", pParentSemaphore->waitValue);

    uint8_t timelineSwitch = 0;

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {
//        FBR_LOG_DEBUG("Child FPS", 1.0 / pApp->pTime->deltaTime);

        updateTime(pTime);

        // Update to current parent time, don't let it go faster than parent allows.
        vkGetSemaphoreCounterValue(pVulkan->device, pParentSemaphore->semaphore, &pParentSemaphore->waitValue);

        beginFrameCommandBuffer(pVulkan, pApp->pFramebuffers[timelineSwitch]->pColorTexture->extent);

        // Receive camera transform over CPU IPC from parent
        FbrNodeCameraIPCBuffer *pCameraIPCBuffer = pApp->pNodeParent->pCameraIPCBuffer->pBuffer;
        glm_mat4_copy(pCameraIPCBuffer->view, pCamera->bufferData.view);
        glm_mat4_copy(pCameraIPCBuffer->proj, pCamera->bufferData.proj);
        glm_mat4_copy(pCameraIPCBuffer->model, pCamera->pTransform->uboData.model);
        pCamera->bufferData.width = pCameraIPCBuffer->width;
        pCamera->bufferData.height = pCameraIPCBuffer->height;
        vec4 pos;
        mat4 rot;
        vec3 scale;
        glm_decompose(pCamera->pTransform->uboData.model, pos, rot, scale);
        glm_mat4_quat(rot, pCamera->pTransform->rot);
        glm_vec3_copy(pos, pCamera->pTransform->pos);
        fbrUpdateCameraUBO(pCamera);

        // Acquire Framebuffer Ownership
        fbrAcquireFramebufferFromExternalToGraphicsAttach(pVulkan, pApp->pFramebuffers[timelineSwitch]);

        beginRenderPassImageless(pVulkan,
                                 pApp->pFramebuffers[timelineSwitch],
                                 pVulkan->renderPass,
                                 (VkClearColorValue) {{0.0f, 0.0f, 0.0f, 0.0f}});
        vkCmdBindPipeline(pVulkan->graphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelines->graphicsPipeStandard);

        // Global
//        uint32_t zeroOffset = 0;
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->graphicsPipeLayoutStandard,
                                FBR_GLOBAL_SET_INDEX,
                                1,
                                &pDescriptors->setGlobal,
                                0,
                                NULL);
//        // Pass
//        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                pPipelines->graphicsPipeLayoutStandard,
//                                FBR_PASS_SET_INDEX,
//                                1,
//                                &pDescriptors->setPass,
//                                0,
//                                NULL);
        // Material
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->graphicsPipeLayoutStandard,
                                FBR_MATERIAL_SET_INDEX,
                                1,
                                &pApp->testQuadMaterialSet,
                                0,
                                NULL);
        //cube 1
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->graphicsPipeLayoutStandard,
                                FBR_OBJECT_SET_INDEX,
                                1,
                                &pApp->testQuadObjectSet,
                                0,
                                NULL);
        recordRenderMesh(pVulkan,
                         pApp->pTestQuadMesh);
        // end framebuffer pass
        vkCmdEndRenderPass(pVulkan->graphicsCommandBuffer);

        fbrUpdateNodeParentMesh(pVulkan, pCamera, timelineSwitch, pApp->pNodeParent);

        fbrReleaseFramebufferFromGraphicsAttachToExternalRead(pVulkan, pApp->pFramebuffers[timelineSwitch]);

        FBR_ACK_EXIT(vkEndCommandBuffer(pVulkan->graphicsCommandBuffer));

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
        FBR_ACK_EXIT(vkWaitSemaphores(pVulkan->device, &releaseSemaphoreWaitInfo, UINT64_MAX));\

        // for some reason this fixes a bug with validation layers thinking the graphicsQueue hasn't finished wait on timeline should be enough!!!
        if (pVulkan->enableValidationLayers) {
            FBR_ACK_EXIT(vkQueueWaitIdle(pVulkan->graphicsQueue));
        }

        timelineSwitch = (timelineSwitch + 1) % 2;

        exitCounter++;
        if (exitCounter > 10) {
            _exit(0);
        }
    }
}

//static void parentMainLoopComputeSSDM(FbrApp *pApp) {
//    FbrVulkan *pVulkan = pApp->pVulkan;
//    FbrSwap *pSwap = pApp->pSwap;
//    FbrTimelineSemaphore *pMainTimelineSemaphore = pVulkan->pMainTimelineSemaphore;
//    FbrTime *pTime = pApp->pTime;
//    FbrCamera *pCamera = pApp->pCamera;
//    FbrPipelines *pPipelines = pApp->pPipelines;
//    FbrDescriptors *pDescriptors = pApp->pDescriptors;
//    FbrNode *pTestNode = pApp->pTestNode;
//
//    uint64_t priorChildTimeline = 0;
//    uint8_t testNodeTimelineSwitch = 1;
//    uint8_t mainFrameBufferIndex = 0;
//
//    VkExtent2D extents = pSwap->extent;
//
//    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {
////        FBR_LOG_DEBUG("Parent FPS", 1.0f / pTime->deltaTime);
//
//        updateTime(pTime);
//
//        processInputFrame(pApp);
//
//        beginFrameCommandBuffer(pVulkan, extents);
//
//        fbrUpdateCameraUBO(pCamera);
//
//        fbrTransitionFramebufferFromIgnoredReadToGraphicsAttach(pVulkan, pApp->pFramebuffers[mainFrameBufferIndex]);
//
//        // -------------------------------------------------------------------------------------------------------------
//        // Retrieve semaphore timeline value of child node to see if rendering is complete
//        //TODO is reading the semaphore slower than just sharing CPU memory?
//        vkGetSemaphoreCounterValue(pVulkan->device,
//                                   pTestNode->pChildSemaphore->semaphore,
//                                   &pTestNode->pChildSemaphore->waitValue);
//        if (priorChildTimeline != pTestNode->pChildSemaphore->waitValue) {
//            priorChildTimeline = pTestNode->pChildSemaphore->waitValue;
//            testNodeTimelineSwitch = (testNodeTimelineSwitch + 1) % 2;
//
//            // Acquire Child Framebuffer Ownership
//            fbrAcquireFramebufferFromExternalAttachToGraphicsRead(pVulkan,
//                                                                  pTestNode->pFramebuffers[testNodeTimelineSwitch]);
//
//            // camera ipc to parent camera ubo, then update camera ipc to latest parent ubo
//            memcpy(&pTestNode->pCamera->bufferData, pTestNode->pCameraIPCBuffer->pBuffer, sizeof(FbrCameraBuffer));
//            fbrUpdateCameraUBO(pTestNode->pCamera);
//            memcpy( pTestNode->pCameraIPCBuffer->pBuffer, &pCamera->bufferData, sizeof(FbrCameraBuffer));
//        }
//
//        // Begin Parent Render Pass
//        beginRenderPassImageless(pVulkan,
//                                 pApp->pFramebuffers[mainFrameBufferIndex],
//                                 pVulkan->renderPass,
//                                 (VkClearColorValue ){{0.1f, 0.2f, 0.3f, 0.0f}});
//
//        // Begin Render Commands
//        vkCmdBindPipeline(pVulkan->graphicsCommandBuffer,
//                          VK_PIPELINE_BIND_POINT_GRAPHICS,
//                          pPipelines->graphicsPipeStandard);
//        // Global
//        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                pPipelines->graphicsPipeLayoutStandard,
//                                FBR_GLOBAL_SET_INDEX,
//                                1,
//                                &pDescriptors->setGlobal,
//                                0,
//                                NULL);
////        // Pass
////        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
////                                VK_PIPELINE_BIND_POINT_GRAPHICS,
////                                pPipelines->graphicsPipeLayoutStandard,
////                                FBR_PASS_SET_INDEX,
////                                1,
////                                &pDescriptors->setPass,
////                                0,
////                                NULL);
//        // Material
//        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                pPipelines->graphicsPipeLayoutStandard,
//                                FBR_MATERIAL_SET_INDEX,
//                                1,
//                                &pApp->testQuadMaterialSet,
//                                0,
//                                NULL);
//
//        //cube 1
//        fbrUpdateTransformUBO(pApp->pTestQuadTransform);
//        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                pPipelines->graphicsPipeLayoutStandard,
//                                FBR_OBJECT_SET_INDEX,
//                                1,
//                                &pApp->testQuadObjectSet,
//                                0,
//                                NULL);
//        recordRenderMesh(pVulkan,
//                         pApp->pTestQuadMesh);
//
//
//        if (pTestNode != NULL) {
//            // Material
//            fbrUpdateTransformUBO(pTestNode->pTransform);
//            vkCmdBindPipeline(pVulkan->graphicsCommandBuffer,
//                              VK_PIPELINE_BIND_POINT_GRAPHICS,
//                              pPipelines->graphicsPipeNodeTess);
//            vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
//                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                    pPipelines->graphicsPipeLayoutNodeTess,
//                                    FBR_GLOBAL_SET_INDEX,
//                                    1,
//                                    &pDescriptors->setGlobal,
//                                    0,
//                                    NULL);
//            vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
//                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                    pPipelines->graphicsPipeLayoutNodeTess,
//                                    FBR_NODE_SET_INDEX,
//                                    1,
//                                    &pApp->pCompMaterialSets[testNodeTimelineSwitch],
//                                    0,
//                                    NULL);
//            recordNodeRenderPass(pVulkan,
//                                 pTestNode,
//                                 testNodeTimelineSwitch);
////            uint64_t childWaitValue = pTestNode->pChildSemaphore->waitValue;
////            FBR_LOG_DEBUG("Displaying: ", testNodeTimelineSwitch, childWaitValue);
//        }
//
//        vkCmdEndRenderPass(pVulkan->graphicsCommandBuffer);
//        // End of Graphics Commands
//
//        fbrReleaseFramebufferFromGraphicsAttachToComputeRead(pVulkan, pApp->pFramebuffers[mainFrameBufferIndex]);
//
//        FBR_ACK_EXIT(vkEndCommandBuffer(pVulkan->graphicsCommandBuffer));
//        // End Command Buffer
//
//        // Submit Graphics
//        const uint64_t waitValue = pMainTimelineSemaphore->waitValue;
//        const uint64_t pRenderWaitSemaphoreValues[] = {
//                waitValue,
//        };
//        const VkSemaphore pRenderWaitSemaphores[] = {
//                pMainTimelineSemaphore->semaphore,
//        };
//        const VkSemaphore pRenderSignalSemaphores[] = {
//                pApp->pFramebuffers[mainFrameBufferIndex]->renderCompleteSemaphore
//        };
//        const VkPipelineStageFlags pRenderWaitDstStageMask[] = {
//                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//        };
//        const VkTimelineSemaphoreSubmitInfo renderTimelineSemaphoreSubmitInfo = {
//                .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
//                .pNext = NULL,
//                .waitSemaphoreValueCount = 1,
//                .pWaitSemaphoreValues =  pRenderWaitSemaphoreValues,
//                .signalSemaphoreValueCount = 0,
//                .pSignalSemaphoreValues = NULL,
//        };
//        const VkSubmitInfo renderSubmitInfo = {
//                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
//                .pNext = &renderTimelineSemaphoreSubmitInfo,
//                .waitSemaphoreCount = 1,
//                .pWaitSemaphores = pRenderWaitSemaphores,
//                .pWaitDstStageMask = pRenderWaitDstStageMask,
//                .commandBufferCount = 1,
//                .pCommandBuffers = &pVulkan->graphicsCommandBuffer,
//                .signalSemaphoreCount = 1,
//                .pSignalSemaphores = pRenderSignalSemaphores
//        };
//        FBR_ACK_EXIT(vkQueueSubmit(pVulkan->graphicsQueue,
//                                   1,
//                                   &renderSubmitInfo,
//                                   VK_NULL_HANDLE));
//        // End Submit Graphics
//
//        // Acquire Compute Swap
//        uint32_t swapIndex;
//        FBR_ACK_EXIT(vkAcquireNextImageKHR(pVulkan->device,
//                                           pSwap->swapChain,
//                                           UINT64_MAX,
//                                           pSwap->acquireCompleteSemaphore,
//                                           VK_NULL_HANDLE,
//                                           &swapIndex));
//
//        // Begin Compute Command Buffer
//        FBR_ACK_EXIT(vkResetCommandBuffer(pVulkan->computeCommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
//        const VkCommandBufferBeginInfo computeBeginInfo = {
//                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//        };
//        FBR_ACK_EXIT(vkBeginCommandBuffer(pVulkan->computeCommandBuffer, &computeBeginInfo));
//
//        // Acquire framebuffers
//        fbrAcquireFramebufferFromGraphicsAttachToComputeRead(pVulkan, pApp->pFramebuffers[mainFrameBufferIndex]);
//        const VkImageMemoryBarrier pTransitionBlitBarrier[] = {
//                {
//                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//                        .srcAccessMask = 0,
//                        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
//                        .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
//                        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
//                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//                        .image = pSwap->pSwapImages[swapIndex],
//                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
//                },
//        };
//        vkCmdPipelineBarrier(pVulkan->computeCommandBuffer,
//                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
//                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
//                             0,
//                             0, NULL,
//                             0, NULL,
//                             COUNT(pTransitionBlitBarrier), pTransitionBlitBarrier);
//
//        // Set descriptor sets
//        FbrSetComposite setComposite;
//        fbrCreateSetComputeComposite(pApp->pVulkan,
//                              pApp->pDescriptors->setLayoutComposite,
//                              pApp->pFramebuffers[mainFrameBufferIndex]->pColorTexture->imageView,
//                              pApp->pFramebuffers[mainFrameBufferIndex]->pNormalTexture->imageView,
//                              pApp->pFramebuffers[mainFrameBufferIndex]->pGBufferTexture->imageView,
//                              pApp->pFramebuffers[mainFrameBufferIndex]->pDepthTexture->imageView,
//                              pSwap->pSwapImageViews[swapIndex],
//                              &setComposite);
//        vkCmdBindPipeline(pVulkan->computeCommandBuffer,
//                          VK_PIPELINE_BIND_POINT_COMPUTE,
//                          pApp->pPipelines->computePipeComposite);
//        vkCmdBindDescriptorSets(pVulkan->computeCommandBuffer,
//                                VK_PIPELINE_BIND_POINT_COMPUTE,
//                                pApp->pPipelines->computePipeLayoutComposite,
//                                FBR_GLOBAL_SET_INDEX,
//                                1,
//                                &pDescriptors->setGlobal,
//                                0,
//                                NULL);
//        vkCmdBindDescriptorSets(pVulkan->computeCommandBuffer,
//                                VK_PIPELINE_BIND_POINT_COMPUTE,
//                                pApp->pPipelines->computePipeLayoutComposite,
//                                FBR_COMPOSITE_SET_INDEX,
//                                1,
//                                &setComposite,
//                                0,
//                                NULL);
//
//        // Dispatch compute timing queries
//        vkResetQueryPool(pVulkan->device, pVulkan->queryPool, 0, 2);
//        vkCmdWriteTimestamp(pVulkan->computeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, pVulkan->queryPool, 0);
//        const int localSize = 32;
//        vkCmdDispatch(pVulkan->computeCommandBuffer, (extents.width / localSize), (extents.height / localSize), 1);
//        vkCmdWriteTimestamp(pVulkan->computeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, pVulkan->queryPool, 1);
//
//        const VkImageMemoryBarrier pTransitionEndBlitBarrier[] = {
//                {
//                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//                    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
//                    .dstAccessMask = 0,
//                    .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
//                    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
//                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//                    .image = pSwap->pSwapImages[swapIndex],
//                    FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
//                },
//        };
//        vkCmdPipelineBarrier(pVulkan->computeCommandBuffer,
//                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
//                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ,
//                             0,
//                             0, NULL,
//                             0, NULL,
//                             COUNT(pTransitionEndBlitBarrier), pTransitionEndBlitBarrier);
//
//        FBR_ACK_EXIT(vkEndCommandBuffer(pVulkan->computeCommandBuffer));
//        // End Compute Command Buffer
//
//        // Submit Compute
//        const VkSemaphore pComputeWaitSemaphores[] = {
//                pApp->pFramebuffers[mainFrameBufferIndex]->renderCompleteSemaphore,
//                pSwap->acquireCompleteSemaphore,
//        };
//        const VkPipelineStageFlags pComputeWaitDstStageMask[] = {
//                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
//                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
//        };
//        pMainTimelineSemaphore->waitValue++;
//        const uint64_t signalValue = pMainTimelineSemaphore->waitValue;
//        const uint64_t pComputeSignalSemaphoreValues[] = {
//                signalValue,
//                0,
//        };
//        const VkSemaphore pSignalSemaphores[] = {
//                pMainTimelineSemaphore->semaphore,
//                pSwap->renderCompleteSemaphore
//        };
//        const VkTimelineSemaphoreSubmitInfo computeTimelineSemaphoreSubmitInfo = {
//                .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
//                .pNext = NULL,
//                .waitSemaphoreValueCount = 0,
//                .pWaitSemaphoreValues =  NULL,
//                .signalSemaphoreValueCount = 2,
//                .pSignalSemaphoreValues = pComputeSignalSemaphoreValues,
//        };
//        const VkSubmitInfo computeSubmitInfo = {
//                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
//                .pNext = &computeTimelineSemaphoreSubmitInfo,
//                .waitSemaphoreCount = 2,
//                .pWaitSemaphores = pComputeWaitSemaphores,
//                .pWaitDstStageMask = pComputeWaitDstStageMask,
//                .commandBufferCount = 1,
//                .pCommandBuffers = &pVulkan->computeCommandBuffer,
//                .signalSemaphoreCount = 2,
//                .pSignalSemaphores = pSignalSemaphores
//        };
//        FBR_ACK_EXIT(vkQueueSubmit(pVulkan->computeQueue,
//                                   1,
//                                   &computeSubmitInfo,
//                                   VK_NULL_HANDLE));
//        // End Submit Compute
//
//        // Submit Present
//        // TODO want to use id+wait ? Doesn't work on Quest 2.
//        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_present_id.html
//        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_present_wait.html
//        const VkPresentInfoKHR presentInfo = {
//                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
//                .waitSemaphoreCount = 1,
//                .pWaitSemaphores = &pSwap->renderCompleteSemaphore,
//                .swapchainCount = 1,
//                .pSwapchains = &pSwap->swapChain,
//                .pImageIndices = &swapIndex,
//        };
//        FBR_ACK_EXIT(vkQueuePresentKHR(pVulkan->computeQueue, &presentInfo));
//        // End Submit Present
//
//        // Wait!
//        const VkSemaphoreWaitInfo semaphoreWaitInfo = {
//                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
//                .pNext = NULL,
//                .flags = 0,
//                .semaphoreCount = 1,
//                .pSemaphores = &pMainTimelineSemaphore->semaphore,
//                .pValues = &pMainTimelineSemaphore->waitValue,
//        };
//        FBR_ACK_EXIT(vkWaitSemaphores(pVulkan->device, &semaphoreWaitInfo, UINT64_MAX));
//
//
//        // for some reason this fixes a bug with validation layers thinking the graphicsQueue hasnt finished
//        // wait on timeline should be enough!!
//        if (pVulkan->enableValidationLayers) {
//            FBR_ACK_EXIT(vkQueueWaitIdle(pVulkan->computeQueue));
//        }
//
//        // log measured compute time
////        uint64_t timestamps[2];
////        vkGetQueryPoolResults(pVulkan->device, pVulkan->queryPool, 0, 2, sizeof(uint64_t) * 2, timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );
////        float ms = (float)(timestamps[1] - timestamps[0]) / 1000000.0f;
////        FBR_LOG_DEBUG("Compute: ", ms);
//
//        vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &setComposite);
//
//        mainFrameBufferIndex = !mainFrameBufferIndex;
//    }
//}

static void parentMainLoopTessellation(FbrApp *pApp) {
    FbrVulkan *pVulkan = pApp->pVulkan;
    FbrSwap *pSwap = pApp->pSwap;
    FbrTimelineSemaphore *pMainTimelineSemaphore = pVulkan->pMainTimelineSemaphore;
    FbrTime *pTime = pApp->pTime;
    FbrCamera *pCamera = pApp->pCamera;
    FbrPipelines *pPipelines = pApp->pPipelines;
    FbrDescriptors *pDescriptors = pApp->pDescriptors;
    FbrNode *pTestNode = pApp->pTestNode;

    uint64_t priorChildTimeline = 0;
    uint8_t testNodeTimelineSwitch = 1;
    uint8_t mainFrameBufferIndex = 0;

    VkExtent2D extents = pSwap->extent;

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {
//        FBR_LOG_DEBUG("Parent FPS", 1.0f / pTime->deltaTime);

        updateTime(pTime);

        processInputFrame(pApp);

        beginFrameCommandBuffer(pVulkan, extents);

        fbrUpdateCameraUBO(pCamera);

        // -------------------------------------------------------------------------------------------------------------
        // Retrieve semaphore timeline value of child node to see if rendering is complete
        //TODO is reading the semaphore slower than just sharing CPU memory?
        vkGetSemaphoreCounterValue(pVulkan->device,
                                   pTestNode->pChildSemaphore->semaphore,
                                   &pTestNode->pChildSemaphore->waitValue);
        if (priorChildTimeline != pTestNode->pChildSemaphore->waitValue) {
            priorChildTimeline = pTestNode->pChildSemaphore->waitValue;
            testNodeTimelineSwitch = (testNodeTimelineSwitch + 1) % 2;

            // Acquire Child Framebuffer Ownership
            fbrAcquireFramebufferFromExternalAttachToGraphicsRead(pVulkan,pTestNode->pFramebuffers[testNodeTimelineSwitch]);

            // Copy the camera transform which child just used to render to the node camera
            FbrNodeCameraIPCBuffer *pRenderingNodeCameraIPCBuffer = pTestNode->pRenderingNodeCameraIPCBuffer;
            glm_mat4_copy(pRenderingNodeCameraIPCBuffer->view, pTestNode->pCamera->bufferData.view);
            glm_mat4_copy(pRenderingNodeCameraIPCBuffer->proj, pTestNode->pCamera->bufferData.proj);
            glm_mat4_copy(pRenderingNodeCameraIPCBuffer->model, pTestNode->pCamera->pTransform->uboData.model);
            pTestNode->pCamera->bufferData.width = pRenderingNodeCameraIPCBuffer->width;
            pTestNode->pCamera->bufferData.height = pRenderingNodeCameraIPCBuffer->height;
            fbrUpdateCameraUBO(pTestNode->pCamera);

            // Update camera min/max projection
            float distanceToCenter = glm_vec3_distance(pTestNode->pTransform->pos, pCamera->pTransform->pos);
            vec3 viewPosition;
            glm_mat4_mulv3(pCamera->bufferData.view, pTestNode->pTransform->pos, 1, viewPosition);
            float viewDistanceToCenter = -viewPosition[2];
            float offset = 0.5f;
            float nearZ = viewDistanceToCenter - offset;
//            float farZ = viewDistanceToCenter + offset;
            float farZ = viewDistanceToCenter;
            if (nearZ < FBR_CAMERA_NEAR_DEPTH) {
                nearZ = FBR_CAMERA_NEAR_DEPTH;
            }
            glm_perspective(FBR_CAMERA_FOV, pVulkan->screenFOV, nearZ, farZ, pRenderingNodeCameraIPCBuffer->proj);

            FBR_LOG_DEBUG(distanceToCenter, viewDistanceToCenter, nearZ, farZ);

            // Copy the current parent camera transform to the CPU IPC for the child to use to render next frame
            glm_mat4_copy(pCamera->bufferData.view, pRenderingNodeCameraIPCBuffer->view);
            glm_mat4_copy(pCamera->bufferData.proj, pRenderingNodeCameraIPCBuffer->proj);
            glm_mat4_copy(pCamera->pTransform->uboData.model, pRenderingNodeCameraIPCBuffer->model);
            pRenderingNodeCameraIPCBuffer->width = pCamera->bufferData.width;
            pRenderingNodeCameraIPCBuffer->height = pCamera->bufferData.height;
            memcpy( pTestNode->pCameraIPCBuffer->pBuffer, pRenderingNodeCameraIPCBuffer, sizeof(FbrNodeCameraIPCBuffer));
        }

//        fbrUpdateTransformUBO(pApp->pTestQuadTransform);
//        fbrUpdateTransformUBO(pTestNode->pTransform);

        // Begin Parent Render Pass
        beginRenderPassImageless(pVulkan,
                                 pApp->pFramebuffers[mainFrameBufferIndex],
                                 pVulkan->renderPass,
                                 (VkClearColorValue ){{0.1f, 0.2f, 0.3f, 0.0f}});

        // Begin Render Commands
        vkCmdBindPipeline(pVulkan->graphicsCommandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pPipelines->graphicsPipeStandard);
        // Global
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->graphicsPipeLayoutStandard,
                                FBR_GLOBAL_SET_INDEX,
                                1,
                                &pDescriptors->setGlobal,
                                0,
                                NULL);
//        // Pass
//        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                pPipelines->graphicsPipeLayoutStandard,
//                                FBR_PASS_SET_INDEX,
//                                1,
//                                &pDescriptors->setPass,
//                                0,
//                                NULL);
        // Material
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->graphicsPipeLayoutStandard,
                                FBR_MATERIAL_SET_INDEX,
                                1,
                                &pApp->testQuadMaterialSet,
                                0,
                                NULL);

        //cube 1
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->graphicsPipeLayoutStandard,
                                FBR_OBJECT_SET_INDEX,
                                1,
                                &pApp->testQuadObjectSet,
                                0,
                                NULL);
        recordRenderMesh(pVulkan,
                         pApp->pTestQuadMesh);


        // Tesselation Node
        vkCmdBindPipeline(pVulkan->graphicsCommandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pPipelines->graphicsPipeNodeTess);
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->graphicsPipeLayoutNodeTess,
                                FBR_GLOBAL_SET_INDEX,
                                1,
                                &pDescriptors->setGlobal,
                                0,
                                NULL);
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->graphicsPipeLayoutNodeTess,
                                FBR_NODE_SET_INDEX,
                                1,
                                &pApp->pCompMaterialSets[testNodeTimelineSwitch],
                                0,
                                NULL);
        recordNodeRenderPass(pVulkan,
                             pTestNode,
                             testNodeTimelineSwitch);


        // Mesh Shader Node
        vkCmdBindPipeline(pVulkan->graphicsCommandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pPipelines->graphicsPipeNodeMesh);
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->graphicsPipeLayoutNodeMesh,
                                FBR_GLOBAL_SET_INDEX,
                                1,
                                &pDescriptors->setGlobal,
                                0,
                                NULL);
        vkCmdBindDescriptorSets(pVulkan->graphicsCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipelines->graphicsPipeLayoutNodeMesh,
                                FBR_MESH_COMPOSITE_SET_INDEX,
                                1,
                                &pDescriptors->setMeshComposites[testNodeTimelineSwitch],
                                0,
                                NULL);

//        const int queryCount = 6;
//        vkResetQueryPool(pVulkan->device, pVulkan->queryPool, 0, queryCount);
//        vkCmdWriteTimestamp(pVulkan->graphicsCommandBuffer, VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT,  pVulkan->queryPool, 0);
//        vkCmdWriteTimestamp(pVulkan->graphicsCommandBuffer, VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT,  pVulkan->queryPool, 1);
//        vkCmdWriteTimestamp(pVulkan->graphicsCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,  pVulkan->queryPool, 2);
//        pVulkan->functions.cmdDrawMeshTasks(pVulkan->graphicsCommandBuffer, 1, 1, 1);
//        vkCmdWriteTimestamp(pVulkan->graphicsCommandBuffer, VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT,  pVulkan->queryPool, 3);
//        vkCmdWriteTimestamp(pVulkan->graphicsCommandBuffer, VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT,  pVulkan->queryPool, 4);
//        vkCmdWriteTimestamp(pVulkan->graphicsCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,  pVulkan->queryPool, 5);

        const int queryCount = 2;
        vkResetQueryPool(pVulkan->device, pVulkan->queryPool, 0, queryCount);
        vkCmdWriteTimestamp(pVulkan->graphicsCommandBuffer, VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT,  pVulkan->queryPool, 0);
        pVulkan->functions.cmdDrawMeshTasks(pVulkan->graphicsCommandBuffer, 1, 1, 1);
        vkCmdWriteTimestamp(pVulkan->graphicsCommandBuffer, VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT,  pVulkan->queryPool, 1);

        vkCmdEndRenderPass(pVulkan->graphicsCommandBuffer);
        // End of Graphics Commands

        // Transfer and blit to swap and transfer back
        uint32_t swapIndex;
        FBR_ACK_EXIT(vkAcquireNextImageKHR(pVulkan->device,
                                           pSwap->swapChain,
                                           UINT64_MAX,
                                           pSwap->acquireCompleteSemaphore,
                                           VK_NULL_HANDLE,
                                           &swapIndex));
        const VkImageMemoryBarrier pTransitionBlitBarrier[] = {
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = 0,
                        .dstAccessMask = 0,
                        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .image = pApp->pFramebuffers[mainFrameBufferIndex]->pColorTexture->image,
                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                },
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = 0,
                        .dstAccessMask = 0,
                        .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .image = pSwap->pSwapImages[swapIndex],
                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                },
        };
        vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT ,
                             0,
                             0, NULL,
                             0, NULL,
                             2, pTransitionBlitBarrier);
        const VkImageBlit imageBlit = {
                .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .srcSubresource.mipLevel = 0,
                .srcSubresource.layerCount = 1,
                .srcSubresource.baseArrayLayer = 0,
                .srcOffsets[1].x = extents.width,
                .srcOffsets[1].y = extents.height,
                .srcOffsets[1].z = 1,
                .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .dstSubresource.mipLevel = 0,
                .dstSubresource.layerCount = 1,
                .dstSubresource.baseArrayLayer = 0,
                .dstOffsets[1].x = extents.width,
                .dstOffsets[1].y = extents.height,
                .dstOffsets[1].z = 1,
        };
        vkCmdBlitImage(pVulkan->graphicsCommandBuffer,
                       pApp->pFramebuffers[mainFrameBufferIndex]->pColorTexture->image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       pSwap->pSwapImages[swapIndex],
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &imageBlit,
                       VK_FILTER_NEAREST);
        const VkImageMemoryBarrier pTransitionPresentBarrier[] = {
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = 0,
                        .dstAccessMask = 0,
                        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .image = pApp->pFramebuffers[mainFrameBufferIndex]->pColorTexture->image,
                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                },
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = 0,
                        .dstAccessMask = 0,
                        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        .srcQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .dstQueueFamilyIndex = pVulkan->graphicsQueueFamilyIndex,
                        .image = pSwap->pSwapImages[swapIndex],
                        FBR_DEFAULT_COLOR_SUBRESOURCE_RANGE
                },
        };
        vkCmdPipelineBarrier(pVulkan->graphicsCommandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ,
                             0,
                             0, NULL,
                             0, NULL,
                             2, pTransitionPresentBarrier);
        // end transfer and blit to swap and transfer back

        FBR_ACK_EXIT(vkEndCommandBuffer(pVulkan->graphicsCommandBuffer));
        // End Command Buffer

        submitQueueAndPresent(pVulkan, pSwap, pMainTimelineSemaphore, swapIndex);

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

        uint64_t timestamps[queryCount];
        vkGetQueryPoolResults(pVulkan->device, pVulkan->queryPool, 0, queryCount, sizeof(uint64_t) * queryCount, timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );
        float ms = (float)(timestamps[1] - timestamps[0]) / 1000000.0f;
        FBR_LOG_MESSAGE("Task/Mesh: ", ms);

        mainFrameBufferIndex = !mainFrameBufferIndex;

        // for some reason this fixes a bug with validation layers thinking the graphicsQueue hasnt finished
        // wait on timeline should be enough!!
        if (pVulkan->enableValidationLayers) {
            FBR_ACK_EXIT(vkQueueWaitIdle(pVulkan->graphicsQueue));
        }
    }
}

static void setHighPriority(){
    // ovr example does this, is it good? https://github.com/ValveSoftware/virtual_display/blob/da13899ea6b4c0e4167ed97c77c6d433718489b1/virtual_display/virtual_display.cpp
#define THREAD_PRIORITY_MOST_URGENT 15
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_MOST_URGENT );
    SetPriorityClass(GetCurrentThread(), REALTIME_PRIORITY_CLASS);
}

void fbrMainLoop(FbrApp *pApp) {
    FBR_LOG_DEBUG("mainloop starting!");

    if (!pApp->isChild) {
        setHighPriority();
//        parentMainLoopComputeSSDM(pApp);
        parentMainLoopTessellation(pApp);
    } else {
        childMainLoop(pApp);
    }

    vkQueueWaitIdle(pApp->pVulkan->graphicsQueue);
    vkDeviceWaitIdle(pApp->pVulkan->device);
}