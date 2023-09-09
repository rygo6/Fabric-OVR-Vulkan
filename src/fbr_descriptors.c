#include "fbr_descriptors.h"
#include "fbr_vulkan.h"
#include "fbr_camera.h"
#include "fbr_texture.h"
#include "fbr_macros.h"

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

#define FBR_SET_LAYOUT_BEGIN(name) \
static FBR_RESULT createSetLayout##name(const FbrVulkan *pVulkan, FbrDescriptors *pDescriptors) \
{ \
    FbrSetLayout *pSetLayout = &pDescriptors->setLayout##name; \
    VkDescriptorSetLayoutBinding pBindings[] = {

#define FBR_SET_LAYOUT_END \
    }; \
    FBR_ACK(createDescriptorSetLayout(pVulkan, \
        COUNT(pBindings), \
        pBindings, \
        pSetLayout)); \
    return FBR_SUCCESS; \
}

#define FBR_CREATE_DESCRIPTOR_BEGIN(name) \
{ \
FBR_ACK(allocateDescriptorSet(pVulkan, \
                              &pDescriptors->setLayout##name, \
                              pSet));
#define FBR_CREATE_DESCRIPTOR_END \
    vkUpdateDescriptorSets(pVulkan->device, \
        COUNT(pDescriptorWrites), \
        pDescriptorWrites, \
        0, \
        NULL); \
    return FBR_SUCCESS; \
}

FBR_RESULT fbrCreateSetGlobal(const FbrVulkan *pVulkan,
                              const FbrDescriptors *pDescriptors,
                              const FbrCamera *pCamera,
                              FbrSetPass *pSet)
FBR_CREATE_DESCRIPTOR_BEGIN(Global)
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
FBR_CREATE_DESCRIPTOR_END
FBR_SET_LAYOUT_BEGIN(Global)
                    {
                            .binding = 0,
                            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            .descriptorCount = 1,
                            // figure someway to selectively enable/disable these as needed?
                            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                          VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                                          VK_SHADER_STAGE_COMPUTE_BIT |
                                          VK_SHADER_STAGE_FRAGMENT_BIT |
                                          VK_SHADER_STAGE_MESH_BIT_EXT |
                                          VK_SHADER_STAGE_TASK_BIT_EXT,
                            .pImmutableSamplers = NULL,
                    },
FBR_SET_LAYOUT_END

FBR_RESULT fbrCreateSetPass(const FbrVulkan *pVulkan,
                            const FbrDescriptors *pDescriptors,
                            const FbrTexture *pNormalTexture,
                            FbrSetPass *pSet)
FBR_CREATE_DESCRIPTOR_BEGIN(Pass)
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
FBR_CREATE_DESCRIPTOR_END
FBR_SET_LAYOUT_BEGIN(Pass)
                    {// normal
                            .binding = 0,
                            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            .descriptorCount = 1,
                            .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                                          VK_SHADER_STAGE_FRAGMENT_BIT,
                            .pImmutableSamplers = NULL,
                    },
FBR_SET_LAYOUT_END

FBR_RESULT fbrCreateSetMaterial(const FbrVulkan *pVulkan,
                                const FbrDescriptors *pDescriptors,
                                const FbrTexture *pTexture,
                                FbrSetMaterial *pSet)
FBR_CREATE_DESCRIPTOR_BEGIN(Material)
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
FBR_CREATE_DESCRIPTOR_END
FBR_SET_LAYOUT_BEGIN(Material)
                    {
                            .binding = 0,
                            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            .descriptorCount = 1,
                            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                            .pImmutableSamplers = NULL,
                    },
FBR_SET_LAYOUT_END

FBR_RESULT fbrCreateSetObject(const FbrVulkan *pVulkan,
                              const FbrDescriptors *pDescriptors,
                              const FbrTransform *pTransform,
                              FbrSetObject *pSet)
FBR_CREATE_DESCRIPTOR_BEGIN(Object)
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
FBR_CREATE_DESCRIPTOR_END
FBR_SET_LAYOUT_BEGIN(Object)
                    {
                            .binding = 0,
                            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            .descriptorCount = 1,
                            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                          VK_SHADER_STAGE_FRAGMENT_BIT,
                            .pImmutableSamplers = NULL,
                    },
FBR_SET_LAYOUT_END

FBR_RESULT fbrCreateSetNode(const FbrVulkan *pVulkan,
                            const FbrDescriptors *pDescriptors,
                            const FbrTransform *pTransform,
                            const FbrCamera *pCamera,
                            const FbrTexture *pColorTexture,
                            const FbrTexture *pNormalTexture,
                            const FbrTexture *pDepthTexture,
                            FbrSetNode *pSet)
FBR_CREATE_DESCRIPTOR_BEGIN(Node)
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
FBR_CREATE_DESCRIPTOR_END
FBR_SET_LAYOUT_BEGIN(Node)
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
FBR_SET_LAYOUT_END

FBR_RESULT fbrCreateSetComposite(const FbrVulkan *pVulkan,
                                 const FbrDescriptors *pDescriptors,
                                 VkImageView inputColorImageView,
                                 VkImageView inputNormalImageView,
                                 VkImageView inputGBufferImageView,
                                 VkImageView inputDepthImageView,
                                 VkImageView outputColorImageView,
                                 FbrSetComposite *pSet)
FBR_CREATE_DESCRIPTOR_BEGIN(Composite)
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
FBR_CREATE_DESCRIPTOR_END
FBR_SET_LAYOUT_BEGIN(Composite)
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
FBR_SET_LAYOUT_END

static FBR_RESULT createSetLayouts(const FbrVulkan *pVulkan, FbrDescriptors *pDescriptors)
{
    createSetLayoutGlobal(pVulkan, pDescriptors);
    createSetLayoutPass(pVulkan, pDescriptors);
    createSetLayoutMaterial(pVulkan, pDescriptors);
    createSetLayoutObject(pVulkan, pDescriptors);
    createSetLayoutNode(pVulkan, pDescriptors);
    createSetLayoutComposite(pVulkan, pDescriptors);
    return FBR_SUCCESS;
}

FBR_RESULT fbrCreateDescriptors(const FbrVulkan *pVulkan,
                              FbrDescriptors **ppAllocDescriptors)
{
    *ppAllocDescriptors = calloc(1, sizeof(FbrDescriptors));
    FbrDescriptors *pDescriptors_Std = *ppAllocDescriptors;
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
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutComposite.layout, NULL);
    vkDestroyDescriptorSetLayout(pVulkan->device, pDescriptors->setLayoutNode.layout, NULL);

    vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &pDescriptors->setGlobal);
    if (pDescriptors->setPass != NULL)
        vkFreeDescriptorSets(pVulkan->device, pVulkan->descriptorPool, 1, &pDescriptors->setPass);



    free(pDescriptors);
}