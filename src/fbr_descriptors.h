#ifndef FABRIC_DESCRIPTOR_SET_H
#define FABRIC_DESCRIPTOR_SET_H

#include "fbr_app.h"
#include "fbr_camera.h"
#include "fbr_swap.h"
#include <vulkan/vulkan.h>

typedef struct FbrSetLayout {
    VkDescriptorSetLayout layout;
    int bindingCount;
} FbrSetLayout;

#define FBR_DEFINE_DESCRIPTOR(name) \
    typedef FbrSetLayout FbrSetLayout##name; \
    typedef VkDescriptorSet FbrSet##name; \

#define FBR_GLOBAL_SET_INDEX 0
FBR_DEFINE_DESCRIPTOR(Global)

#define FBR_PASS_SET_INDEX 1
FBR_DEFINE_DESCRIPTOR(Pass)

#define FBR_MATERIAL_SET_INDEX 2
FBR_DEFINE_DESCRIPTOR(Material)

#define FBR_OBJECT_SET_INDEX 3
FBR_DEFINE_DESCRIPTOR(Object)

#define FBR_NODE_SET_INDEX 3
FBR_DEFINE_DESCRIPTOR(Node)

#define FBR_COMPUTE_COMPOSITE_SET_INDEX 1
FBR_DEFINE_DESCRIPTOR(ComputeComposite)

#define FBR_MESH_COMPOSITE_SET_INDEX 1
FBR_DEFINE_DESCRIPTOR(MeshComposite)

#define FBR_STRUCT_DESCRIPTOR(name) FbrSetLayout##name setLayout##name;

typedef struct FbrDescriptors {
    FBR_STRUCT_DESCRIPTOR(Global)
    FbrSetGlobal setGlobal;

    FBR_STRUCT_DESCRIPTOR(Pass)
    FbrSetPass setPass;

    FBR_STRUCT_DESCRIPTOR(Material)
    FbrSetMaterial setMaterial;

    FBR_STRUCT_DESCRIPTOR(Object)
    FbrSetObject setObject;

    FBR_STRUCT_DESCRIPTOR(Node)
    FbrSetNode setNode;

    FBR_STRUCT_DESCRIPTOR(ComputeComposite)
    FbrSetComputeComposite setComputeComposite;

    FBR_STRUCT_DESCRIPTOR(MeshComposite)
    FbrSetMeshComposite setMeshComposites[2];
} FbrDescriptors;

FBR_RESULT fbrCreateSetGlobal(const FbrVulkan *pVulkan,
                              const FbrDescriptors *pDescriptors,
                              const FbrCamera *pCamera,
                              FbrSetPass *pSet);

FBR_RESULT fbrCreateSetPass(const FbrVulkan *pVulkan,
                            const FbrDescriptors *pDescriptors,
                            const FbrTexture *pNormalTexture,
                            FbrSetPass *pSet);

FBR_RESULT fbrCreateSetMaterial(const FbrVulkan *pVulkan,
                                const FbrDescriptors *pDescriptors,
                                const FbrTexture *pTexture,
                                FbrSetMaterial *pSet);

FBR_RESULT fbrCreateSetObject(const FbrVulkan *pVulkan,
                              const FbrDescriptors *pDescriptors,
                              const FbrTransform *pTransform,
                              FbrSetObject *pSet);

FBR_RESULT fbrCreateSetNode(const FbrVulkan *pVulkan,
                            const FbrDescriptors *pDescriptors,
                            const FbrTransform *pTransform,
                            const FbrCamera *pCamera,
                            const FbrTexture *pColorTexture,
                            const FbrTexture *pNormalTexture,
                            const FbrTexture *pDepthTexture,
                            FbrSetNode *pSet);

FBR_RESULT fbrCreateSetComputeComposite(const FbrVulkan *pVulkan,
                                        const FbrDescriptors *pDescriptors,
                                        VkImageView inputColorImageView,
                                        VkImageView inputNormalImageView,
                                        VkImageView inputGBufferImageView,
                                        VkImageView inputDepthImageView,
                                        VkImageView outputColorImageView,
                                        FbrSetComputeComposite *pSet);

FBR_RESULT fbrCreateSetMeshComposite(const FbrVulkan *pVulkan,
                                     const FbrDescriptors *pDescriptors,
                                     const FbrCamera *pNodeCamera,
                                     VkImageView inputColorImageView,
                                     VkImageView inputNormalImageView,
                                     VkImageView inputGBufferImageView,
                                     VkImageView inputDepthImageView,
                                     FbrSetComputeComposite *pSet);

FBR_RESULT fbrCreateDescriptors(const FbrVulkan *pVulkan,
                                FbrDescriptors **ppAllocDescriptors);

void fbrDestroyDescriptors(const FbrVulkan *pVulkan,
                           FbrDescriptors *pDescriptors);

#endif //FABRIC_DESCRIPTOR_SET_H
