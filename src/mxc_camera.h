//
// Created by rygo6 on 11/28/2022.
//

#ifndef MOXAIC_MXC_CAMERA_H
#define MOXAIC_MXC_CAMERA_H

#include "mxc_app.h"
#include "mxc_input.h"
#include "mxc_transform.h"

typedef struct mxcCameraUBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} mxcCameraUBO;

typedef struct mxcCameraState {
    mxcTransformState transformState;
    mxcCameraUBO ubo;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* pUniformBufferMapped;
} mxcCameraState;

void mxcUpdateCamera(mxcCameraState *pCameraState, mxcInputEvent inputEvent);

void mxcInitCamera(mxcAppState* pAppState);

void mxcCleanupCamera(mxcAppState* pAppState);

#endif //MOXAIC_MXC_CAMERA_H
