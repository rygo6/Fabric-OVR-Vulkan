#ifndef FABRIC_CAMERA_H
#define FABRIC_CAMERA_H

#include "fbr_app.h"
#include "fbr_input.h"
#include "fbr_transform.h"
#include "fbr_buffer.h"

typedef struct FbrMVP {
    mat4 model;
    mat4 view;
    mat4 proj;
} FbrMVP;

typedef struct FbrCameraState {
    FbrEntityState entityState;
    FbrTransformState transformState;
    FbrMVP mvp;
    UniformBufferObject mvpUBO;
} FbrCameraState;

void fbrUpdateCamera(FbrCameraState *pCameraState, const FbrInputEvent *pInputEvent, const FbrTimeState *pTimeState);

void fbrCreateCamera(const FbrAppState* pAppState, FbrCameraState **ppAllocCameraState);

void fbrFreeCamera(const FbrAppState *pAppState, FbrCameraState *pCameraState);

#endif //FABRIC_CAMERA_H
