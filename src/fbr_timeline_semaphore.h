#ifndef FABRIC_TIMELINE_SEMAPHORE_H
#define FABRIC_TIMELINE_SEMAPHORE_H

#include "fbr_app.h"

#ifdef WIN32
#include "windows.h"
#endif

typedef struct FbrTimelineSemaphore {
    uint64_t waitValue;
    VkSemaphore semaphore;
    HANDLE externalHandle;
} FbrTimelineSemaphore;

VkResult fbrCreateTimelineSemaphore(const FbrVulkan *pVulkan, bool external, bool readOnly, FbrTimelineSemaphore **ppAllocTimelineSemaphore);

VkResult fbrImportTimelineSemaphore(const FbrVulkan *pVulkan, bool readOnly, HANDLE externalTimelineSemaphore, FbrTimelineSemaphore **ppAllocTimelineSemaphore);

void fbrDestroyTimelineSemaphore(const FbrVulkan *pVulkan, FbrTimelineSemaphore *pTimelineSemaphore);

#endif //FABRIC_TIMELINE_SEMAPHORE_H
