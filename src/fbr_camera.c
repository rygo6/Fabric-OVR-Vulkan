#include "fbr_camera.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

#include <memory.h>

void fbrUpdateCameraUBO(FbrCamera *pCamera) {
    glm_mat4_copy(pCamera->transform.matrix, pCamera->gpuData.view);
    memcpy(pCamera->gpuUBO.pUniformBufferMapped, &pCamera->gpuData, sizeof(FbrCameraGpuData));
}

void fbrUpdateCamera(FbrCamera *pCamera, const FbrInputEvent *pInputEvent, const FbrTime *pTimeState) {
    switch (pInputEvent->type) {
        case FBR_NO_INPUT:
            break;
        case FBR_KEY_INPUT: {
            if (pInputEvent->keyInput.action != GLFW_PRESS && pInputEvent->keyInput.action != FBR_HELD)
                break;

            const float moveAmount = (float) pTimeState->deltaTime;
            vec3 deltaPos = GLM_VEC3_ZERO_INIT;
            if (pInputEvent->keyInput.key == GLFW_KEY_W) {
                deltaPos[2] = -moveAmount;
            } else if (pInputEvent->keyInput.key == GLFW_KEY_S) {
                deltaPos[2] = moveAmount;
            } else if (pInputEvent->keyInput.key == GLFW_KEY_D) {
                deltaPos[0] = moveAmount;
            } else if (pInputEvent->keyInput.key == GLFW_KEY_A) {
                deltaPos[0] = -moveAmount;
            }

            glm_quat_rotatev(pCamera->transform.rot, deltaPos, deltaPos);
            glm_vec3_add(pCamera->transform.pos, deltaPos, pCamera->transform.pos);

            fbrUpdateTransformMatrix(&pCamera->transform);
            break;
        }
        case FBR_MOUSE_POS_INPUT: {
            float yRot = (float) -pInputEvent->mousePosInput.xDelta / 100.0f;
            versor rotQ;
            glm_quatv(rotQ, yRot, GLM_YUP);
            glm_quat_mul(pCamera->transform.rot, rotQ, pCamera->transform.rot);

            fbrUpdateTransformMatrix(&pCamera->transform);
            break;
        }
        case FBR_MOUSE_BUTTON_INPUT: {

            break;
        }
    }
}

void fbrInitCamera(const FbrVulkan *pVulkan, FbrCamera *pCamera) {
    fbrInitTransform(&pCamera->transform);
    vec3 pos = {0, 0, -1};
    glm_vec3_copy(pos, pCamera->transform.pos);
    glm_perspective(90, 1, .01f, 10, pCamera->proj);
    fbrUpdateTransformMatrix(&pCamera->transform);

    fbrCreateUniformBuffers(pVulkan, &pCamera->gpuUBO, sizeof(FbrCameraGpuData));
    glm_perspective(90, 1, .01f, 10, pCamera->gpuData.proj);
    fbrUpdateCameraUBO(pCamera);
}

void fbrCreateCamera(const FbrVulkan *pVulkan, FbrCamera **ppAllocCameraState) {
    *ppAllocCameraState = calloc(1, sizeof(FbrCamera));
    FbrCamera *pCameraState = *ppAllocCameraState;
    fbrInitCamera(pVulkan, pCameraState);
}

void fbrCleanupCamera(const FbrVulkan *pVulkan, FbrCamera *pCameraState) {
    fbrCleanupUniformBuffers(pVulkan, &pCameraState->gpuUBO);
    free(pCameraState);
}