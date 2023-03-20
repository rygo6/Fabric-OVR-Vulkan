#ifndef FABRIC_STANDARD_PIPELINES_H
#define FABRIC_STANDARD_PIPELINES_H

#include "fbr_app.h"

typedef VkPipelineLayout FbrPipeLayoutStandard;
typedef VkPipeline FbrPipeStandard;

typedef VkPipelineLayout FbrPipeLayoutNode;
typedef VkPipeline FbrPipeNode;

typedef struct FbrPipelines {
    FbrPipeLayoutStandard pipeLayoutStandard;
    FbrPipeStandard pipeStandard;

    FbrPipeLayoutNode pipeLayoutNode;
    FbrPipeNode pipeNode;
} FbrPipelines;

VkResult fbrCreatePipeStandard(const FbrVulkan *pVulkan,
                               VkPipelineLayout layout,
                               const char *pVertShaderPath,
                               const char *pFragShaderPath,
                               VkPipeline *pPipe);

VkResult fbrCreatePipelines(const FbrVulkan *pVulkan,
                            const FbrDescriptors *pDescriptors,
                            FbrPipelines **ppAllocPipes);

void fbrDestroyPipelines(const FbrVulkan *pVulkan,
                                 FbrPipelines *pPipelines);

#endif //FABRIC_STANDARD_PIPELINES_H
