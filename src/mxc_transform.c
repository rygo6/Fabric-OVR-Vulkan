//
// Created by rygo6 on 12/4/2022.
//

#include "mxc_transform.h"

void mxcInitTransform(MxcTransformState *transformState) {
    glm_vec3_zero(transformState->pos);
    glm_quat_identity(transformState->rot);
    glm_mat4_identity(transformState->matrix);
}

void mxcUpdateTransformMatrix(MxcTransformState *transformState){
    glm_quat_look(transformState->pos, transformState->rot, transformState->matrix);
}