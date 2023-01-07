#ifndef FABRIC_PIPELINE_H
#define FABRIC_PIPELINE_H

#include "fbr_app.h"

typedef struct FbrPipeline {
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
} FbrPipeline;

void fbrCreatePipeline(const FbrVulkan *pVulkan,
                       const FbrCamera *pCameraState,
                       const VkImageView imageView,
                       FbrPipeline **ppAllocPipeline);

void fbrCleanupPipeline(const FbrVulkan *pVulkan, FbrPipeline *pPipeline);

#endif //FABRIC_PIPELINE_H
