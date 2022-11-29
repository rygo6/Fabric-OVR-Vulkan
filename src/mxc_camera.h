//
// Created by rygo6 on 11/28/2022.
//

#ifndef MOXAIC_MXC_CAMERA_H
#define MOXAIC_MXC_CAMERA_H

#include "cglm/cglm.h"
#include "mxc_transform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CameraUniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} CameraUniformBufferObject;

typedef struct CameraState {
    TransformState transformState;
    CameraUniformBufferObject ubo;
} CameraState;

void mxcUpdateCameraUBO(CameraState *pCameraState);

void mxcInitCamera(CameraState *pCameraState);

#endif //MOXAIC_MXC_CAMERA_H
