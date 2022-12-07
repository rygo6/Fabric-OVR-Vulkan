#include "fbr_camera.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

#include <memory.h>

static inline void fbrUpdateCameraUBO(FbrCameraState *pCameraState) {
    fbrUpdateTransformMatrix(&pCameraState->transformState);
    glm_mat4_copy(pCameraState->transformState.matrix, pCameraState->mvp.view);

    glm_perspective(90, 1, .01f, 10, pCameraState->mvp.proj);

    memcpy(pCameraState->mvpUBO.pUniformBufferMapped, &pCameraState->mvp, sizeof(FbrMVP));
}

void fbrUpdateCamera(FbrCameraState *pCameraState, const FbrInputEvent *pInputEvent, const FbrTimeState *pTimeState) {
    switch (pInputEvent->type) {
        case MXC_NO_INPUT:
            break;
        case MXC_KEY_INPUT: {
            if (pInputEvent->keyInput.action != GLFW_PRESS && pInputEvent->keyInput.action != MXC_HELD)
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

            glm_quat_rotatev(pCameraState->transformState.rot, deltaPos, deltaPos);
            glm_vec3_add(pCameraState->transformState.pos, deltaPos, pCameraState->transformState.pos);

            fbrUpdateCameraUBO(pCameraState);
//            fbrLogDebugInfo3("MXC_KEY_INPUT", %f, deltaPos[0], %f, deltaPos[1], %f, deltaPos[2]);
            break;
        }
        case MXC_MOUSE_POS_INPUT: {
            float yRot = (float) -pInputEvent->mousePosInput.xDelta / 100.0f;
            versor rotQ;
            glm_quatv(rotQ, yRot, GLM_YUP);
            glm_quat_mul(pCameraState->transformState.rot, rotQ, pCameraState->transformState.rot);

            fbrUpdateCameraUBO(pCameraState);
            break;
        }
        case MXC_MOUSE_BUTTON_INPUT: {

            break;
        }
    }
}

void fbrAllocCamera(const FbrAppState *pAppState, FbrCameraState **ppAllocCameraState) {
    *ppAllocCameraState = malloc(sizeof(FbrCameraState));
    FbrCameraState* pCameraState = *ppAllocCameraState;
    memset(pCameraState, 0, sizeof(FbrCameraState));

    fbrInitTransform(&pCameraState->transformState);
    vec3 pos = {0, 0, -1};
    glm_vec3_copy(pos, pCameraState->transformState.pos);

    createUniformBuffers(pAppState, &pAppState->pCameraState->mvpUBO, sizeof(FbrMVP));
    fbrUpdateCameraUBO(pAppState->pCameraState);
}

void fbrFreeCamera(const FbrAppState *pAppState, FbrCameraState *pCameraState) {
    fbrCleanupBuffers(pAppState, &pCameraState->mvpUBO);
    free(pCameraState);
}