#include "mxc_camera.h"
#include "mxc_buffer.h"

#include <memory.h>

static void mxcUpdateCameraUBO(mxcCameraState *pCameraState) {
    glm_mat4_identity(pCameraState->ubo.model);
    glm_quat_look(pCameraState->transformState.pos, pCameraState->transformState.rot, pCameraState->ubo.view);
    glm_perspective(90, 1, .01f, 10, pCameraState->ubo.proj);
}

static void createUniformBuffers(mxcAppState* pAppState, mxcCameraState *pCameraState) {
    VkDeviceSize bufferSize = sizeof(mxcCameraUBO);
    createBuffer(pAppState, bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &pCameraState->uniformBuffer,
                 &pCameraState->uniformBufferMemory);

    vkMapMemory(pAppState->device, pCameraState->uniformBufferMemory, 0, bufferSize, 0, &pCameraState->pUniformBufferMapped);
}

void mxcUpdateCamera(mxcCameraState *pCameraState, mxcInputEvent inputEvent) {
    switch (inputEvent.type) {
        case MXC_KEY_INPUT:
            break;
        case MXC_MOUSE_POS_INPUT: {
            float yRot = (float) inputEvent.mousePosInput.xDelta / 100.0f;
            versor rotQ;
            glm_quatv(rotQ, yRot, GLM_YUP);
            glm_quat_mul(pCameraState->transformState.rot, rotQ, pCameraState->transformState.rot);
            mxcUpdateCameraUBO(pCameraState);
            memcpy(pCameraState->pUniformBufferMapped, &pCameraState->ubo, sizeof(mxcCameraUBO));
            break;
        }
        case MXC_MOUSE_BUTTON_INPUT:{


            break;
        }
    }
}

void mxcInitCamera(mxcAppState* pAppState) {
    pAppState->pCameraState = malloc(sizeof(mxcCameraState));

    createUniformBuffers(pAppState, pAppState->pCameraState);

    vec3 pos = {0,0,-1};
    glm_vec3_copy(pos, pAppState->pCameraState->transformState.pos);
    glm_mat4_identity(&pAppState->pCameraState->transformState.rot);
    mxcUpdateCameraUBO(pAppState->pCameraState);
}

void mxcCleanupCamera(mxcAppState* pAppState) {
    vkDestroyBuffer(pAppState->device, pAppState->pCameraState->uniformBuffer, NULL);
    vkFreeMemory(pAppState->device, pAppState->pCameraState->uniformBufferMemory, NULL);
    free(pAppState->pCameraState);
}