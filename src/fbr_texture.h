#ifndef FABRIC_TEXTURE_H
#define FABRIC_TEXTURE_H

#include "fbr_app.h"

typedef struct FbrTexture {
    VkImage texture;
    VkImageView textureView;
    VkDeviceMemory textureMemory;
    VkSampler textureSampler;
} FbrTexture;

void fbrCreateTexture(const FbrApp *pApp, FbrTexture **ppAllocTexture);

void fbrCleanupTexture(const FbrApp *pApp, FbrTexture *pTexture);

#endif //FABRIC_TEXTURE_H
