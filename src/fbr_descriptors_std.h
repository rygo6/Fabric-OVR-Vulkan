#ifndef FABRIC_DESCRIPTOR_SET_H
#define FABRIC_DESCRIPTOR_SET_H

#include "fbr_app.h"
#include <vulkan/vulkan.h>

typedef VkDescriptorSetLayout FbrSetLayout_VertUBO_FragSampler;
typedef VkDescriptorSet FbrDescriptorSet_CameraUBO_TextureSampler;

typedef struct FbrDescriptors_Std {
    FbrSetLayout_VertUBO_FragSampler dsl_VertUBO_FragSampler;
} FbrDescriptors_Std;

VkResult fbrCreateDescriptorSet_CameraUBO_TextureSampler(const FbrVulkan *pVulkan,
                                                         FbrSetLayout_VertUBO_FragSampler setLayout,
                                                         const FbrCamera *pCamera,
                                                         const FbrTexture *pTexture,
                                                         FbrDescriptorSet_CameraUBO_TextureSampler *pDescriptorSet);

VkResult fbrCreateDescriptors_Std(const FbrVulkan *pVulkan,
                                  FbrDescriptors_Std **ppAllocDescriptors_Std);

VkResult fbrDestroyDescriptors_Std(const FbrVulkan *pVulkan,
                                   FbrDescriptors_Std *pDescriptors_Std);

#endif //FABRIC_DESCRIPTOR_SET_H
