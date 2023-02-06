#include "fbr_node.h"
#include "fbr_log.h"

void fbrCreateNode(const FbrApp *pApp, const char *pName, FbrNode **ppAllocNode) {
    *ppAllocNode = calloc(1, sizeof(FbrNode));
    FbrNode *pNode = *ppAllocNode;
    pNode->pName = strdup(pName);

    fbrCreateProcess(&pNode->pProcess);

    if (fbrCreateProducerIPC(&pNode->pProducerIPC) != 0){
        FBR_LOG_ERROR("fbrCreateProducerIPC fail");
        return;
    }
    // todo create receiverIPC

    fbrCreateExternalFrameBuffer(pApp->pVulkan, &pNode->pFramebuffer);
}

void fbrDestroyNode(const FbrApp *pApp, FbrNode *pNode) {
    free(pNode->pName);

    fbrDestroyProcess(pNode->pProcess);

    fbrDestroyIPC(pNode->pProducerIPC);
    fbrDestroyIPC(pNode->pReceiverIPC);

    fbrDestroyFrameBuffer(pApp->pVulkan, pNode->pFramebuffer);

    free(pNode);
}
