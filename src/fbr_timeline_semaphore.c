#include "fbr_timeline_semaphore.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"

#ifdef WIN32
#include <vulkan/vulkan_win32.h>
#define FBR_EXTERNAL_SEMAPHORE_HANDLE_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
#endif

//static VkResult checkForSupport(){
    // TODO validate external timeline semaphore
//    const VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
//            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
//            .pNext = NULL,
//            .initialValue = 0,
//            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
//    };
//    const VkPhysicalDeviceExternalSemaphoreInfo externalSemaphoreInfo = {
//            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO,
//            .pNext = &semaphoreTypeCreateInfo,
//            .handleType = externalHandleType,
//    };
//    VkExternalSemaphoreProperties externalSemaphoreProperties = {
//            .sType = VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES,
//    };
//    vkGetPhysicalDeviceExternalSemaphoreProperties(pVulkan->physicalDevice, &externalSemaphoreInfo, &externalSemaphoreProperties);
//    if ((externalSemaphoreProperties.exportFromImportedHandleTypes & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT) == 0) {
//        FBR_LOG_ERROR("exportFromImportedHandleTypes Does not supported VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT!");
//    }
//    if ((externalSemaphoreProperties.compatibleHandleTypes & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT) == 0) {
//        FBR_LOG_ERROR("compatibleHandleTypes Does not supported VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT!");
//    }
//    if ((externalSemaphoreProperties.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT) == 0) {
//        FBR_LOG_ERROR("externalSemaphoreFeatures Does not supported VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT!");
//    }
//    if ((externalSemaphoreProperties.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT) == 0) {
//        FBR_LOG_ERROR("externalSemaphoreFeatures Does not supported VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT!");
//    }
//}

static VkResult createExternalTimelineSemaphore(const FbrVulkan *pVulkan, bool external, bool readOnly, FbrTimelineSemaphore *pTimelineSemaphore) {
    const VkExportSemaphoreWin32HandleInfoKHR exportSemaphoreWin32HandleInfo = {
            .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR,
            .pNext = NULL,
            // TODO are these the best security options? Read seems to affect it and solves issue of
            // child corrupting semaphore on crash... but not 100%
            .dwAccess = readOnly ? GENERIC_READ : GENERIC_ALL,
            .pAttributes = NULL,
//            .name = L"FBR_SEMAPHORE"
    };
    const VkExportSemaphoreCreateInfo exportSemaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
            .pNext = external ? &exportSemaphoreWin32HandleInfo : NULL,
            .handleTypes = FBR_EXTERNAL_SEMAPHORE_HANDLE_TYPE,
    };
    const VkSemaphoreTypeCreateInfo timelineSemaphoreTypeCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = &exportSemaphoreCreateInfo,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = 0,
    };
    const VkSemaphoreCreateInfo timelineSemaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &timelineSemaphoreTypeCreateInfo,
            .flags = 0,
    };
    VK_CHECK(vkCreateSemaphore(pVulkan->device, &timelineSemaphoreCreateInfo, NULL, &pTimelineSemaphore->semaphore));

    return VK_SUCCESS;
}

static VkResult getWin32Handle(const FbrVulkan *pVulkan, FbrTimelineSemaphore *pTimelineSemaphore) {
#if WIN32
    const VkSemaphoreGetWin32HandleInfoKHR semaphoreGetWin32HandleInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR,
            .pNext = NULL,
            .handleType = FBR_EXTERNAL_SEMAPHORE_HANDLE_TYPE,
            .semaphore = pTimelineSemaphore->semaphore,
    };

    // TODO move to preloaded refs
    const PFN_vkGetSemaphoreWin32HandleKHR getSemaphoreWin32HandleFunc = (PFN_vkGetSemaphoreWin32HandleKHR) vkGetInstanceProcAddr(pVulkan->instance, "vkGetSemaphoreWin32HandleKHR");
    if (getSemaphoreWin32HandleFunc == NULL) {
        FBR_LOG_DEBUG("Failed to get PFN_vkGetMemoryWin32HandleKHR!");
    }
    VK_CHECK(getSemaphoreWin32HandleFunc(pVulkan->device, &semaphoreGetWin32HandleInfo, &pTimelineSemaphore->externalHandle));
#endif

    return VK_SUCCESS;
}

static VkResult importTimelineSemaphore(const FbrVulkan *pVulkan, HANDLE externalTimelineSemaphore, FbrTimelineSemaphore *pTimelineSemaphore) {
#if WIN32
    const VkImportSemaphoreWin32HandleInfoKHR importSemaphoreWin32HandleInfoKhr = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR,
            .pNext = NULL,
            .handle = externalTimelineSemaphore,
            .handleType = FBR_EXTERNAL_SEMAPHORE_HANDLE_TYPE,
            .semaphore = pTimelineSemaphore->semaphore,
    };
    // TODO move to preloaded refs
    PFN_vkImportSemaphoreWin32HandleKHR importSemaphoreWin32HandleFunc = (PFN_vkImportSemaphoreWin32HandleKHR) vkGetInstanceProcAddr(pVulkan->instance, "vkImportSemaphoreWin32HandleKHR");
    if (importSemaphoreWin32HandleFunc == NULL) {
        FBR_LOG_DEBUG("Failed to get PFN_vkGetMemoryWin32HandleKHR!");
    }
    VK_CHECK(importSemaphoreWin32HandleFunc(pVulkan->device, &importSemaphoreWin32HandleInfoKhr));
#endif

    pTimelineSemaphore->externalHandle = externalTimelineSemaphore;

    return VK_SUCCESS;
}

VkResult fbrCreateTimelineSemaphore(const FbrVulkan *pVulkan, bool external, bool readOnly, FbrTimelineSemaphore **ppAllocTimelineSemaphore) {
    *ppAllocTimelineSemaphore = calloc(1, sizeof(FbrTimelineSemaphore));
    FbrTimelineSemaphore *pTimelineSemaphore = *ppAllocTimelineSemaphore;

    VK_CHECK(createExternalTimelineSemaphore(pVulkan, external, readOnly, pTimelineSemaphore));
    if (external) {
        VK_CHECK(getWin32Handle(pVulkan, pTimelineSemaphore));
    }
}

VkResult fbrImportTimelineSemaphore(const FbrVulkan *pVulkan, bool readOnly, HANDLE externalTimelineSemaphore, FbrTimelineSemaphore **ppAllocTimelineSemaphore) {
    *ppAllocTimelineSemaphore = calloc(1, sizeof(FbrTimelineSemaphore));
    FbrTimelineSemaphore *pTimelineSemaphore = *ppAllocTimelineSemaphore;

    VK_CHECK(createExternalTimelineSemaphore(pVulkan, true, readOnly, pTimelineSemaphore));
    VK_CHECK(importTimelineSemaphore(pVulkan, externalTimelineSemaphore, pTimelineSemaphore));
}

void fbrDestroyTimelineSemaphore(const FbrVulkan *pVulkan, FbrTimelineSemaphore *pTimelineSemaphore) {
    if (pTimelineSemaphore->externalHandle != NULL)
        CloseHandle(pTimelineSemaphore->externalHandle);

    vkDestroySemaphore(pVulkan->device, pTimelineSemaphore->semaphore, NULL);
    free(pTimelineSemaphore);
}