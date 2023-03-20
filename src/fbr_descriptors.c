#include "fbr_descriptors.h"
#include "fbr_vulkan.h"
#include "fbr_camera.h"
#include "fbr_texture.h"

static VkResult createDescriptorSetLayout(const FbrVulkan *pVulkan,
                                          uint32_t bindingCount,
                                          const VkDescriptorSetLayoutBinding *pBindings,
                                          VkDescriptorSetLayout *pSetLayout) {
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .bindingCount = bindingCount,
            .pBindings = pBindings,
    };
    VK_CHECK(vkCreateDescriptorSetLayout(pVulkan->device,
                                         &layoutInfo,
                                         NULL,
                                         pSetLayout));
    return VK_SUCCESS;
}

static VkResult createSetLayoutGlobal(const FbrVulkan *pVulkan, FbrSetLayoutGlobal *pSetLayout) {
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    VK_CHECK(createDescriptorSetLayout(pVulkan, 1, bindings, pSetLayout));
    return VK_SUCCESS;
}

static VkResult createSetLayoutPass(const FbrVulkan *pVulkan, FbrSetLayoutPass *pSetLayout) {
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    VK_CHECK(createDescriptorSetLayout(pVulkan, 1, bindings, pSetLayout));
    return VK_SUCCESS;
}

static VkResult createSetLayoutMaterial(const FbrVulkan *pVulkan, FbrSetLayoutMaterial *pSetLayout) {
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    VK_CHECK(createDescriptorSetLayout(pVulkan, 1, bindings, pSetLayout));
    return VK_SUCCESS;
}

static VkResult createSetLayoutObject(const FbrVulkan *pVulkan, FbrSetLayoutMaterial *pSetLayout) {
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    VK_CHECK(createDescriptorSetLayout(pVulkan, 1, bindings, pSetLayout));
    return VK_SUCCESS;
}

static VkResult createSetLayouts_Std(const FbrVulkan *pVulkan, FbrDescriptors *pDescriptors_Std) {
//    createSetLayout_vUBO_fSampler(pVulkan, &pDescriptors_Std->setLayout_vUniform_fSampler);

    createSetLayoutGlobal(pVulkan, &pDescriptors_Std->setLayoutGlobal);
    createSetLayoutPass(pVulkan, &pDescriptors_Std->setLayoutPass);
    createSetLayoutMaterial(pVulkan, &pDescriptors_Std->setLayoutMaterial);
    createSetLayoutObject(pVulkan, &pDescriptors_Std->setLayoutObject);
    return VK_SUCCESS;
}

static VkResult allocateDescriptorSet(const FbrVulkan *pVulkan,
                                      uint32_t descriptorSetCount,
                                      const VkDescriptorSetLayout *pSetLayouts,
                                      VkDescriptorSet *pDescriptorSet) {
    const VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = NULL,
            .descriptorPool = pVulkan->descriptorPool,
            .descriptorSetCount = descriptorSetCount,
            .pSetLayouts = pSetLayouts,
    };
    VK_CHECK(vkAllocateDescriptorSets(pVulkan->device,
                                      &allocInfo,
                                      pDescriptorSet));
    return VK_SUCCESS;
}

VkResult fbrCreateSetGlobal(const FbrVulkan *pVulkan,
                            FbrSetLayoutGlobal setLayout,
                           const FbrCamera *pCamera,
                            FbrSetGlobal *pSet) {
    VK_CHECK(allocateDescriptorSet(pVulkan,
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

VkResult fbrCreateSetMaterial(const FbrVulkan *pVulkan,
                            FbrSetLayoutMaterial setLayout,
                            const FbrTexture *pTexture,
                            FbrSetMaterial *pSet) {
    VK_CHECK(allocateDescriptorSet(pVulkan,
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
                              FbrSetObject *pSet) {
    VK_CHECK(allocateDescriptorSet(pVulkan,
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

VkResult fbrCreateDescriptors(const FbrVulkan *pVulkan,
                              FbrDescriptors **ppAllocDescriptors_Std) {
    *ppAllocDescriptors_Std = calloc(1, sizeof(FbrDescriptors));
    FbrDescriptors *pDescriptors_Std = *ppAllocDescriptors_Std;
    createSetLayouts_Std(pVulkan, pDescriptors_Std);
}

void fbrDestroyDescriptors(const FbrVulkan *pVulkan,
                           FbrDescriptors *pDescriptors) {
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutGlobal, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutPass, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutMaterial, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutObject, NULL);
    vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &pDescriptors->setGlobal);
    free(pDescriptors);
}