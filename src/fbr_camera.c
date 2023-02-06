#include "fbr_camera.h"
#include "fbr_buffer.h"
#include "fbr_log.h"

#include <windows.h>

void fbrUpdateCameraUBO(FbrCamera *pCamera) {
    glm_mat4_copy(pCamera->transform.matrix, pCamera->uboData.view);
    memcpy(pCamera->ubo.pUniformBufferMapped, &pCamera->uboData, sizeof(FbrCameraUBO));
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
            float yRot = (float) -pInputEvent->mousePosInput.xDelta / 10.0f;
            versor rotQ;
            glm_quatv(rotQ, glm_rad(yRot), GLM_YUP);
            glm_quat_mul(pCamera->transform.rot, rotQ, pCamera->transform.rot);

            fbrUpdateTransformMatrix(&pCamera->transform);
            break;
        }
        case FBR_MOUSE_BUTTON_INPUT: {

            break;
        }
    }
}

void fbrIPCTargetImportCamera(FbrApp *pApp, FbrIPCParamImportCamera *pParam) {
    FBR_LOG_DEBUG("Importing Camera.", pParam->handle);
    fbrImportCamera(pApp->pVulkan, &pApp->pCamera,pParam->handle);
}

void fbrImportCamera(const FbrVulkan *pVulkan, FbrCamera **ppAllocCameraState, HANDLE externalMemory) {
    *ppAllocCameraState = calloc(1, sizeof(FbrCamera));
    FbrCamera *pCamera = *ppAllocCameraState;

    // don't need to set or map anything because parent does it!
//    fbrInitTransform(&pCamera->transform);
//    glm_vec3_copy((vec3){0, 0, -1}, pCamera->transform.pos);
//    glm_quatv(pCamera->transform.rot, glm_rad(-180), GLM_YUP);
//    glm_perspective(90, 1, .01f, 10, pCamera->proj);
//    fbrUpdateTransformMatrix(&pCamera->transform);

    fbrImportUniformBuffer(pVulkan, &pCamera->ubo, sizeof(FbrCameraUBO), externalMemory);

//    glm_perspective(90, 1, .01f, 10, pCamera->uboData.proj);
//    fbrUpdateCameraUBO(pCamera);
}

void fbrCreateCamera(const FbrVulkan *pVulkan, FbrCamera **ppAllocCameraState) {
    *ppAllocCameraState = calloc(1, sizeof(FbrCamera));
    FbrCamera *pCamera = *ppAllocCameraState;

    fbrInitTransform(&pCamera->transform);
    glm_vec3_copy((vec3){0, 0, -1}, pCamera->transform.pos);
    glm_quatv(pCamera->transform.rot, glm_rad(-180), GLM_YUP);
    glm_perspective(90, 1, .01f, 10, pCamera->proj);
    fbrUpdateTransformMatrix(&pCamera->transform);

//    fbrCreateUniformBuffer(pVulkan, &pCamera->ubo, sizeof(FbrCameraUBO));
    fbrCreateExternalUniformBuffer(pVulkan, &pCamera->ubo, sizeof(FbrCameraUBO));

    glm_perspective(90, 1, .01f, 10, pCamera->uboData.proj);
    fbrUpdateCameraUBO(pCamera);
}

void fbrCleanupCamera(const FbrVulkan *pVulkan, FbrCamera *pCameraState) {
    fbrCleanupUniformBuffer(pVulkan, &pCameraState->ubo);

    free(pCameraState);
}