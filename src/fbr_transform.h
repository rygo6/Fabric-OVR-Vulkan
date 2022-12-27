#ifndef FABRIC_TRANSFORM_H
#define FABRIC_TRANSFORM_H

#include "fbr_app.h"

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

void fbrInitTransform(FbrTransform *transform);

void fbrUpdateTransformMatrix(FbrTransform *transform);

#endif //FABRIC_TRANSFORM_H
