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

void fbrCreatePipeline(const FbrAppState* pAppState, const FbrCameraState* pCameraState, FbrPipeline** ppAllocPipeline);
void fbrFreePipeline(const FbrAppState* pAppState, FbrPipeline* restrict pPipeline);

#endif //FABRIC_PIPELINE_H
