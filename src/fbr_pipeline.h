#ifndef FABRIC_PIPELINE_H
#define FABRIC_PIPELINE_H

#include <vulkan/vulkan.h>
#include "fbr_app.h"

#define FBR_DESCRIPTOR_SET_COUNT 1
#define FBR_DESCRIPTOR_COUNT 2
#define FBR_BINDING_DESCRIPTOR_COUNT 1
#define FBR_ATTRIBUTE_DESCRIPTOR_COUNT 3

typedef struct FbrPipeline {
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
} FbrPipeline;

void fbrCreatePipeline(const FbrApp *pApp,
                       const FbrCamera *pCameraState,
                       const FbrTexture *pTexture,
                       FbrPipeline **ppAllocPipeline);

void fbrFreePipeline(const FbrApp *pApp, FbrPipeline *pPipeline);

#endif //FABRIC_PIPELINE_H
