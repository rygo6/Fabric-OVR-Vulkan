#include "fbr_descriptors.h"
#include "fbr_vulkan.h"
#include "fbr_camera.h"
#include "fbr_texture.h"
#include "fbr_macros.h"
#include "fbr_log.h"
#include "stb_ds.h"

VkDescriptorSetLayoutBinding *pLayoutBindingArray = NULL;
VkWriteDescriptorSet *pDescriptorWriteArray = NULL;

#define CREATE_SET_LAYOUT(name) \
static FBR_RESULT createSetLayout##name(const FbrVulkan *pVulkan, FbrSetLayout##name *pSetLayout)

#define END_SET_LAYOUT \
FBR_ACK(createDescriptorSetLayout(pVulkan, \
    COUNT(pBindings), \
    pBindings, \
    pSetLayout)); \
return FBR_SUCCESS;

#define END_PUSH_SET_LAYOUT \
FBR_ACK(createDescriptorSetLayout(pVulkan, \
                                  arrlen(pLayoutBindingArray), \
                                  pLayoutBindingArray, \
                                  pSetLayout)); \
arrsetlen(pLayoutBindingArray, 0); \
return FBR_SUCCESS;

#define CREATE_SET(name, ...) \
FBR_RESULT fbrCreateSet##name( \
    const FbrVulkan *pVulkan, \
    const FbrDescriptors *pDescriptors, \
    __VA_ARGS__, \
    FbrSet##name *pSet)

#define BEGIN_SET(name) \
    FBR_ACK(allocateDescriptorSet(pVulkan, \
                                  &pDescriptors->setLayout##name, \
                                  pSet));

#define END_SET \
    vkUpdateDescriptorSets(pVulkan->device, \
        COUNT(pDescriptorWrites), \
        pDescriptorWrites, \
        0, \
        NULL); \
    return FBR_SUCCESS;

#define END_PUSH_SET \
vkUpdateDescriptorSets(pVulkan->device, \
    arrlen(pDescriptorWriteArray), \
    pDescriptorWriteArray, \
    0, \
    NULL); \
arrsetlen(pDescriptorWriteArray, 0); \
return FBR_SUCCESS;

static void pushDescriptorWrite(VkWriteDescriptorSet descriptorWrite, VkDescriptorSet *pSet)
{
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = *pSet;
    descriptorWrite.dstBinding = arrlen(pDescriptorWriteArray);
    descriptorWrite.descriptorCount = descriptorWrite.descriptorCount == 0 ? 1 : descriptorWrite.descriptorCount;
    FBR_LOG_DEBUG(descriptorWrite.dstBinding);
    arrput(pDescriptorWriteArray, descriptorWrite);
}

static void pushLayoutBinding(VkDescriptorSetLayoutBinding binding)
{
    binding.binding = arrlen(pLayoutBindingArray);
    binding.descriptorCount = binding.descriptorCount == 0 ? 1 : binding.descriptorCount;
            arrput(pLayoutBindingArray, binding);
}

static void setDefaultDescriptorLayoutParams(int count, VkDescriptorSetLayoutBinding *pBindings)
{
    for (int i = 0; i < count; ++i) {
        pBindings[i].binding = i;
    }
}

static void setDefaultDescriptorParams(int count, VkWriteDescriptorSet *pDescriptorWrites, VkDescriptorSet *pSet)
{
    for (int i = 0; i < count; ++i) {
        pDescriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pDescriptorWrites[i].dstSet = *pSet;
        pDescriptorWrites[i].dstBinding = i;
        pDescriptorWrites[i].descriptorCount = 1;
    }
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

CREATE_SET(Global,
           const FbrCamera *pCamera)
{
    BEGIN_SET(Global)
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
    END_SET
}

CREATE_SET_LAYOUT(Global)
{
    const VkDescriptorSetLayoutBinding pBindings[] = {
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
    };
    END_SET_LAYOUT
}

CREATE_SET(Pass,
           const FbrTexture *pNormalTexture)
{
    BEGIN_SET(Pass)
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
    END_SET;
}

CREATE_SET_LAYOUT(Pass)
{
    const VkDescriptorSetLayoutBinding pBindings[] = {
            {// normal
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            },
    };
    END_SET_LAYOUT
}

CREATE_SET(Material,
           const FbrTexture *pTexture)
{
    BEGIN_SET(Material)
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
    END_SET
}

CREATE_SET_LAYOUT(Material)
{
    const VkDescriptorSetLayoutBinding pBindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            },
    };
    END_SET_LAYOUT;
}

