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

typedef struct FbrCamera {
    FbrEntity entity;
    FbrTransform transform;
    FbrMVP mvp;
    UniformBufferObject mvpUBO;
} FbrCamera;

void fbrUpdateCamera(FbrCamera *pCamera, const FbrInputEvent *pInputEvent, const FbrTime *pTimeState);

void fbrCreateCamera(const FbrVulkan *pVulkan, FbrCamera **ppAllocCameraState);

void fbrCleanupCamera(const FbrVulkan *pVulkan, FbrCamera *pCameraState);

#endif //FABRIC_CAMERA_H
