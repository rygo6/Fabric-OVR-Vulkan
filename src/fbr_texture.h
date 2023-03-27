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

void fbrCreateTextureFromImage(const FbrVulkan *pVulkan,
                               VkFormat format,
                               VkExtent2D extent,
                               VkImage image,
                               FbrTexture **ppAllocTexture);

void fbrCreateTextureFromFile(const FbrVulkan *pVulkan,
                              bool external,
                              char const *filename,
                              FbrTexture **ppAllocTexture);

void fbrImportTexture(const FbrVulkan *pVulkan,
                      VkFormat format,
                      VkExtent2D extent,
                      VkImageUsageFlags usage,
                      HANDLE externalMemory,
                      FbrTexture **ppAllocTexture);

void fbrCreateTexture(const FbrVulkan *pVulkan,
                      VkFormat format,
                      VkExtent2D extent,
                      VkImageUsageFlags usage,
                      VkImageAspectFlags aspectMask,
                      bool external,
                      FbrTexture **ppAllocTexture);

void fbrDestroyTexture(const FbrVulkan *pVulkan, FbrTexture *pTexture);

#endif //FABRIC_TEXTURE_H
