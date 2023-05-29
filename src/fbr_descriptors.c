#include "fbr_descriptors.h"
#include "fbr_vulkan.h"
#include "fbr_camera.h"
#include "fbr_texture.h"

static FBR_RESULT createDescriptorSetLayout(const FbrVulkan *pVulkan,
                                          uint32_t bindingCount,
                                          const VkDescriptorSetLayoutBinding *pBindings,
                                          VkDescriptorSetLayout *pSetLayout)
{
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .bindingCount = bindingCount,
            .pBindings = pBindings,
    };
    FBR_ACK(vkCreateDescriptorSetLayout(pVulkan->device,
                                        &layoutInfo,
                                        NULL,
                                        pSetLayout));

    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutGlobal(const FbrVulkan *pVulkan, FbrSetLayoutGlobal *pSetLayout)
{
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    FBR_ACK(createDescriptorSetLayout(pVulkan, 1, bindings, pSetLayout));

    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutPass(const FbrVulkan *pVulkan, FbrSetLayoutPass *pSetLayout)
{
    VkDescriptorSetLayoutBinding bindings[] = {
            {// normal
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            },
    };
    FBR_ACK(createDescriptorSetLayout(pVulkan, 1, bindings, pSetLayout));

    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutMaterial(const FbrVulkan *pVulkan, FbrSetLayoutMaterial *pSetLayout)
{
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    FBR_ACK(createDescriptorSetLayout(pVulkan, 1, bindings, pSetLayout));

    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutObject(const FbrVulkan *pVulkan, FbrSetLayoutMaterial *pSetLayout)
{
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    FBR_ACK(createDescriptorSetLayout(pVulkan, 1, bindings, pSetLayout));

    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutNode(const FbrVulkan *pVulkan, FbrSetLayoutNode *pSetLayout)
{
    VkDescriptorSetLayoutBinding pBindings[] = {
            {// transform ubo
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                    .pImmutableSamplers = NULL,
            },
            {// camera rendered from ubo
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                    .pImmutableSamplers = NULL,
            },
            {// color
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            },
            {// normal
                    .binding = 3,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            },
            {// depth
                    .binding = 4,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            },

    };
    FBR_ACK(createDescriptorSetLayout(pVulkan,
                                      sizeof(pBindings) / sizeof(VkDescriptorSetLayoutBinding),
                                      pBindings,
                                      pSetLayout));

    return FBR_SUCCESS;
}

static FBR_RESULT allocateDescriptorSet(const FbrVulkan *pVulkan,
                                      uint32_t descriptorSetCount,
                                      const VkDescriptorSetLayout *pSetLayouts,
                                      VkDescriptorSet *pDescriptorSet)
{
    const VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = NULL,
            .descriptorPool = pVulkan->descriptorPool,
            .descriptorSetCount = descriptorSetCount,
            .pSetLayouts = pSetLayouts,
    };
    FBR_ACK(vkAllocateDescriptorSets(pVulkan->device,
                                     &allocInfo,
                                     pDescriptorSet));

    return FBR_SUCCESS;
}

VkResult fbrCreateSetGlobal(const FbrVulkan *pVulkan,
                            FbrSetLayoutGlobal setLayout,
                            const FbrCamera *pCamera,
                            FbrSetGlobal *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  1,
                                  &setLayout,
                                  pSet));
    const VkDescriptorBufferInfo bufferInfo = {
            .buffer = pCamera->pUBO->uniformBuffer,
            .offset = 0,
            .range = sizeof(FbrCameraUBO),
    };
    const VkWriteDescriptorSet pDescriptorWrites[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                    .pImageInfo = NULL,
                    .pBufferInfo = &bufferInfo,
                    .pTexelBufferView = NULL,
            },
    };
    vkUpdateDescriptorSets(pVulkan->device,
                           1,
                           pDescriptorWrites,
                           0,
                           NULL);
    return VK_SUCCESS;
}

VkResult fbrCreateSetPass(const FbrVulkan *pVulkan,
                          FbrSetLayoutPass setLayout,
                          const FbrTexture *pNormalTexture,
                          FbrSetPass *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  1,
                                  &setLayout,
                                  pSet));
    const VkDescriptorImageInfo normalImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pNormalTexture->imageView,
            .sampler = pVulkan->sampler,
    };
    const VkWriteDescriptorSet pDescriptorWrites[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &normalImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
    };
    vkUpdateDescriptorSets(pVulkan->device,
                           1,
                           pDescriptorWrites,
                           0,
                           NULL);
    return VK_SUCCESS;
}

VkResult fbrCreateSetMaterial(const FbrVulkan *pVulkan,
                              FbrSetLayoutMaterial setLayout,
                              const FbrTexture *pTexture,
                              FbrSetMaterial *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  1,
                                  &setLayout,
                                  pSet));
    const VkDescriptorImageInfo imageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pTexture->imageView,
            .sampler = pVulkan->sampler,
    };
    const VkWriteDescriptorSet pDescriptorWrites[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &imageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
    };
    vkUpdateDescriptorSets(pVulkan->device,
                           1,
                           pDescriptorWrites,
                           0,
                           NULL);
    return VK_SUCCESS;
}


