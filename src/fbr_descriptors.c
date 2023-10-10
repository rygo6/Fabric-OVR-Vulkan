#include "fbr_descriptors.h"
#include "fbr_vulkan.h"
#include "fbr_camera.h"
#include "fbr_texture.h"
#include "fbr_macros.h"
#include "fbr_log.h"
#include "stb_ds.h"

// TODO rewrite descriptors using push scheme
VkDescriptorSetLayoutBinding *pLayoutBindingArray = NULL;
VkWriteDescriptorSet *pDescriptorWriteArray = NULL;

static void pushLayoutBinding(VkDescriptorSetLayoutBinding binding)
{
    binding.binding = arrlen(pLayoutBindingArray);
    binding.descriptorCount = binding.descriptorCount == 0 ? 1 : binding.descriptorCount;
            arrput(pLayoutBindingArray, binding);
}

static FBR_RESULT createDescriptorSetLayout(const FbrVulkan *pVulkan,
                                            int bindingsCount,
                                            const VkDescriptorSetLayoutBinding *pBindings,
                                            FbrSetLayout *pSetLayout)
{
    pSetLayout->bindingCount = bindingsCount;
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .bindingCount = pSetLayout->bindingCount,
            .pBindings = pBindings,
    };
    FBR_ACK(vkCreateDescriptorSetLayout(pVulkan->device,
                                        &layoutInfo,
                                        NULL,
                                        &pSetLayout->layout));
    return FBR_SUCCESS;
}

static FBR_RESULT createPushedDescriptorSetLayout(const FbrVulkan *pVulkan, FbrSetLayout *pSetLayout)
{
    FBR_ACK(createDescriptorSetLayout(pVulkan,
                                      arrlen(pLayoutBindingArray),
                                      pLayoutBindingArray,
                                      pSetLayout));
    arrsetlen(pLayoutBindingArray, 0);
    return FBR_SUCCESS;
}

static FBR_RESULT allocateDescriptorSet(const FbrVulkan *pVulkan,
                                        const FbrSetLayout *pSetLayout,
                                        VkDescriptorSet *pDescriptorSet)
{
    const VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = NULL,
            .descriptorPool = pVulkan->descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &pSetLayout->layout,
    };
    FBR_ACK(vkAllocateDescriptorSets(pVulkan->device,
                                     &allocInfo,
                                     pDescriptorSet));
    return FBR_SUCCESS;
}

static void pushDescriptorWrite(VkWriteDescriptorSet descriptorWrite, VkDescriptorSet *pSet)
{
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = *pSet;
    descriptorWrite.dstBinding = arrlen(pDescriptorWriteArray);
    descriptorWrite.descriptorCount = descriptorWrite.descriptorCount == 0 ? 1 : descriptorWrite.descriptorCount;
    arrput(pDescriptorWriteArray, descriptorWrite);
}

static void writePushedDescriptors(const FbrVulkan *pVulkan)
{
    vkUpdateDescriptorSets(pVulkan->device,
                           arrlen(pDescriptorWriteArray),
                           pDescriptorWriteArray,
                           0,
                           NULL);
    arrsetlen(pDescriptorWriteArray, 0);
}

