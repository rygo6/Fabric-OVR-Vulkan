#include "fbr_transform.h"

void fbrInitTransform(FbrTransform *transform) {
    glm_vec3_zero(transform->pos);
    glm_quat_identity(transform->rot);
    glm_mat4_identity(transform->matrix);
}

void fbrUpdateTransformMatrix(FbrTransform *transform) {
    mat4 newTransform;
    glm_translate_make(newTransform, transform->pos);
    glm_quat_rotate(newTransform, transform->rot, newTransform);
    glm_mat4_copy(newTransform, transform->matrix);
}