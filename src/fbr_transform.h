#ifndef FABRIC_TRANSFORM_H
#define FABRIC_TRANSFORM_H

#include "fbr_app.h"

#include <cglm/cglm.h>

typedef struct FbrComponent {
} FbrComponent;

typedef struct FbrTransform {
    vec3 pos;
    versor rot;
    mat4 matrix;
} FbrTransform;

typedef struct FbrEntity {
    int entityId;
} FbrEntity;

void fbrInitTransform(FbrTransform *transform); // should this be pointer? prolly

void fbrUpdateTransformMatrix(FbrTransform *transform);

void fbrTransformUp(FbrTransform *pTransform, vec3 dest);

void fbrTransformRight(FbrTransform *pTransform, vec3 dest);

void fbrTransformForward(FbrTransform *pTransform, vec3 dest);

#endif //FABRIC_TRANSFORM_H
