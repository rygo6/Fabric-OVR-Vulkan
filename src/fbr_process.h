#ifndef FABRIC_FBR_PROCESS_H
#define FABRIC_FBR_PROCESS_H

#if WIN32
#include <windows.h>
#endif

typedef struct FbrProcess {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
} FbrProcess;

void fbrCreateProcess(FbrProcess **ppAllocProcess, HANDLE textureSharedMemory, HANDLE cameraSharedMemory);

#endif //FABRIC_FBR_PROCESS_H
