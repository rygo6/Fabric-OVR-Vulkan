#ifndef FABRIC_FBR_PROCESS_H
#define FABRIC_FBR_PROCESS_H

#include <windows.h>

typedef struct FbrProcess {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
} FbrProcess;

void fbrCreateProcess(FbrProcess **ppAllocProcess);

#endif //FABRIC_FBR_PROCESS_H
