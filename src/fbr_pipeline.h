#ifndef FABRIC_PIPELINE_H
#define FABRIC_PIPELINE_H

#include <vulkan/vulkan.h>
#include "fbr_app.h"

typedef struct FbrPipeline {
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
} FbrPipeline;

void fbrCreatePipeline(const FbrApp *pApp, const FbrCamera *pCameraState, FbrPipeline **ppAllocPipeline);

void fbrFreePipeline(const FbrApp *pApp, FbrPipeline *pPipeline);

#endif //FABRIC_PIPELINE_H
