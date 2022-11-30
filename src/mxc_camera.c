#include "mxc_camera.h"
#include "mxc_buffer.h"
#include "mxc_log.h"

#include <memory.h>

static inline void mxcUpdateCameraUBO(MxcCameraState *pCameraState) {
    glm_mat4_identity(pCameraState->ubo.model);
    glm_quat_look(pCameraState->transformState.pos, pCameraState->transformState.rot, pCameraState->ubo.view);
    glm_perspective(90, 1, .01f, 10, pCameraState->ubo.proj);
    memcpy(pCameraState->pUniformBufferMapped, &pCameraState->ubo, sizeof(MxcCameraUBO));
}

void mxcUpdateCamera(MxcCameraState *pCameraState, MxcInputEvent inputEvent, double deltaFrameTime) {
    switch (inputEvent.type) {
        case MXC_NO_INPUT:
            break;
        case MXC_KEY_INPUT: {
            if (inputEvent.keyInput.action != GLFW_PRESS && inputEvent.keyInput.action != MXC_HELD)
                break;

            const float moveAmount = (float) deltaFrameTime;
            vec3 deltaPos = GLM_VEC3_ZERO_INIT;
            if (inputEvent.keyInput.key == GLFW_KEY_W) {
                deltaPos[2] = -moveAmount;
            } else if (inputEvent.keyInput.key == GLFW_KEY_S) {
                deltaPos[2] = moveAmount;
            } else if (inputEvent.keyInput.key == GLFW_KEY_D) {
                deltaPos[0] = moveAmount;
            } else if (inputEvent.keyInput.key == GLFW_KEY_A) {
                deltaPos[0] = -moveAmount;
            }

            glm_quat_rotatev(pCameraState->transformState.rot, deltaPos, deltaPos);
            glm_vec3_add(pCameraState->transformState.pos, deltaPos, pCameraState->transformState.pos);

            mxcUpdateCameraUBO(pCameraState);

//            mxcLogDebugInfo3("MXC_KEY_INPUT", %f, deltaPos[0], %f, deltaPos[1], %f, deltaPos[2]);
            break;
        }
        case MXC_MOUSE_POS_INPUT: {
            float yRot = (float) -inputEvent.mousePosInput.xDelta / 100.0f;
            versor rotQ;
            glm_quatv(rotQ, yRot, GLM_YUP);
            glm_quat_mul(pCameraState->transformState.rot, rotQ, pCameraState->transformState.rot);

            mxcUpdateCameraUBO(pCameraState);
            break;
        }
        case MXC_MOUSE_BUTTON_INPUT: {

            break;
        }
    }
}

static void createUniformBuffers(MxcAppState *pAppState, MxcCameraState *pCameraState) {
    VkDeviceSize bufferSize = sizeof(MxcCameraUBO);
    createBuffer(pAppState, bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &pCameraState->uniformBuffer,
                 &pCameraState->uniformBufferMemory);
    vkMapMemory(pAppState->device, pCameraState->uniformBufferMemory, 0, bufferSize, 0,&pCameraState->pUniformBufferMapped);
}

void mxcInitCamera(MxcAppState *pAppState) {
    pAppState->pCameraState = malloc(sizeof(MxcCameraState));
    createUniformBuffers(pAppState, pAppState->pCameraState);
    vec3 pos = {0, 0, -1};
    glm_vec3_copy(pos, pAppState->pCameraState->transformState.pos);
    glm_mat4_identity(&pAppState->pCameraState->transformState.rot);
    mxcUpdateCameraUBO(pAppState->pCameraState);
}

void mxcCleanupCamera(MxcAppState *pAppState) {
    vkDestroyBuffer(pAppState->device, pAppState->pCameraState->uniformBuffer, NULL);
    vkFreeMemory(pAppState->device, pAppState->pCameraState->uniformBufferMemory, NULL);
    free(pAppState->pCameraState);
}