VkResult fbrCreateSetObject(const FbrVulkan *pVulkan,
                            FbrSetLayoutObject setLayout,
                            const FbrTransform *pTransform,
                            FbrSetObject *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  1,
                                  &setLayout,
                                  pSet));
    const VkDescriptorBufferInfo bufferInfo = {
            .buffer = pTransform->pUBO->uniformBuffer,
            .offset = 0,
            .range = sizeof(FbrTransform),
    };
    const VkWriteDescriptorSet pDescriptorWrites[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pImageInfo = NULL,
                    .pBufferInfo = &bufferInfo,
                    .pTexelBufferView = NULL,
            },
    };
    vkUpdateDescriptorSets(pVulkan->device,
                           1,
                           pDescriptorWrites,
                           0,
                           NULL);
    return VK_SUCCESS;
}

VkResult fbrCreateSetNode(const FbrVulkan *pVulkan,
                          FbrSetLayoutNode setLayout,
                          const FbrTransform *pTransform,
                          const FbrCamera *pCamera,
                          const FbrTexture *pColorTexture,
                          const FbrTexture *pNormalTexture,
                          const FbrTexture *pDepthTexture,
                          FbrSetNode *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  1,
                                  &setLayout,
                                  pSet));
    const VkDescriptorBufferInfo transformBufferInfo = {
            .buffer = pTransform->pUBO->uniformBuffer,
            .offset = 0,
            .range = sizeof(FbrTransform),
    };
    const VkDescriptorBufferInfo cameraBufferInfo = {
            .buffer = pCamera->pUBO->uniformBuffer,
            .offset = 0,
            .range = sizeof(FbrCamera),
    };
    const VkDescriptorImageInfo colorImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pColorTexture->imageView,
            .sampler = pVulkan->sampler,
    };
    const VkDescriptorImageInfo normalImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pNormalTexture->imageView,
            .sampler = pVulkan->sampler,
    };
    const VkDescriptorImageInfo depthImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pDepthTexture->imageView,
            .sampler = pVulkan->sampler,
    };
    const VkWriteDescriptorSet pDescriptorWrites[] = {
            {// transform UBO
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pImageInfo = NULL,
                    .pBufferInfo = &transformBufferInfo,
                    .pTexelBufferView = NULL,
            },
            {// camera UBO from which it was rendered
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                    .pImageInfo = NULL,
                    .pBufferInfo = &cameraBufferInfo,
                    .pTexelBufferView = NULL,
            },
            {// rgb map
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 2,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &colorImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
            {// normal map
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 3,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &normalImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
            {// depth map
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 4,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &depthImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
    };
    vkUpdateDescriptorSets(pVulkan->device,
                           sizeof(pDescriptorWrites) / sizeof(VkWriteDescriptorSet),
                           pDescriptorWrites,
                           0,
                           NULL);
    return VK_SUCCESS;
}

static FBR_RESULT createSetLayoutComposite(const FbrVulkan *pVulkan, FbrSetLayoutComposite *pSetLayout)
{
    VkDescriptorSetLayoutBinding pBindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = NULL,
            },
            {
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    FBR_ACK(createDescriptorSetLayout(pVulkan,
                                      sizeof(pBindings) / sizeof(VkDescriptorSetLayoutBinding),
                                      pBindings,
                                      pSetLayout));

    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateSetComposite(const FbrVulkan *pVulkan,
                               FbrSetLayoutComposite setLayout,
                               VkImageView sourceImageView,
                               VkImageView destinationImageView,
                               FbrSetComposite *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  1,
                                  &setLayout,
                                  pSet));

    const VkDescriptorImageInfo sourceImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            .imageView = sourceImageView,
//            .sampler = pVulkan->sampler,
    };
    const VkDescriptorImageInfo targetImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            .imageView = destinationImageView,
//            .sampler = pVulkan->sampler,
    };
    const VkWriteDescriptorSet pDescriptorWrites[] = {
            // Color Buffer
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &sourceImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
            // Swap
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &targetImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
    };
    vkUpdateDescriptorSets(pVulkan->device,
                           sizeof(pDescriptorWrites) / sizeof(VkWriteDescriptorSet),
                           pDescriptorWrites,
                           0,
                           NULL);

    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayouts(const FbrVulkan *pVulkan, FbrDescriptors *pDescriptors)
{
    createSetLayoutGlobal(pVulkan, &pDescriptors->setLayoutGlobal);
    createSetLayoutPass(pVulkan, &pDescriptors->setLayoutPass);
    createSetLayoutMaterial(pVulkan, &pDescriptors->setLayoutMaterial);
    createSetLayoutObject(pVulkan, &pDescriptors->setLayoutObject);

    createSetLayoutNode(pVulkan, &pDescriptors->setLayoutNode);

    createSetLayoutComposite(pVulkan, &pDescriptors->setLayoutComposite);

    return FBR_SUCCESS;
}

VkResult fbrCreateDescriptors(const FbrVulkan *pVulkan,
                              FbrDescriptors **ppAllocDescriptors_Std)
{
    *ppAllocDescriptors_Std = calloc(1, sizeof(FbrDescriptors));
    FbrDescriptors *pDescriptors_Std = *ppAllocDescriptors_Std;
    createSetLayouts(pVulkan, pDescriptors_Std);

    return VK_SUCCESS;
}

void fbrDestroyDescriptors(const FbrVulkan *pVulkan,
                           FbrDescriptors *pDescriptors)
{
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutGlobal, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutPass, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutMaterial, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutObject, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutComposite, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutNode, NULL);

    vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &pDescriptors->setGlobal);
    if (pDescriptors->setPass != NULL)
        vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &pDescriptors->setPass);



    free(pDescriptors);
}