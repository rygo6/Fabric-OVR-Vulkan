#ifndef FABRIC_CAMERA_H
#define FABRIC_CAMERA_H

#include "fbr_app.h"
#include "fbr_input.h"
#include "fbr_transform.h"
#include "fbr_buffer.h"

typedef struct FbrCameraUBO {
    mat4 view;
    mat4 proj;
} FbrCameraUBO;

typedef struct FbrCamera {
    FbrEntity entity;
    FbrTransform transform;
    mat4 proj;

    FbrCameraUBO uboData;
    UniformBufferObject ubo;
} FbrCamera;

void fbrUpdateCameraUBO(FbrCamera *pCamera);

void fbrUpdateCamera(FbrCamera *pCamera, const FbrInputEvent *pInputEvent, const FbrTime *pTimeState);

void fbrCreateCamera(const FbrVulkan *pVulkan, FbrCamera **ppAllocCameraState);

void fbrCleanupCamera(const FbrVulkan *pVulkan, FbrCamera *pCameraState);

#endif //FABRIC_CAMERA_H
