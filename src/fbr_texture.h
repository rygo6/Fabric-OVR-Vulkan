#ifndef FABRIC_TEXTURE_H
#define FABRIC_TEXTURE_H

#include "fbr_app.h"

typedef struct FbrTexture {
    VkImage image;
    VkDeviceMemory imageMemory;
} FbrTexture;

void fbrCreateTexture(FBR_APP_PARAM, FbrTexture** ppAllocTexture);
void fbrCleanupTexture(FBR_APP_PARAM, FbrTexture* pTexture);

#endif //FABRIC_TEXTURE_H