FBR_RESULT fbrCreateSetGlobal(const FbrVulkan *pVulkan,
                              const FbrDescriptors *pDescriptors,
                              const FbrCamera *pCamera,
                              FbrSetPass *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  &pDescriptors->setLayoutGlobal,
                                  pSet));
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &(VkDescriptorBufferInfo) {
                    .buffer = pCamera->pUBO->uniformBuffer,
                    .range = sizeof(FbrCameraBuffer),
            },
    }, pSet);
    writePushedDescriptors(pVulkan);
    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutGlobal(const FbrVulkan *pVulkan,
                                        FbrSetLayoutGlobal *pSetLayout)
{
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                          VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                          VK_SHADER_STAGE_COMPUTE_BIT |
                          VK_SHADER_STAGE_FRAGMENT_BIT |
                          VK_SHADER_STAGE_MESH_BIT_EXT |
                          VK_SHADER_STAGE_TASK_BIT_EXT,
    });
    FBR_ACK(createPushedDescriptorSetLayout(pVulkan, pSetLayout));
    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateSetPass(const FbrVulkan *pVulkan,
                            const FbrDescriptors *pDescriptors,
                            const FbrTexture *pNormalTexture,
                            FbrSetPass *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  &pDescriptors->setLayoutPass,
                                  pSet));
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = pNormalTexture->imageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    writePushedDescriptors(pVulkan);
    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutPass(const FbrVulkan *pVulkan,
                                      FbrSetLayoutPass *pSetLayout)
{
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                          VK_SHADER_STAGE_FRAGMENT_BIT,
    });
    FBR_ACK(createPushedDescriptorSetLayout(pVulkan, pSetLayout));
    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateSetMaterial(const FbrVulkan *pVulkan,
                                const FbrDescriptors *pDescriptors,
                                const FbrTexture *pTexture,
                                FbrSetMaterial *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  &pDescriptors->setLayoutMaterial,
                                  pSet));
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = pTexture->imageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    writePushedDescriptors(pVulkan);
    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutMaterial(const FbrVulkan *pVulkan,
                                          FbrSetLayoutMaterial *pSetLayout)
{
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    });
    FBR_ACK(createPushedDescriptorSetLayout(pVulkan, pSetLayout));
    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateSetObject(const FbrVulkan *pVulkan,
                              const FbrDescriptors *pDescriptors,
                              const FbrTransform *pTransform,
                              FbrSetObject *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  &pDescriptors->setLayoutObject,
                                  pSet));
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &(VkDescriptorBufferInfo) {
                    .buffer = pTransform->pUBO->uniformBuffer,
                    .range = sizeof(FbrTransform),
            },
    }, pSet);
    writePushedDescriptors(pVulkan);
    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutObject(const FbrVulkan *pVulkan,
                                        FbrSetLayoutObject *pSetLayout)
{
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                          VK_SHADER_STAGE_FRAGMENT_BIT,
    });
    FBR_ACK(createPushedDescriptorSetLayout(pVulkan, pSetLayout));
    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateSetNode(const FbrVulkan *pVulkan,
                            const FbrDescriptors *pDescriptors,
                            const FbrTransform *pTransform,
                            const FbrCamera *pCamera,
                            const FbrTexture *pColorTexture,
                            const FbrTexture *pNormalTexture,
                            const FbrTexture *pDepthTexture,
                            FbrSetNode *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  &pDescriptors->setLayoutNode,
                                  pSet));
    // transform UBO
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &(VkDescriptorBufferInfo) {
                    .buffer = pTransform->pUBO->uniformBuffer,
                    .range = sizeof(FbrTransform),
            }
    }, pSet);
    // camera UBO from which it was rendered
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &(VkDescriptorBufferInfo) {
                    .buffer = pCamera->pUBO->uniformBuffer,
                    .range = sizeof(FbrCamera),
            },
    }, pSet);
    // rgb map
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = pColorTexture->imageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    // normal map
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = pNormalTexture->imageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    // depth map
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = pDepthTexture->imageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    writePushedDescriptors(pVulkan);
    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutNode(const FbrVulkan *pVulkan,
                                      FbrSetLayoutNode *pSetLayout)
{
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                          VK_SHADER_STAGE_FRAGMENT_BIT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                          VK_SHADER_STAGE_FRAGMENT_BIT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                          VK_SHADER_STAGE_FRAGMENT_BIT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                          VK_SHADER_STAGE_FRAGMENT_BIT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                          VK_SHADER_STAGE_FRAGMENT_BIT,
    });
    FBR_ACK(createPushedDescriptorSetLayout(pVulkan, pSetLayout));
    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateSetComputeComposite(const FbrVulkan *pVulkan,
                                        const FbrDescriptors *pDescriptors,
                                        VkImageView inputColorImageView,
                                        VkImageView inputNormalImageView,
                                        VkImageView inputGBufferImageView,
                                        VkImageView inputDepthImageView,
                                        VkImageView outputColorImageView,
                                        FbrSetComputeComposite *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  &pDescriptors->setLayoutComputeComposite,
                                  pSet));
    // input color
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = inputColorImageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    // input normal
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = inputNormalImageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    // input g buffer
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = inputGBufferImageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    // input depth
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = inputDepthImageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    // output color
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .imageView = outputColorImageView,
//            .linearSampler = pVulkan->linearSampler,
            },
    }, pSet);
    writePushedDescriptors(pVulkan);
    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutComputeComposite(const FbrVulkan *pVulkan,
                                                  FbrSetLayoutComputeComposite *pSetLayout)
{
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    });
    FBR_ACK(createPushedDescriptorSetLayout(pVulkan, pSetLayout));
    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateSetMeshComposite(const FbrVulkan *pVulkan,
                                     const FbrDescriptors *pDescriptors,
                                     const FbrCamera *pNodeCamera,
                                     VkImageView inputColorImageView,
                                     VkImageView inputNormalImageView,
                                     VkImageView inputGBufferImageView,
                                     VkImageView inputDepthImageView,
                                     FbrSetComputeComposite *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  &pDescriptors->setLayoutMeshComposite,
                                  pSet));
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &(VkDescriptorBufferInfo) {
                    .buffer = pNodeCamera->pUBO->uniformBuffer,
                    .range = sizeof(FbrCamera),
            },
    }, pSet);
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = inputColorImageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = inputNormalImageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = inputGBufferImageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    pushDescriptorWrite((VkWriteDescriptorSet) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &(VkDescriptorImageInfo) {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = inputDepthImageView,
                    .sampler = pVulkan->linearSampler,
            },
    }, pSet);
    writePushedDescriptors(pVulkan);
    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutMeshComposite(const FbrVulkan *pVulkan,
                                               FbrSetLayoutMeshComposite *pSetLayout)
{
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
    });
    pushLayoutBinding((VkDescriptorSetLayoutBinding) {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
    });
    FBR_ACK(createPushedDescriptorSetLayout(pVulkan, pSetLayout));
    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayouts(const FbrVulkan *pVulkan,
                                   FbrDescriptors *pDescriptors)
{
    createSetLayoutGlobal(pVulkan, &pDescriptors->setLayoutGlobal);
    createSetLayoutPass(pVulkan, &pDescriptors->setLayoutPass);
    createSetLayoutMaterial(pVulkan, &pDescriptors->setLayoutMaterial);
    createSetLayoutObject(pVulkan, &pDescriptors->setLayoutObject);
    createSetLayoutNode(pVulkan, &pDescriptors->setLayoutNode);
    createSetLayoutComputeComposite(pVulkan, &pDescriptors->setLayoutComputeComposite);
    createSetLayoutMeshComposite(pVulkan, &pDescriptors->setLayoutMeshComposite);
    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateDescriptors(const FbrVulkan *pVulkan,
                              FbrDescriptors **ppAllocDescriptors)
{
    *ppAllocDescriptors = calloc(1, sizeof(FbrDescriptors));
    FbrDescriptors *pDescriptors_Std = *ppAllocDescriptors;

    arrsetcap(pLayoutBindingArray, 16);
    arrsetcap(pDescriptorWriteArray, 16);

    createSetLayouts(pVulkan, pDescriptors_Std);
    return FBR_SUCCESS;
}

void fbrDestroyDescriptors(const FbrVulkan *pVulkan,
                           FbrDescriptors *pDescriptors)
{
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutGlobal.layout, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutPass.layout, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutMaterial.layout, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutObject.layout, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutComputeComposite.layout, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutNode.layout, NULL);

    vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &pDescriptors->setGlobal);
    if (pDescriptors->setPass != NULL)
        vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &pDescriptors->setPass);

    free(pDescriptors);
}