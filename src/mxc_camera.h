//
// Created by rygo6 on 11/28/2022.
//

#ifndef MOXAIC_MXC_CAMERA_H
#define MOXAIC_MXC_CAMERA_H

#include "mxc_app.h"
#include "mxc_input.h"
#include "mxc_transform.h"
#include "mxc_buffer.h"

typedef struct MxcMVP {
    mat4 model;
    mat4 view;
    mat4 proj;
} MxcMVP;

typedef struct MxcCameraState {
    MxcTransformState transformState;
    MxcMVP mvp;
    UniformBufferObject mvpUBO;
} MxcCameraState;

void mxcUpdateCamera(MxcCameraState *pCameraState, MxcInputEvent inputEvent, const MxcTimeState *pTimeState);

void mxcInitCamera(MxcAppState* pAppState);

void mxcCleanupCamera(MxcAppState *pAppState, MxcCameraState *pCameraState);

#endif //MOXAIC_MXC_CAMERA_H