/// Object
CREATE_SET(Object,
           const FbrTransform *pTransform)
{
    BEGIN_SET(Object)
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
    END_SET
}

CREATE_SET_LAYOUT(Object)
{
    const VkDescriptorSetLayoutBinding pBindings[] = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
            },
    };
    END_SET_LAYOUT
}

CREATE_SET(Node,
            const FbrTransform *pTransform,
            const FbrCamera *pCamera,
            const FbrTexture *pColorTexture,
            const FbrTexture *pNormalTexture,
            const FbrTexture *pDepthTexture)
{
    BEGIN_SET(Node)
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
    END_PUSH_SET
}

CREATE_SET_LAYOUT(Node)
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
    END_PUSH_SET_LAYOUT
}

/// ComputeComposite
CREATE_SET(ComputeComposite,
           VkImageView inputColorImageView,
           VkImageView inputNormalImageView,
           VkImageView inputGBufferImageView,
           VkImageView inputDepthImageView,
           VkImageView outputColorImageView)
{
    BEGIN_SET(ComputeComposite)
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
    END_SET
}

CREATE_SET_LAYOUT(ComputeComposite)
{
    const VkDescriptorSetLayoutBinding pBindings[] = {
            {// input color
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = NULL,
            },
            {// input normal
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
    END_SET_LAYOUT
}

CREATE_SET(MeshComposite,
           FbrCamera *pNodeCamera,
           VkImageView inputColorImageView,
           VkImageView inputNormalImageView,
           VkImageView inputGBufferImageView,
           VkImageView inputDepthImageView)
{
    BEGIN_SET(MeshComposite)
    const VkDescriptorBufferInfo cameraBufferInfo = {
            .buffer = pNodeCamera->pUBO->uniformBuffer,
            .offset = 0,
            .range = sizeof(FbrCamera),
    };
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
    VkWriteDescriptorSet pDescriptorWrites[] = {
            {// camera UBO from which it was rendered
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &cameraBufferInfo,
            },
            // input color
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &inputColorImageInfo,
            },
            // input normal
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &inputNormalImageInfo,
            },
            // input g buffer
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &inputGBufferImageInfo,
            },
            // input depth
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &inputDepthImageInfo,
            },
    };
    setDefaultDescriptorParams(COUNT(pDescriptorWrites), pDescriptorWrites, pSet);
    END_SET
}

CREATE_SET_LAYOUT(MeshComposite)
{
    VkDescriptorSetLayoutBinding pBindings[] = {
            {// camera rendered from ubo
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
            },
            {// input color
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
            },
            {// input normal
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
            },
            {// input depth
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
            },
            {// input node depth
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
            },
    };
    setDefaultDescriptorLayoutParams(COUNT(pBindings), pBindings);
    END_SET_LAYOUT
}

static FBR_RESULT createSetLayouts(const FbrVulkan *pVulkan, FbrDescriptors *pDescriptors)
{
    createSetLayoutGlobal(pVulkan, &pDescriptors->setLayoutGlobal);
    createSetLayoutPass(pVulkan, &pDescriptors->setLayoutPass);
    createSetLayoutMaterial(pVulkan, &pDescriptors->setLayoutMaterial);
    createSetLayoutObject(pVulkan, &pDescriptors->setLayoutObject);
    createSetLayoutNode(pVulkan, &pDescriptors->setLayoutNode);
    createSetLayoutComputeComposite(pVulkan, &pDescriptors->setLayoutComputeComposite);
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