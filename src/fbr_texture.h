#ifndef FABRIC_TEXTURE_H
#define FABRIC_TEXTURE_H

#include "fbr_app.h"

#include <windows.h>

typedef struct FbrTexture {
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory deviceMemory;
    VkSampler sampler;
    HANDLE sharedMemory;
} FbrTexture;

void fbrCreateTexture(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, char const *filename, bool external);

void fbrCleanupTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture);

#endif //FABRIC_TEXTURE_H
