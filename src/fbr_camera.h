#ifndef FABRIC_CAMERA_H
#define FABRIC_CAMERA_H

#include "fbr_app.h"
#include "fbr_input.h"
#include "fbr_transform.h"
#include "fbr_buffer.h"

#define FBR_DYNAMIC_MAIN_CAMERA_COUNT 64
#define FBR_CAMERA_NEAR_DEPTH 0.01f
#define FBR_CAMERA_FAR_DEPTH 10000.0f

typedef struct FbrCameraUBO {
    mat4 view;
    mat4 proj;
    mat4 trs;
} FbrCameraUBO;

typedef struct FbrCamera {
    FbrTransform *pTransform;
    FbrCameraUBO uboData;
    FbrUniformBufferObject *pUBO;
} FbrCamera;

void fbrUpdateCameraUBO(FbrCamera *pCamera, uint32_t dynamicOffset);

void fbrUpdateCamera(FbrCamera *pCamera,
                     const FbrInputEvent *pInputEvent,
                     const FbrTime *pTimeState);

FBR_RESULT fbrImportCamera(const FbrVulkan *pVulkan,
                           uint32_t dynamicCount,
                           HANDLE externalMemory,
                           FbrCamera **ppAllocCameraState);

FBR_RESULT fbrCreateCamera(const FbrVulkan *pVulkan,
                           uint32_t dynamicCount,
                           FbrCamera **ppAllocCameraState);

void fbrDestroyCamera(const FbrVulkan *pVulkan, FbrCamera *pCameraState);

// IPC

typedef struct FbrIPCParamImportCamera {
    HANDLE handle;
} FbrIPCParamImportCamera;

void fbrIPCTargetImportCamera(FbrApp *pApp, FbrIPCParamImportCamera *pParam);

#endif //FABRIC_CAMERA_H
