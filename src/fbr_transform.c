#include "fbr_transform.h"

void fbrInitTransform(FbrTransform *transform) {
    glm_vec3_zero(transform->pos);
    glm_quat_identity(transform->rot);
    glm_mat4_identity(transform->matrix);
}

void fbrUpdateTransformMatrix(FbrTransform *transform) {
    glm_quat_look(transform->pos, transform->rot, transform->matrix);
}