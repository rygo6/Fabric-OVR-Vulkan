#ifndef FABRIC_TRANSFORM_H
#define FABRIC_TRANSFORM_H

#include "cglm/cglm.h"

typedef struct FbrComponent {
} FbrComponent;

typedef struct FbrTransformState {
    vec3 pos;
    versor rot;
    mat4 matrix;
} FbrTransformState;

typedef struct FbrEntityState {
    int entityId;
} FbrEntityState;

void fbrInitTransform(FbrTransformState *transformState);

void fbrUpdateTransformMatrix(FbrTransformState *transformState);

#endif //FABRIC_TRANSFORM_H
