#ifndef FABRIC_CAMERA_H
#define FABRIC_CAMERA_H

#include "fbr_app.h"
#include "fbr_input.h"
#include "fbr_transform.h"
#include "fbr_buffer.h"

#define FBR_CAMERA_NEAR_DEPTH 0.0001f
#define FBR_CAMERA_FAR_DEPTH 100.0f
#define FBR_CAMERA_FOV 45

typedef struct FbrCameraBuffer {
    mat4 view;
    mat4 proj;
    mat4 trs; // do I need this if the camera has a transform struct!?
} FbrCameraBuffer;

typedef struct FbrCamera {
    FbrTransform *pTransform;
    FbrCameraBuffer bufferData;
    FbrUniformBufferObject *pUBO;
} FbrCamera;

void fbrUpdateCameraUBO(FbrCamera *pCamera);

void fbrUpdateCamera(FbrCamera *pCamera,
                     const FbrInputEvent *pInputEvent,
                     const FbrTime *pTimeState);

FBR_RESULT fbrImportCamera(const FbrVulkan *pVulkan,
                           HANDLE externalMemory,
                           FbrCamera **ppAllocCameraState);

FBR_RESULT fbrCreateCamera(const FbrVulkan *pVulkan,
                           FbrCamera **ppAllocCameraState);

void fbrDestroyCamera(const FbrVulkan *pVulkan, FbrCamera *pCameraState);

// IPC

typedef struct FbrIPCParamImportCamera {
    HANDLE handle;
} FbrIPCParamImportCamera;

void fbrIPCTargetImportCamera(FbrApp *pApp, FbrIPCParamImportCamera *pParam);

#endif //FABRIC_CAMERA_H
