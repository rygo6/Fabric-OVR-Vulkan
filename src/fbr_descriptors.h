#ifndef FABRIC_DESCRIPTOR_SET_H
#define FABRIC_DESCRIPTOR_SET_H

#include "fbr_app.h"
#include <vulkan/vulkan.h>

// Uniform.VertStage.UBO _ Uniform.FragStage.Sampler
typedef VkDescriptorSetLayout FbrSetLayout_uvUBO_ufSampler;
typedef VkDescriptorSet FbrSet_uvUBO_ufSampler;
// Uniform.VertStage.Camera _ Uniform.FragStage.Texture
typedef FbrSet_uvUBO_ufSampler FbrDescriptorSet_uvCamera_ufTexture;

typedef struct FbrDescriptors {
    FbrSetLayout_uvUBO_ufSampler setLayout_uvUBO_ufSampler;
} FbrDescriptors;

VkResult fbrCreateDescriptorSet_Camera_Texture(const FbrVulkan *pVulkan,
                                               // Layout
                                               FbrSetLayout_uvUBO_ufSampler setLayout_uvUBO_ufSampler,
                                               // Values
                                               const FbrCamera *pCamera,
                                               const FbrTexture *pTexture,
                                               // Output
                                               FbrDescriptorSet_uvCamera_ufTexture *pDescriptorSet);

VkResult fbrCreateDescriptors(const FbrVulkan *pVulkan, FbrDescriptors **ppAllocDescriptors_Std);

VkResult fbrDestroyDescriptors_Std(const FbrVulkan *pVulkan, FbrDescriptors *pDescriptors_Std);

#endif //FABRIC_DESCRIPTOR_SET_H
