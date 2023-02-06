#ifndef FABRIC_PIPELINE_H
#define FABRIC_PIPELINE_H

#include "fbr_app.h"

typedef struct FbrPipeline {
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorSetLayout descriptorSetLayout;
//    VkDescriptorSet descriptorSet;
} FbrPipeline;

void fbrInitDescriptorSet(const FbrVulkan *pVulkan,
                          const FbrPipeline *pPipeline,
                          const FbrCamera *pCameraState,
                          VkImageView imageView,
                          VkDescriptorSet *descriptorSet);

void fbrCreatePipeline(const FbrVulkan *pVulkan,
                       const FbrCamera *pCameraState,
//                       VkImageView imageView,
                       VkRenderPass renderPass,
                       FbrPipeline **ppAllocPipeline);

void fbrCleanupPipeline(const FbrVulkan *pVulkan, FbrPipeline *pPipeline);

#endif //FABRIC_PIPELINE_H
