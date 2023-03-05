#include "fbr_ipc_targets.h"
#include "fbr_ipc.h"
#include "fbr_framebuffer.h"
#include "fbr_camera.h"
#include "fbr_vulkan.h"
#include "fbr_node_parent.h"

// todo can these be a macro somehow?
const int FbrIPCTargetParamSize[] = {
        sizeof(FbrIPCParamImportFrameBuffer),
        sizeof(FbrIPCParamImportCamera),
        sizeof(FbrIPCParamImportTimelineSemaphore),
        sizeof(FbrIPCParamImportNodeParent),
};

int fbrIPCTargetParamSize(int target) {
    return FbrIPCTargetParamSize[target];
}

void fbrIPCSetTargets(FbrIPC *pIPC) {
    pIPC->pTargetFuncs[FBR_IPC_TARGET_IMPORT_FRAMEBUFFER] = (void (*)(FbrApp *, void *)) fbrIPCTargetImportFrameBuffer;
    pIPC->pTargetFuncs[FBR_IPC_TARGET_IMPORT_CAMERA] = (void (*)(FbrApp *, void *)) fbrIPCTargetImportCamera;
    pIPC->pTargetFuncs[FBR_IPC_TARGET_IMPORT_MAIN_SEMAPHORE] = (void (*)(FbrApp *, void *)) fbrIPCTargetImportMainSemaphore;
    // todo all above can delete?
    pIPC->pTargetFuncs[FBR_IPC_TARGET_IMPORT_NODE_PARENT] = (void (*)(FbrApp *, void *)) fbrIPCTargetImportNodeParent;
}