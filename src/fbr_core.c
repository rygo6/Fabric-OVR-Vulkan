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

static uint32_t beginRenderPass(FbrVulkan *pVulkan) {
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

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pVulkan->renderPass,
            .framebuffer = pVulkan->pSwapChainFramebuffers[imageIndex],
            .renderArea.offset = {0, 0},
            .renderArea.extent = pVulkan->swapChainExtent,
            .clearValueCount = 1,
            .pClearValues = &clearColor,
    };

    vkCmdBeginRenderPass(pVulkan->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

    return imageIndex;
}

static void recordRenderPass(const FbrVulkan *pVulkan, const FbrPipeline *pPipeline, const FbrCamera *pCamera, const FbrMesh *pMesh) {
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

static void endRenderPass(FbrVulkan *pVulkan, uint32_t imageIndex) {
    vkCmdEndRenderPass(pVulkan->commandBuffer);

    if (vkEndCommandBuffer(pVulkan->commandBuffer) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to record command buffer!");
    }

    VkSemaphore waitSemaphores[] = {pVulkan->imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {pVulkan->renderFinishedSemaphore};
    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &pVulkan->commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signalSemaphores,
    };

    if (vkQueueSubmit(pVulkan->queue, 1, &submitInfo, pVulkan->inFlightFence) != VK_SUCCESS) {
        FBR_LOG_DEBUG("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = {pVulkan->swapChain};
    VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphores,
            .swapchainCount = 1,
            .pSwapchains = swapChains,
            .pImageIndices = &imageIndex,
    };

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

        fbrUpdateCameraUBO(pApp->pCamera);

        uint32_t imageIndex = beginRenderPass(pApp->pVulkan);

        fbrUpdateTransformMatrix(&pApp->pMesh->transform);
        recordRenderPass(pApp->pVulkan, pApp->pPipeline, pApp->pCamera, pApp->pMesh);

//        vec3 add = {.0001f,0,0,};
//        glm_vec3_add(pApp->pMeshExternalTest->transform.pos, add, pApp->pMeshExternalTest->transform.pos);
        fbrUpdateTransformMatrix(&pApp->pMeshExternalTest->transform);
        recordRenderPass(pApp->pVulkan, pApp->pPipelineExternalTest, pApp->pCamera, pApp->pMeshExternalTest);

        endRenderPass(pApp->pVulkan, imageIndex);
    }

    vkDeviceWaitIdle(pApp->pVulkan->device);
}