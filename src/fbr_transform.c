#include "fbr_transform.h"

void fbrInitTransform(FbrTransform *transform) {
    glm_vec3_zero(transform->pos);
    glm_quat_identity(transform->rot);
    glm_mat4_identity(transform->matrix);
}

void fbrUpdateTransformMatrix(FbrTransform *transform) {
    mat4 newTransform = GLM_MAT4_IDENTITY_INIT;
    glm_translate_to(newTransform, transform->pos, newTransform);
    glm_quat_rotate(newTransform, transform->rot, newTransform);
    glm_mat4_copy(newTransform, transform->matrix);
}

void fbrTransformUp(FbrTransform *pTransform, vec3 dest) {
    dest = ((vec3) {0.0f, 1.0f, 0.0f});
    glm_quat_rotatev(pTransform->rot, dest, dest);
}

void fbrTransformRight(FbrTransform *pTransform, vec3 dest) {
    dest = ((vec3) {1.0f, 0.0f, 0.0f});
    glm_quat_rotatev(pTransform->rot, dest, dest);
}

void fbrTransformForward(FbrTransform *pTransform, vec3 dest) {
    dest = ((vec3) {0.0f, 0.0f, 1.0f});
    glm_quat_rotatev(pTransform->rot, dest, dest);
}