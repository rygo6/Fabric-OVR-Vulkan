#ifndef FABRIC_NODE_H
#define FABRIC_NODE_H

#include "fbr_process.h"
#include "fbr_framebuffer.h"
#include "fbr_ipc.h"

typedef struct FbrNode {
    char *pName;

    FbrProcess *pProcess;

    FbrFramebuffer *pFramebuffer;

    FbrIPC *pProducerIPC;
    FbrIPC *pReceiverIPC;

} FbrNode;

void fbrCreateNode(const FbrApp *pApp, const char *pName, FbrNode **ppAllocNode);

void fbrDestroyNode(const FbrApp *pApp, FbrNode *pNode);

#endif //FABRIC_NODE_H
