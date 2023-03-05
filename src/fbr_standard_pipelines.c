#include "fbr_standard_pipelines.h"
#include "fbr_camera.h"
#include "fbr_texture.h"

// not used is this a good idea!?

//void fbrCreateUnlitStandardPipeline(const FbrVulkan *pVulkan, FbrCamera *pCamera, FbrTexture *pTexture, FbrPipeline *pPipeline){
//    VkDescriptorSetLayout descriptorSetLayout

//    VkDescriptorSetLayoutBinding bindings[] = {
//            {
//                    .binding = 0,
//                    .descriptorCount = 1,
//                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//                    .pImmutableSamplers = NULL,
//                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
//            },
//            {
//                    .binding = 1,
//                    .descriptorCount = 1,
//                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//                    .pImmutableSamplers = NULL,
//                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
//            }
//    };
//
//
//    VkDescriptorBufferInfo bufferInfo = {
//            .buffer = pCamera->ubo.uniformBuffer,
//            .offset = 0,
//            .range = sizeof(FbrCameraUBO),
//    };
//    VkDescriptorImageInfo imageInfo = {
//            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//            .imageView = pTexture->imageView,
//            .sampler = pVulkan->sampler,
//    };
//    VkWriteDescriptorSet descriptorWrites[] = {
//            {
//                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//                    .dstSet = *pDescriptorSet,
//                    .dstBinding = 0,
//                    .dstArrayElement = 0,
//                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//                    .descriptorCount = 1,
//                    .pBufferInfo = &bufferInfo,
//            },
//            {
//                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//                    .dstSet = *pDescriptorSet,
//                    .dstBinding = 1,
//                    .dstArrayElement = 0,
//                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//                    .descriptorCount = 1,
//                    .pImageInfo = &imageInfo,
//            },
//    };


//}