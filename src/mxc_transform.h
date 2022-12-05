#ifndef MOXAIC_TRANSFORM_H
#define MOXAIC_TRANSFORM_H

#include "cglm/cglm.h"

typedef struct MxcComponent {
} MxcComponent;

typedef struct MxcTransformState {
    vec3 pos;
    versor rot;
    mat4 matrix;
} MxcTransformState;

typedef struct MxcEntityState {
    int entityId;
} MxcEntityState;

void mxcInitTransform(MxcTransformState *transformState);

void mxcUpdateTransformMatrix(MxcTransformState *transformState);

#endif //MOXAIC_TRANSFORM_H
