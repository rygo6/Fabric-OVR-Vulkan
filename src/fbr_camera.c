#include "fbr_camera.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

#include <memory.h>

static inline void fbrUpdateCameraUBO(FbrCamera *pCameraState) {
    fbrUpdateTransformMatrix(&pCameraState->transform);
    glm_mat4_copy(pCameraState->transform.matrix, pCameraState->mvp.view);
    glm_perspective(90, 1, .01f, 10, pCameraState->mvp.proj);
    // TODO this is getting copied multiple places.. in fbr_mesh.c too
    memcpy(pCameraState->mvpUBO.pUniformBufferMapped, &pCameraState->mvp, sizeof(FbrMVP));
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

            fbrUpdateCameraUBO(pCamera);
//            fbrLogDebugInfo3("FBR_KEY_INPUT", %f, deltaPos[0], %f, deltaPos[1], %f, deltaPos[2]);
            break;
        }
        case FBR_MOUSE_POS_INPUT: {
            float yRot = (float) -pInputEvent->mousePosInput.xDelta / 100.0f;
            versor rotQ;
            glm_quatv(rotQ, yRot, GLM_YUP);
            glm_quat_mul(pCamera->transform.rot, rotQ, pCamera->transform.rot);

            fbrUpdateCameraUBO(pCamera);
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
    fbrCreateUniformBuffers(pVulkan, &pCamera->mvpUBO, sizeof(FbrMVP));
    fbrUpdateCameraUBO(pCamera);
}

void fbrCreateCamera(const FbrVulkan *pVulkan, FbrCamera **ppAllocCameraState) {
    *ppAllocCameraState = calloc(1, sizeof(FbrCamera));
    FbrCamera *pCameraState = *ppAllocCameraState;
    fbrInitCamera(pVulkan, pCameraState);
}

void fbrCleanupCamera(const FbrVulkan *pVulkan, FbrCamera *pCameraState) {
    fbrCleanupUniformBuffers(pVulkan, &pCameraState->mvpUBO);
    free(pCameraState);
}