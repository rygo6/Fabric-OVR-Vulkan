#ifndef FABRIC_FBR_PROCESS_H
#define FABRIC_FBR_PROCESS_H

#include "fbr_ipc.h"

#if WIN32
#include <windows.h>
#endif

typedef struct FbrProcess {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    FbrIPC *pProducerIPC;
//    FbrIPC *pReceiverIPC;    // todo create receiverIPC

} FbrProcess;

void fbrCreateProcess(FbrProcess **ppAllocProcess);

#endif //FABRIC_FBR_PROCESS_H
