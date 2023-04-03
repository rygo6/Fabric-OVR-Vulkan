#include "fbr_camera.h"
#include "fbr_buffer.h"
#include "fbr_log.h"
#include "fbr_vulkan.h"

#include <windows.h>

void fbrUpdateCameraUBO(FbrCamera *pCamera, uint32_t dynamicIndex)
{
    uint32_t dynamicOffset = dynamicIndex * pCamera->pUBO->dynamicAlignment;
    memcpy(pCamera->pUBO->pUniformBufferMapped + dynamicOffset, &pCamera->uboData, sizeof(FbrCameraUBO));
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

            glm_quat_rotatev(pCamera->pTransform->rot, deltaPos, deltaPos);
            glm_vec3_add(pCamera->pTransform->pos, deltaPos, pCamera->pTransform->pos);

            fbrUpdateTransformMatrix(pCamera->pTransform);

            glm_mat4_copy(pCamera->pTransform->uboData.model, pCamera->uboData.trs);
            glm_quat_look(pCamera->pTransform->pos, pCamera->pTransform->rot, pCamera->uboData.view);
            break;
        }
        case FBR_MOUSE_POS_INPUT: {
            float yRot = (float) -pInputEvent->mousePosInput.xDelta / 10.0f;
            versor rotQ;
            glm_quatv(rotQ, glm_rad(yRot), GLM_YUP);
            glm_quat_mul(pCamera->pTransform->rot, rotQ, pCamera->pTransform->rot);

            fbrUpdateTransformMatrix(pCamera->pTransform);

            glm_mat4_copy(pCamera->pTransform->uboData.model, pCamera->uboData.trs);
            glm_quat_look(pCamera->pTransform->pos, pCamera->pTransform->rot, pCamera->uboData.view);
            break;
        }
        case FBR_MOUSE_BUTTON_INPUT: {

            break;
        }
    }
}

void fbrIPCTargetImportCamera(FbrApp *pApp, FbrIPCParamImportCamera *pParam) {
    FBR_LOG_DEBUG("Importing Camera.", pParam->handle);
    fbrImportCamera(pApp->pVulkan, 0, &pApp->pCamera,pParam->handle);
}

FBR_RESULT fbrImportCamera(const FbrVulkan *pVulkan,
                     uint32_t dynamicCount,
                     HANDLE externalMemory,
                     FbrCamera **ppAllocCameraState) {
    *ppAllocCameraState = calloc(1, sizeof(FbrCamera));
    FbrCamera *pCamera = *ppAllocCameraState;

    fbrCreateTransform(pVulkan, &pCamera->pTransform);
    glm_vec3_copy((vec3){0, 0, -1}, pCamera->pTransform->pos);
    glm_quatv(pCamera->pTransform->rot, glm_rad(-180), GLM_YUP);
    glm_perspective(90, 1, .01f, 10, pCamera->uboData.proj);
    fbrUpdateTransformMatrix(pCamera->pTransform);

    fbrImportUBO(pVulkan,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 sizeof(FbrCamera),
                 dynamicCount,
                 externalMemory,
                 &pCamera->pUBO);

    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateCamera(const FbrVulkan *pVulkan,
                           uint32_t dynamicCount,
                           FbrCamera **ppAllocCameraState) {
    *ppAllocCameraState = calloc(1, sizeof(FbrCamera));
    FbrCamera *pCamera = *ppAllocCameraState;

    fbrCreateTransform(pVulkan, &pCamera->pTransform);
    glm_vec3_copy((vec3){0, 0, -1}, pCamera->pTransform->pos);
    glm_quatv(pCamera->pTransform->rot, glm_rad(-180), GLM_YUP);
    glm_perspective(90, 1, .01f, 10, pCamera->uboData.proj);
    fbrUpdateTransformMatrix(pCamera->pTransform);

    FBR_ACK(fbrCreateUBO(pVulkan,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         sizeof(FbrCamera),
                         dynamicCount,
                         true,
                         &pCamera->pUBO));

    fbrUpdateCameraUBO(pCamera, 0);

    return FBR_SUCCESS;
}

void fbrDestroyCamera(const FbrVulkan *pVulkan, FbrCamera *pCameraState) {
    fbrDestroyUBO(pVulkan, pCameraState->pUBO);
    free(pCameraState);
}