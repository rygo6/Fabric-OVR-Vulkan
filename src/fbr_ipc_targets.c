#include "fbr_ipc_targets.h"
#include "fbr_ipc.h"
#include "fbr_framebuffer.h"
#include "fbr_camera.h"

const int FbrIPCTargetParamSize[] = {
        sizeof(FbrIPCParamImportFrameBuffer),
        sizeof(FbrIPCParamImportCamera),
};

int fbrIPCTargetParamSize(int target) {
    return FbrIPCTargetParamSize[target];
}

void fbrIPCSetTargets(FbrIPC *pIPC) {
    pIPC->pTargetFuncs[FBR_IPC_TARGET_IMPORT_FRAMEBUFFER] = (void (*)(FbrApp *, void *)) fbrIPCTargetImportFrameBuffer;
    pIPC->pTargetFuncs[FBR_IPC_TARGET_IMPORT_CAMERA] = (void (*)(FbrApp *, void *)) fbrIPCTargetImportCamera;
}