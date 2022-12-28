#ifndef FABRIC_TEXTURE_H
#define FABRIC_TEXTURE_H

#include "fbr_app.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef X11
#include <X11/Xlib.h>
#endif

typedef struct FbrTexture {
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory deviceMemory;
    VkSampler sampler;
#ifdef WIN32
    HANDLE sharedMemory;
#endif
} FbrTexture;

void fbrCreateTexture(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, char const *filename, bool external);

void fbrCleanupTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture);

#endif //FABRIC_TEXTURE_H
