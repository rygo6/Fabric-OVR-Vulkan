#include "fbr_core.h"
#include "fbr_mesh.h"
#include "fbr_pipeline.h"
#include "fbr_texture.h"
#include "fbr_camera.h"
#include "fbr_log.h"
#include "fbr_vulkan.h"
#include "fbr_input.h"
#include "fbr_framebuffer.h"

#include "cglm/cglm.h"

static void waitForLastFrame(FbrVulkan *pVulkan) {
    vkWaitForFences(pVulkan->device, 1, &pVulkan->inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(pVulkan->device, 1, &pVulkan->inFlightFence);
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

static uint32_t beginRenderPass(const FbrVulkan *pVulkan, const VkRenderPass renderPass, const VkFramebuffer framebuffer) {
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = framebuffer,
            .renderArea.offset = {0, 0},
            .renderArea.extent = pVulkan->swapChainExtent,
            .clearValueCount = 1,
            .pClearValues = &clearColor,
    };
    vkCmdBeginRenderPass(pVulkan->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

static void recordRenderPass(const FbrVulkan *pVulkan, const FbrPipeline *pPipeline, const FbrMesh *pMesh) {
    vkCmdBindPipeline(pVulkan->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->graphicsPipeline);

    vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pPipeline->pipelineLayout,
                            0,
                            1,
                            &pPipeline->descriptorSet,
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

static void endRenderPassNoSwapChain(FbrVulkan *pVulkan, FbrFramebuffer *pFrambuffer) {
    vkCmdEndRenderPass(pVulkan->commandBuffer);

    FBR_VK_CHECK(vkEndCommandBuffer(pVulkan->commandBuffer));

    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = NULL,
            .pWaitDstStageMask =  NULL,
            .commandBufferCount = 1,
            .pCommandBuffers = &pVulkan->commandBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = NULL,
    };

    FBR_VK_CHECK(vkQueueSubmit(pVulkan->queue, 1, &submitInfo, pVulkan->inFlightFence));
}

static void submitToPresenmt(FbrVulkan *pVulkan, uint32_t imageIndex) {
    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &pVulkan->imageAvailableSemaphore,
            .pWaitDstStageMask =  (const VkPipelineStageFlags[]) { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            .commandBufferCount = 1,
            .pCommandBuffers = &pVulkan->commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &pVulkan->renderFinishedSemaphore,
    };

    FBR_VK_CHECK(vkQueueSubmit(pVulkan->queue, 1, &submitInfo, pVulkan->inFlightFence));

    VkSwapchainKHR swapChains[] = {pVulkan->swapChain};
    VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &pVulkan->renderFinishedSemaphore,
            .swapchainCount = 1,
            .pSwapchains = swapChains,
            .pImageIndices = &imageIndex,
    };

    vkQueuePresentKHR(pVulkan->queue, &presentInfo);
}

void fbrMainLoop(FbrApp *pApp) {
    FBR_LOG_DEBUG("mainloop starting!");

    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(pApp->pWindow) && !pApp->exiting) {
        pApp->pTime->currentTime = glfwGetTime();
        pApp->pTime->deltaTime = pApp->pTime->currentTime - lastFrameTime;
        lastFrameTime = pApp->pTime->currentTime;

        waitForLastFrame(pApp->pVulkan);
        processInputFrame(pApp);

        fbrUpdateCameraUBO(pApp->pCamera);

        // Get open swap image
        uint32_t swapIndex;
        vkAcquireNextImageKHR(pApp->pVulkan->device,
                              pApp->pVulkan->swapChain,
                              UINT64_MAX,
                              pApp->pVulkan->imageAvailableSemaphore,
                              VK_NULL_HANDLE,
                              &swapIndex);

        // begin command buffer
        vkResetCommandBuffer(pApp->pVulkan->commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        VkCommandBufferBeginInfo beginInfo = {
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
        };
        FBR_VK_CHECK(vkBeginCommandBuffer(pApp->pVulkan->commandBuffer, &beginInfo));

        VkViewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = (float) pApp->pVulkan->swapChainExtent.width,
                .height = (float) pApp->pVulkan->swapChainExtent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
        };
        vkCmdSetViewport(pApp->pVulkan->commandBuffer, 0, 1, &viewport);
        VkRect2D scissor = {
                .offset = {0, 0},
                .extent = pApp->pVulkan->swapChainExtent,
        };
        vkCmdSetScissor(pApp->pVulkan->commandBuffer, 0, 1, &scissor);


//        fbrTransitionForRender(pApp->pVulkan->commandBuffer, pApp->pFramebuffer);
//        beginRenderPass(pApp->pVulkan, pApp->pFramebuffer->renderPass, pApp->pFramebuffer->framebuffer);
//        //cube 1
//        fbrUpdateTransformMatrix(&pApp->pMesh->transform);
//        recordRenderPass(pApp->pVulkan, pApp->pTestPipeline, pApp->pMesh);
//
//        vkCmdEndRenderPass(pApp->pVulkan->commandBuffer);
//        fbrTransitionForDisplay(pApp->pVulkan->commandBuffer, pApp->pFramebuffer);


        //swap pass
        beginRenderPass(pApp->pVulkan, pApp->pVulkan->renderPass, pApp->pVulkan->pSwapChainFramebuffers[swapIndex]);
        //cube 1
        fbrUpdateTransformMatrix(&pApp->pMesh->transform);
        recordRenderPass(pApp->pVulkan, pApp->pPipeline, pApp->pMesh);
        //cube2
        fbrUpdateTransformMatrix(&pApp->pMeshExternalTest->transform);
        recordRenderPass(pApp->pVulkan, pApp->pPipelineExternalTest, pApp->pMeshExternalTest);
        // end swap pass
        vkCmdEndRenderPass(pApp->pVulkan->commandBuffer);


        FBR_VK_CHECK(vkEndCommandBuffer(pApp->pVulkan->commandBuffer));
        submitToPresenmt(pApp->pVulkan, swapIndex);
    }

    vkDeviceWaitIdle(pApp->pVulkan->device);
}