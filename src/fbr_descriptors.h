#ifndef FABRIC_DESCRIPTOR_SET_H
#define FABRIC_DESCRIPTOR_SET_H

#include "fbr_app.h"
#include "fbr_camera.h"
#include "fbr_swap.h"
#include <vulkan/vulkan.h>

// Standard Pipeline
#define FBR_GLOBAL_SET_INDEX 0
typedef VkDescriptorSetLayout FbrSetLayoutGlobal;
//typedef struct FbrGlobalSet { // Not used but should I?
//    FbrCamera camera;
//} FbrGlobalSet;
typedef VkDescriptorSet FbrSetGlobal;

#define FBR_PASS_SET_INDEX 1
typedef VkDescriptorSetLayout FbrSetLayoutPass;
typedef VkDescriptorSet FbrSetPass;

#define FBR_MATERIAL_SET_INDEX 2
typedef VkDescriptorSetLayout FbrSetLayoutMaterial;
typedef VkDescriptorSet FbrSetMaterial;

#define FBR_OBJECT_SET_INDEX 3
typedef VkDescriptorSetLayout FbrSetLayoutObject;
typedef VkDescriptorSet FbrSetObject;

#define FBR_NODE_SET_INDEX 3
typedef VkDescriptorSetLayout FbrSetLayoutNode;
typedef VkDescriptorSet FbrSetNode;

#define FBR_COMPOSITE_SET_INDEX 1
typedef VkDescriptorSetLayout FbrSetLayoutComposite;
typedef VkDescriptorSet FbrSetComposite;


typedef struct FbrDescriptors {
    FbrSetLayoutGlobal setLayoutGlobal;
    FbrSetGlobal setGlobal;


    FbrSetLayoutPass setLayoutPass;
    FbrSetPass setPass;


    FbrSetLayoutMaterial setLayoutMaterial;
    FbrSetLayoutObject setLayoutObject;


    FbrSetLayoutNode setLayoutNode;


    FbrSetLayoutComposite setLayoutComposite;
//    FbrSetComposite setComposite;

} FbrDescriptors;

VkResult fbrCreateSetGlobal(const FbrVulkan *pVulkan,
                            FbrSetLayoutGlobal setLayout,
                            const FbrCamera *pCamera,
                            FbrSetGlobal *pSet);

VkResult fbrCreateSetPass(const FbrVulkan *pVulkan,
                          FbrSetLayoutPass setLayout,
                          const FbrTexture *pNormalTexture,
                          FbrSetPass *pSet);

VkResult fbrCreateSetMaterial(const FbrVulkan *pVulkan,
                              FbrSetLayoutMaterial setLayout,
                              const FbrTexture *pTexture,
                              FbrSetMaterial *pSet);

VkResult fbrCreateSetObject(const FbrVulkan *pVulkan,
                            FbrSetLayoutObject setLayout,
                            const FbrTransform *pTransform,
                            FbrSetObject *pSet);

VkResult fbrCreateSetNode(const FbrVulkan *pVulkan,
                          FbrSetLayoutNode setLayout,
                          const FbrTransform *pTransform,
                          const FbrCamera *pCamera,
                          const FbrTexture *pColorTexture,
                          const FbrTexture *pNormalTexture,
                          const FbrTexture *pDepthTexture,
                          FbrSetNode *pSet);

FBR_RESULT fbrCreateSetComposite(const FbrVulkan *pVulkan,
                                 FbrSetLayoutComposite setLayout,
                                 VkImageView inputColorImageView,
                                 VkImageView inputNormalImageView,
                                 VkImageView inputGBufferImageView,
                                 VkImageView inputDepthImageView,
                                 VkImageView outputColorImageView,
                                 FbrSetComposite *pSet);

VkResult fbrCreateDescriptors(const FbrVulkan *pVulkan, FbrDescriptors **ppAllocDescriptors_Std);

void fbrDestroyDescriptors(const FbrVulkan *pVulkan, FbrDescriptors *pDescriptors);

#endif //FABRIC_DESCRIPTOR_SET_H
