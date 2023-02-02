#ifndef FABRIC_IPC_TARGETS_H
#define FABRIC_IPC_TARGETS_H

#include "fbr_app.h"

typedef enum FbrIPCTargetType {
    FBR_IPC_TARGET_IMPORT_FRAMEBUFFER = 0,
    FBR_IPC_TARGET_IMPORT_CAMERA = 1,
} FbrIPCTargetType;

int fbrIPCTargetParamSize(int target);

void fbrIPCSetTargets(FbrIPC *pIPC);

#endif //FABRIC_IPC_TARGETS_H
