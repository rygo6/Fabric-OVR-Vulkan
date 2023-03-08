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
    FbrUniformBufferObject *pUBO;
} FbrCamera;

void fbrUpdateCameraUBO(FbrCamera *pCamera);

void fbrUpdateCamera(FbrCamera *pCamera, const FbrInputEvent *pInputEvent, const FbrTime *pTimeState);

void fbrImportCamera(const FbrVulkan *pVulkan, FbrCamera **ppAllocCameraState, HANDLE externalMemory);

VkResult fbrCreateCamera(const FbrVulkan *pVulkan, FbrCamera **ppAllocCameraState);

void fbrDestroyCamera(const FbrVulkan *pVulkan, FbrCamera *pCameraState);

// IPC

typedef struct FbrIPCParamImportCamera {
    HANDLE handle;
} FbrIPCParamImportCamera;

void fbrIPCTargetImportCamera(FbrApp *pApp, FbrIPCParamImportCamera *pParam);

#endif //FABRIC_CAMERA_H
