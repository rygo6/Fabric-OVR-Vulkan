#include "fbr_ipc_targets.h"
#include "fbr_ipc.h"
#include "fbr_framebuffer.h"
#include "fbr_camera.h"
#include "fbr_vulkan.h"

// todo can these be a macro somehow?
const int FbrIPCTargetParamSize[] = {
        sizeof(FbrIPCParamImportFrameBuffer),
        sizeof(FbrIPCParamImportCamera),
        sizeof(FbrIPCParamImportCamera),
};

int fbrIPCTargetParamSize(int target) {
    return FbrIPCTargetParamSize[target];
}

void fbrIPCSetTargets(FbrIPC *pIPC) {
    pIPC->pTargetFuncs[FBR_IPC_TARGET_IMPORT_FRAMEBUFFER] = (void (*)(FbrApp *, void *)) fbrIPCTargetImportFrameBuffer;
    pIPC->pTargetFuncs[FBR_IPC_TARGET_IMPORT_CAMERA] = (void (*)(FbrApp *, void *)) fbrIPCTargetImportCamera;
    pIPC->pTargetFuncs[FBR_IPC_TARGET_IMPORT_TIMELINE_SEMAPHORE] = (void (*)(FbrApp *, void *)) fbrIPCTargetImportTimelineSemaphore;
}