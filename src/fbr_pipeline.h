#ifndef FABRIC_PIPELINE_H
#define FABRIC_PIPELINE_H

#include "fbr_app.h"

typedef struct FbrPipeline {
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
} FbrPipeline;

void fbrCreatePipeline(const FbrVulkan *pVulkan,
                       VkRenderPass renderPass,
                       uint32_t setLayoutCount,
                       const VkDescriptorSetLayout *pSetLayouts,
                       const char* pVertShaderPath,
                       const char* pFragShaderPath,
                       FbrPipeline **ppAllocPipeline);

void fbrCleanupPipeline(const FbrVulkan *pVulkan, FbrPipeline *pPipeline);

#endif //FABRIC_PIPELINE_H
