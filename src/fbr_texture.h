#ifndef FABRIC_TEXTURE_H
#define FABRIC_TEXTURE_H

#include "fbr_app.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef X11
#include <X11/Xlib.h>
#endif

#define FBR_DEFAULT_TEXTURE_FORMAT VK_FORMAT_R8G8B8A8_SRGB

typedef struct FbrTexture {
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory deviceMemory;
    int width;
    int height;
#ifdef WIN32
    HANDLE externalMemory;
#endif
} FbrTexture;

void fbrCreateTextureFromFile(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, char const *filename, bool external);

void fbrCreateTextureFromExternalMemory(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, HANDLE externalMemory, int width, int height);

void fbrCreateWriteFramebufferTextureFromExternalMemory(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, HANDLE externalMemory, int width, int height);

void fbrCreateReadFramebufferTexture(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, int width, int height);

void fbrDestroyTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture);

#endif //FABRIC_TEXTURE_H
