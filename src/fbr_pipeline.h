#ifndef FABRIC_PIPELINE_H
#define FABRIC_PIPELINE_H

#include "fbr_app.h"

typedef struct FbrPipeline {
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorSetLayout descriptorSetLayout;



} FbrPipeline;

void fbrInitDescriptorSet(const FbrVulkan *pVulkan,
                          const FbrCamera *pCameraState,
                          VkDescriptorSetLayout descriptorSetLayout,
                          VkImageView imageView,
                          VkDescriptorSet *pDescriptorSet);

void fbrCreatePipeline(const FbrVulkan *pVulkan,
                       VkRenderPass renderPass,
                       const char* pVertShaderPath,
                       const char* pFragShaderPath,
                       FbrPipeline **ppAllocPipeline);

void fbrCleanupPipeline(const FbrVulkan *pVulkan, FbrPipeline *pPipeline);

#endif //FABRIC_PIPELINE_H
