#ifndef FABRIC_IPC_H
#define FABRIC_IPC_H

#include <windows.h>

typedef struct FbrIPC {
    HANDLE hMapFile;
    LPCTSTR pBuf;
} FbrIPC;

int fbrCreateProducerIPC(FbrIPC **ppAllocIPC);

int fbrCreateReceiverIPC(FbrIPC **ppAllocIPC);

int fbrDestroyIPC(FbrIPC *pIPC);

#endif //FABRIC_IPC_H
