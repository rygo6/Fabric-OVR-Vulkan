#include "mxc_camera.h"
#include "mxc_buffer.h"
#include "mxc_log.h"

#include <memory.h>

static inline void mxcUpdateCameraUBO(MxcCameraState *pCameraState) {
    glm_mat4_identity(pCameraState->mvp.model);

    mxcUpdateTransformMatrix(&pCameraState->transformState);
    glm_mat4_copy(pCameraState->transformState.matrix, pCameraState->mvp.view);

    glm_perspective(90, 1, .01f, 10, pCameraState->mvp.proj);

    memcpy(pCameraState->mvpUBO.pUniformBufferMapped, &pCameraState->mvp, sizeof(MxcMVP));
}

void mxcUpdateCamera(MxcCameraState *pCameraState, const MxcInputEvent *pInputEvent, const MxcTimeState *pTimeState) {
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

            mxcUpdateCameraUBO(pCameraState);
//            mxcLogDebugInfo3("MXC_KEY_INPUT", %f, deltaPos[0], %f, deltaPos[1], %f, deltaPos[2]);
            break;
        }
        case MXC_MOUSE_POS_INPUT: {
            float yRot = (float) -pInputEvent->mousePosInput.xDelta / 100.0f;
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

void mxcAllocCamera(const MxcAppState *pAppState, MxcCameraState **ppAllocCameraState) {
    *ppAllocCameraState = malloc(sizeof(MxcCameraState));
    MxcCameraState* pCameraState = *ppAllocCameraState;
    memset(pCameraState, 0, sizeof(MxcCameraState));

    mxcInitTransform(&pCameraState->transformState);
    vec3 pos = {0, 0, -1};
    glm_vec3_copy(pos, pCameraState->transformState.pos);

    createUniformBuffers(pAppState, &pAppState->pCameraState->mvpUBO, sizeof(MxcMVP));
    mxcUpdateCameraUBO(pAppState->pCameraState);
}

void mxcFreeCamera(const MxcAppState *pAppState, MxcCameraState *pCameraState) {
    mxcCleanupBuffers(pAppState, &pCameraState->mvpUBO);
    free(pCameraState);
}