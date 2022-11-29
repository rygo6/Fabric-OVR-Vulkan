#include "mxc_camera.h"

void mxcUpdateCameraUBO(CameraState *pCameraState) {
    glm_mat4_identity(pCameraState->ubo.model);
    glm_quat_look(pCameraState->transformState.pos, pCameraState->transformState.rot, pCameraState->ubo.view);
    glm_perspective(90, 1, .01f, 10, pCameraState->ubo.proj);
}

void mxcInitCamera(CameraState *pCameraState) {
    vec3 pos = {0,0,-1};
    glm_vec3_copy(pos, pCameraState->transformState.pos);
    glm_mat4_identity(&pCameraState->transformState.rot);
    mxcUpdateCameraUBO(pCameraState);
}