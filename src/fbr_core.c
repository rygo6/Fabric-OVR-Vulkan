#include "fbr_core.h"
#include "fbr_mesh.h"
#include "fbr_pipeline.h"
#include "fbr_texture.h"
#include "fbr_camera.h"
#include "fbr_log.h"
#include "fbr_vulkan.h"
#include "fbr_input.h"

#include "cglm/cglm.h"

#include <stdio.h>

static void waitForLastFrame(FbrVulkan *pVulkan) {
    vkWaitForFences(pVulkan->device, 1, &pVulkan->inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(pVulkan->device, 1, &pVulkan->inFlightFence);
}

static void processInputFrame(FbrApp *pApp) {
    fbrProcessInput();
    for (int i = 0; i < fbrInputEventCount(); ++i) {
        fbrUpdateCamera(pApp->pCamera, fbrGetKeyEvent(i), pApp->pTime);
    }
}

static uint32_t beginFrame(FbrVulkan *pVulkan) {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(pVulkan->device,
                          pVulkan->swapChain,
                          UINT64_MAX,
                          pVulkan->imageAvailableSemaphore,
                          VK_NULL_HANDLE,
                          &imageIndex);

    vkResetCommandBuffer(pVulkan->commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);

    VkCommandBufferBeginInfo beginInfo = {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };

    if (vkBeginCommandBuffer(pVulkan->commandBuffer, &beginInfo) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to begin recording command buffer!");
    }

    return imageIndex;
}

static void recordCommandBuffer(const FbrVulkan *pVulkan, const FbrPipeline *pPipeline, const FbrMesh *pMesh, uint32_t imageIndex) {
    VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pVulkan->renderPass,
            .framebuffer = pVulkan->pSwapChainFramebuffers[imageIndex],
            .renderArea.offset = {0, 0},
            .renderArea.extent = pVulkan->swapChainExtent,
    };

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(pVulkan->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(pVulkan->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->graphicsPipeline);

        VkViewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = (float) pVulkan->swapChainExtent.width,
                .height = (float) pVulkan->swapChainExtent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
        };
        vkCmdSetViewport(pVulkan->commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {
                .offset = {0, 0},
                .extent = pVulkan->swapChainExtent,
        };
        vkCmdSetScissor(pVulkan->commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {pMesh->vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(pVulkan->commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(pVulkan->commandBuffer, pMesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(pVulkan->commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pPipeline->pipelineLayout,
                                0,
                                FBR_DESCRIPTOR_SET_COUNT,
                                &pPipeline->descriptorSet,
                                0,
                                NULL);

        vkCmdDrawIndexed(pVulkan->commandBuffer, FBR_TEST_INDICES_COUNT, 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(pVulkan->commandBuffer);

    if (vkEndCommandBuffer(pVulkan->commandBuffer) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to record command buffer!");
    }
}

static void endFrame(FbrVulkan *pVulkan, uint32_t imageIndex) {
    VkSubmitInfo submitInfo = {
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO
    };

    VkSemaphore waitSemaphores[] = {pVulkan->imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pVulkan->commandBuffer;

    VkSemaphore signalSemaphores[] = {pVulkan->renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(pVulkan->queue, 1, &submitInfo, pVulkan->inFlightFence) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR
    };

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {pVulkan->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(pVulkan->queue, &presentInfo);
}

void fbrMainLoop(FbrApp *pApp) {
    FBR_LOG_DEBUG("mainloop starting!");

    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(pApp->pWindow)) {
        pApp->pTime->currentTime = glfwGetTime();
        pApp->pTime->deltaTime = pApp->pTime->currentTime - lastFrameTime;
        lastFrameTime = pApp->pTime->currentTime;

        waitForLastFrame(pApp->pVulkan);
        processInputFrame(pApp);
        fbrMeshUpdateCameraUBO(pApp->pMesh, pApp->pCamera); //todo I dont like this

        uint32_t imageIndex = beginFrame(pApp->pVulkan);
        recordCommandBuffer(pApp->pVulkan, pApp->pPipeline, pApp->pMesh, imageIndex);
        endFrame(pApp->pVulkan, imageIndex);
    }

    vkDeviceWaitIdle(pApp->pVulkan->device);
}