#ifndef FABRIC_TEXTURE_H
#define FABRIC_TEXTURE_H

#include "fbr_app.h"

typedef struct FbrTexture {
    VkImage texture;
    VkImageView textureView;
    VkDeviceMemory textureMemory;
    VkSampler textureSampler;
} FbrTexture;

void fbrCreateTexture(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture);

void fbrCleanupTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture);

#endif //FABRIC_TEXTURE_H
