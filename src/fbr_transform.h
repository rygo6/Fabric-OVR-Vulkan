#ifndef FABRIC_TRANSFORM_H
#define FABRIC_TRANSFORM_H

#include "fbr_app.h"
#include "fbr_buffer.h"
#include "fbr_vulkan.h"
#include "fbr_cglm.h"

typedef struct FbrComponent {
} FbrComponent;

typedef struct FbrTransformUBO {
    mat4 model;
} FbrTransformUBO;

typedef struct FbrTransform {
    vec3 pos;
    versor rot;
    FbrTransformUBO uboData;
    FbrUniformBufferObject *pUBO;
} FbrTransform;

typedef struct FbrEntity {
    int entityId;
} FbrEntity;

void fbrInitTransform(FbrTransform *pTransform); // should this be pointer? prolly

void fbrUpdateTransformUBO(FbrTransform *pTransform);

void fbrTransformUp(FbrTransform *pTransform, vec3 dest);

void fbrTransformRight(FbrTransform *pTransform, vec3 dest);

void fbrTransformForward(FbrTransform *pTransform, vec3 dest);

VkResult fbrCreateTransform(const FbrVulkan *pVulkan, FbrTransform **ppAllocTransform);

void fbrDestroyTransform(const FbrVulkan *pVulkan, FbrTransform *pTransform);

#endif //FABRIC_TRANSFORM_H
