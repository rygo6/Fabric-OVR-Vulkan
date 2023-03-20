#ifndef FABRIC_DESCRIPTOR_SET_H
#define FABRIC_DESCRIPTOR_SET_H

#include "fbr_app.h"
#include "fbr_camera.h"
#include <vulkan/vulkan.h>

//Global Binding 0
#define FBR_GLOBAL_SET_INDEX 0
typedef VkDescriptorSetLayout FbrSetLayoutGlobal;
//typedef struct FbrGlobalSet { // Not used but should I?
//    FbrCamera camera;
//} FbrGlobalSet;
typedef VkDescriptorSet FbrSetGlobal;

// Pass Binding 1
#define FBR_PASS_SET_INDEX 1
typedef VkDescriptorSetLayout FbrSetLayoutPass;

// Material Binding 2
#define FBR_MATERIAL_SET_INDEX 2
typedef VkDescriptorSetLayout FbrSetLayoutMaterial;
//typedef struct FbrMaterialSet { // Not used but should I?
//    VkImageView imageView;
//} FbrMaterialSet;
typedef VkDescriptorSet FbrSetMaterial;

// Object Binding 3
#define FBR_OBJECT_SET_INDEX 3
typedef VkDescriptorSetLayout FbrSetLayoutObject;
typedef VkDescriptorSet FbrSetObject;


typedef struct FbrDescriptors {
    FbrSetLayoutGlobal setLayoutGlobal;
    FbrSetLayoutPass setLayoutPass;
    FbrSetLayoutMaterial setLayoutMaterial;
    FbrSetLayoutObject setLayoutObject;
    FbrSetGlobal setGlobal;
} FbrDescriptors;

//VkResult fbrCreateDescriptorSet_Camera_Texture(const FbrVulkan *pVulkan,
//                                               // Layout
//                                               FbrSetLayout_vUniform_fSampler setLayout_uvUBO_ufSampler,
//                                               // Values
//                                               const FbrCamera *pCamera,
//                                               const FbrTexture *pTexture,
//                                               // Output
//                                               FbrDescriptorSet_vCamera_fTexture *pDescriptorSet);

VkResult fbrCreateSetGlobal(const FbrVulkan *pVulkan,
                            FbrSetLayoutGlobal setLayout,
                            const FbrCamera *pCamera,
                            FbrSetGlobal *pSet);

VkResult fbrCreateSetMaterial(const FbrVulkan *pVulkan,
                              FbrSetLayoutMaterial setLayout,
                              const FbrTexture *pTexture,
                              FbrSetMaterial *pSet);

VkResult fbrCreateSetObject(const FbrVulkan *pVulkan,
                              FbrSetLayoutMaterial setLayout,
                              const FbrTransform *pTransform,
                              FbrSetMaterial *pSet);

VkResult fbrCreateDescriptors(const FbrVulkan *pVulkan, FbrDescriptors **ppAllocDescriptors_Std);

void fbrDestroyDescriptors(const FbrVulkan *pVulkan, FbrDescriptors *pDescriptors);

#endif //FABRIC_DESCRIPTOR_SET_H
