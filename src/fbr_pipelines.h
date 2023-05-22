#ifndef FABRIC_STANDARD_PIPELINES_H
#define FABRIC_STANDARD_PIPELINES_H

#include "fbr_app.h"
#include "fbr_vulkan.h"

typedef VkPipelineLayout FbrPipeLayoutStandard;
typedef VkPipeline FbrPipeStandard;

typedef VkPipelineLayout FbrPipeLayoutNode;
typedef VkPipeline FbrPipeNode;

typedef VkPipelineLayout FbrComputePipeLayoutComposite;
typedef VkPipeline FbrComputePipeComposite;

typedef struct FbrPipelines {
    FbrPipeLayoutStandard pipeLayoutStandard;
    FbrPipeStandard pipeStandard;

    FbrPipeLayoutNode pipeLayoutNode;
    FbrPipeNode pipeNode;

    FbrComputePipeLayoutComposite computePipeLayoutComposite;
    FbrComputePipeComposite computePipeComposite;
} FbrPipelines;

VkResult fbrCreatePipeStandard(const FbrVulkan *pVulkan,
                               VkPipelineLayout layout,
                               const char *pVertShaderPath,
                               const char *pFragShaderPath,
                               VkPipeline *pPipe);

VkResult fbrCreatePipelines(const FbrVulkan *pVulkan,
                            const FbrDescriptors *pDescriptors,
                            FbrPipelines **ppAllocPipes);

FBR_RESULT fbrCreateComputePipeComposite(const FbrVulkan *pVulkan,
                                         FbrComputePipeLayoutComposite layout,
                                         const char *pCompositeShaderPath,
                                         FbrComputePipeComposite *pPipe);

void fbrDestroyPipelines(const FbrVulkan *pVulkan,
                                 FbrPipelines *pPipelines);

#endif //FABRIC_STANDARD_PIPELINES_H
