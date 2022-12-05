//
// Created by rygo6 on 11/28/2022.
//

#ifndef MOXAIC_MXC_TRANSFORM_H
#define MOXAIC_MXC_TRANSFORM_H

#include "cglm/cglm.h"

typedef struct MxcTransformState {
    vec3 pos;
    versor rot;
    mat4 matrix;
} MxcTransformState;

void mxcInitTransform(MxcTransformState *transformState);

void mxcUpdateTransformMatrix(MxcTransformState *transformState);

#endif //MOXAIC_MXC_TRANSFORM_H
