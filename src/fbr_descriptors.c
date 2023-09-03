#include "fbr_descriptors.h"
#include "fbr_vulkan.h"
#include "fbr_camera.h"
#include "fbr_texture.h"
#include "fbr_macros.h"

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
    FBR_ACK(createDescriptorSetLayout(pVulkan,
                                      1,
                                      bindings,
                                      pSetLayout));

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
    FBR_ACK(createDescriptorSetLayout(pVulkan,
                                      1,
                                      bindings,
                                      pSetLayout));

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

static FBR_RESULT createSetLayoutGlobal(const FbrVulkan *pVulkan, FbrSetLayoutGlobal *pSetLayout)
{
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    // figure someway to selectively enable/disable these as needed?
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                                  VK_SHADER_STAGE_COMPUTE_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT |
                                  VK_SHADER_STAGE_MESH_BIT_EXT,
                    .pImmutableSamplers = NULL,
            }
    };
    FBR_ACK(createDescriptorSetLayout(pVulkan,
                                      1,
                                      bindings,
                                      pSetLayout));
    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateSetGlobal(const FbrVulkan *pVulkan,
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
            .range = sizeof(FbrCameraBuffer),
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
    return FBR_SUCCESS;
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
            .sampler = pVulkan->linearSampler,
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
            .sampler = pVulkan->linearSampler,
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

static FBR_RESULT createSetLayoutNode(const FbrVulkan *pVulkan, FbrSetLayoutNode *pSetLayout)
{
    VkDescriptorSetLayoutBinding pBindings[] = {
            {// transform ubo
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                            VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            },
            {// camera rendered from ubo
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                                VK_SHADER_STAGE_FRAGMENT_BIT,
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
                                      COUNT(pBindings),
                                      pBindings,
                                      pSetLayout));

    return FBR_SUCCESS;
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
            .sampler = pVulkan->linearSampler,
    };
    const VkDescriptorImageInfo normalImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pNormalTexture->imageView,
            .sampler = pVulkan->linearSampler,
    };
    const VkDescriptorImageInfo depthImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pDepthTexture->imageView,
            .sampler = pVulkan->linearSampler,
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
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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
                           COUNT(pDescriptorWrites),
                           pDescriptorWrites,
                           0,
                           NULL);
    return FBR_SUCCESS;
}

static FBR_RESULT createSetLayoutComposite(const FbrVulkan *pVulkan, FbrSetLayoutComposite *pSetLayout)
{
    VkDescriptorSetLayoutBinding pBindings[] = {
            {// input color
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = NULL,
            },
            {// input normal
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = NULL,
            },
            {// input depth
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = NULL,
            },
            {// input node depth
                    .binding = 3,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = NULL,
            },
            {// output color
                    .binding = 4,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = NULL,
            }
    };
    FBR_ACK(createDescriptorSetLayout(pVulkan,
                                      COUNT(pBindings),
                                      pBindings,
                                      pSetLayout));

    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateSetComposite(const FbrVulkan *pVulkan,
                                 FbrSetLayoutComposite setLayout,
                                 VkImageView inputColorImageView,
                                 VkImageView inputNormalImageView,
                                 VkImageView inputGBufferImageView,
                                 VkImageView inputDepthImageView,
                                 VkImageView outputColorImageView,
                                 FbrSetComposite *pSet)
{
    FBR_ACK(allocateDescriptorSet(pVulkan,
                                  1,
                                  &setLayout,
                                  pSet));

    const VkDescriptorImageInfo inputColorImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = inputColorImageView,
            .sampler = pVulkan->linearSampler,
    };
    const VkDescriptorImageInfo inputNormalImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = inputNormalImageView,
            .sampler = pVulkan->linearSampler,
    };
    const VkDescriptorImageInfo inputGBufferImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = inputGBufferImageView,
            .sampler = pVulkan->linearSampler,
    };
    const VkDescriptorImageInfo inputDepthImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = inputDepthImageView,
            .sampler = pVulkan->linearSampler,
    };
    const VkDescriptorImageInfo outputColorImageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            .imageView = outputColorImageView,
//            .linearSampler = pVulkan->linearSampler,
    };
    const VkWriteDescriptorSet pDescriptorWrites[] = {
            // input color
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &inputColorImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
            // input normal
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &inputNormalImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
            // input g buffer
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 2,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &inputGBufferImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
            // input depth
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 3,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &inputDepthImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
            // output color
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = *pSet,
                    .dstBinding = 4,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &outputColorImageInfo,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL,
            },
    };
    vkUpdateDescriptorSets(pVulkan->device,
                           COUNT(pDescriptorWrites),
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