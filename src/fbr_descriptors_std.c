#include "fbr_descriptors_std.h"
#include "fbr_vulkan.h"
#include "fbr_camera.h"
#include "fbr_texture.h"

static VkResult createDescriptorSetLayout(const FbrVulkan *pVulkan,
                                          uint32_t bindingCount,
                                          const VkDescriptorSetLayoutBinding *pBindings,
                                          VkDescriptorSetLayout *pSetLayout) {
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .bindingCount = bindingCount,
            .pBindings = pBindings,
    };
    VK_CHECK(vkCreateDescriptorSetLayout(pVulkan->device,
                                         &layoutInfo,
                                         NULL,
                                         pSetLayout));
}

static VkResult allocateDescriptorSet(const FbrVulkan *pVulkan,
                                      uint32_t descriptorSetCount,
                                      const VkDescriptorSetLayout *pSetLayouts,
                                      VkDescriptorSet *pDescriptorSet) {
    const VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .descriptorPool = pVulkan->descriptorPool,
            .descriptorSetCount = descriptorSetCount,
            .pSetLayouts = pSetLayouts,
    };
    VK_CHECK(vkAllocateDescriptorSets(pVulkan->device,
                                      &allocInfo,
                                      pDescriptorSet));
}

static VkResult createSetLayout_VertUBO_FragSampler(const FbrVulkan *pVulkan, FbrSetLayout_VertUBO_FragSampler *pSetLayout) {
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .pImmutableSamplers = NULL,
            },
            {
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    VK_CHECK(createDescriptorSetLayout(pVulkan, 2, bindings, pSetLayout));
}

static VkResult createSetLayouts_Std(const FbrVulkan *pVulkan, FbrDescriptors_Std *pDescriptors_Std) {
    createSetLayout_VertUBO_FragSampler(pVulkan, &pDescriptors_Std->dsl_VertUBO_FragSampler);
}

VkResult fbrCreateDescriptorSet_CameraUBO_TextureSampler(const FbrVulkan *pVulkan,
                                                         FbrSetLayout_VertUBO_FragSampler setLayout,
                                                         const FbrCamera *pCamera,
                                                         const FbrTexture *pTexture,
                                                         FbrDescriptorSet_CameraUBO_TextureSampler *pDescriptorSet) {
    VK_CHECK(allocateDescriptorSet(pVulkan, 1, &setLayout, pDescriptorSet));
    const VkDescriptorBufferInfo bufferInfo = {
            .buffer = pCamera->pUBO->uniformBuffer,
            .offset = 0,
            .range = sizeof(FbrCameraUBO),
    };
    const VkDescriptorImageInfo imageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pTexture->imageView,
            .sampler = pVulkan->sampler,
    };
    const VkWriteDescriptorSet descriptorWrites[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = VK_NULL_HANDLE,
                    .dstSet = *pDescriptorSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pImageInfo = VK_NULL_HANDLE,
                    .pBufferInfo = &bufferInfo,
                    .pTexelBufferView = VK_NULL_HANDLE,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = VK_NULL_HANDLE,
                    .dstSet = *pDescriptorSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &imageInfo,
                    .pBufferInfo = VK_NULL_HANDLE,
                    .pTexelBufferView = VK_NULL_HANDLE,
            },
    };
    vkUpdateDescriptorSets(pVulkan->device, 2, descriptorWrites, 0, NULL);
}

VkResult fbrCreateDescriptors_Std(const FbrVulkan *pVulkan, FbrDescriptors_Std **ppAllocDescriptors_Std) {
    *ppAllocDescriptors_Std = calloc(1, sizeof(FbrDescriptors_Std));
    FbrDescriptors_Std *pDescriptors_Std = *ppAllocDescriptors_Std;

    createSetLayouts_Std(pVulkan, pDescriptors_Std);
}

VkResult fbrDestroyDescriptors_Std(const FbrVulkan *pVulkan,
                                  FbrDescriptors_Std *pDescriptors_Std) {
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors_Std->dsl_VertUBO_FragSampler, NULL);
}