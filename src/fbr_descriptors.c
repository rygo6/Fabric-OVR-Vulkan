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
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            }
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
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                    .pImmutableSamplers = NULL,
            },
            {
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            },
            {
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    FBR_ACK(createDescriptorSetLayout(pVulkan, 3, bindings, pSetLayout));

    FBR_SUCCESS;
}

static VkResult createSetLayouts(const FbrVulkan *pVulkan, FbrDescriptors *pDescriptors_Std)
{
    createSetLayoutGlobal(pVulkan, &pDescriptors_Std->setLayoutGlobal);
    createSetLayoutPass(pVulkan, &pDescriptors_Std->setLayoutPass);
    createSetLayoutMaterial(pVulkan, &pDescriptors_Std->setLayoutMaterial);
    createSetLayoutObject(pVulkan, &pDescriptors_Std->setLayoutObject);

    createSetLayoutNode(pVulkan, &pDescriptors_Std->setLayoutNode);

    return VK_SUCCESS;
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
                          const FbrTexture *pDepthTexture,
                          const FbrTexture *pColorTexture,
                          FbrSetNode *pSet)
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
    const VkDescriptorImageInfo depthImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pDepthTexture->imageView,
            .sampler = pVulkan->sampler,
    };
    const VkDescriptorImageInfo colorImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pColorTexture->imageView,
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
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pImageInfo = NULL,
                    .pBufferInfo = &bufferInfo,
                    .pTexelBufferView = NULL,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &depthImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
            {
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
    };
    vkUpdateDescriptorSets(pVulkan->device,
                           sizeof(pDescriptorWrites) / sizeof(VkWriteDescriptorSet),
                           pDescriptorWrites,
                           0,
                           NULL);
    return VK_SUCCESS;
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
    vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &pDescriptors->setGlobal);

    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutNode, NULL);

    free(pDescriptors);
}