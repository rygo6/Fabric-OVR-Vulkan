#include "fbr_transform.h"

void fbrInitTransform(FbrTransform *pTransform) {
    glm_vec3_zero(pTransform->pos);
    glm_quat_identity(pTransform->rot);
    glm_mat4_identity(pTransform->uboData.model);
}

void fbrUpdateTransformMatrix(FbrTransform *pTransform) {
    glm_translate_to(GLM_MAT4_IDENTITY, pTransform->pos, pTransform->uboData.model);
    glm_quat_rotate(pTransform->uboData.model, pTransform->rot, pTransform->uboData.model);
    memcpy(pTransform->pUBO->pUniformBufferMapped, &pTransform->uboData, sizeof(FbrTransform));
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

VkResult fbrCreateTransform(const FbrVulkan *pVulkan, FbrTransform **ppAllocTransform) {
    *ppAllocTransform = calloc(1, sizeof(FbrTransform));
    FbrTransform *pTransform = *ppAllocTransform;

    FBR_ACK(fbrCreateUBO(pVulkan,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         sizeof(FbrTransform),
                         FBR_NO_DYNAMIC_BUFFER,
                         false,
                         &pTransform->pUBO));

    fbrInitTransform(pTransform);
}

VkResult fbrDestroyTransform(const FbrVulkan *pVulkan, FbrTransform *pTransform) {
    fbrDestroyUBO(pVulkan, pTransform->pUBO);
    free(pTransform);
}