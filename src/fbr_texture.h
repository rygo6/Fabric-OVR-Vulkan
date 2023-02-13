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
    VkExtent2D extent;
#ifdef WIN32
    HANDLE externalMemory;
#endif
} FbrTexture;

void fbrCreateTextureFromFile(const FbrVulkan *pVulkan, FbrTexture **ppAllocTexture, char const *filename, bool external);

void fbrImportTexture(const FbrVulkan *pVulkan,
                      FbrTexture **ppAllocTexture,
                      HANDLE externalMemory,
                      VkExtent2D extent,
                      VkImageUsageFlags usage,
                      VkFormat format);

void fbrCreateExternalTexture(const FbrVulkan *pVulkan,
                              FbrTexture **ppAllocTexture,
                              VkExtent2D extent,
                              VkImageUsageFlags usage,
                              VkFormat format);

void fbrDestroyTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture);

#endif //FABRIC_TEXTURE_H
